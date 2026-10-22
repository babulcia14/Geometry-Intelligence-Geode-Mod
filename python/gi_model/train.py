"""Train a new model or resume our own checkpoint; never downloads weights."""
import argparse
from dataclasses import asdict
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import random
import time
import torch
from .model import TinyGI, ModelConfig, PAD, dialogue
from .tokenizer import Tokenizer


def load_records(path):
    raw = Path(path).read_bytes()
    if len(raw) > 128 * 1024 * 1024:
        raise ValueError("Use a dataset under 128 MiB for this starter trainer")
    rows = []
    for line in raw.decode("utf-8-sig").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if not row.get("license") or not row.get("source") or not row.get("group"):
            raise ValueError("Each record needs license, source and group provenance")
        messages = row.get("messages")
        if not isinstance(messages, list) or not messages or messages[-1].get("role") != "assistant":
            raise ValueError("Each training conversation must end in an assistant response")
        for m in messages:
            if m.get("role") not in {"user", "assistant"} or not isinstance(m.get("content"), str) or not m["content"].strip():
                raise ValueError("Invalid message")
        rows.append(row)
    if len(rows) < 12:
        raise ValueError("At least 12 records are required for a train/validation split")
    return rows, hashlib.sha256(raw).hexdigest()


def split_records(rows, seed):
    # Related paraphrases stay in one split; no train/validation overlap by group.
    groups = sorted({r["group"] for r in rows})
    if len(groups) < 3:
        raise ValueError("At least 3 distinct groups required")
    random.Random(seed).shuffle(groups)
    held = set(groups[:max(1, len(groups) // 10)])
    return ([r for r in rows if r["group"] not in held], [r for r in rows if r["group"] in held])


def encode_rows(rows, tokenizer=None):
    return [dialogue(row["messages"], training=True, tokenizer=tokenizer) for row in rows]


def batch(data, size, context, rng, device):
    xs, ys = [], []
    for _ in range(size):
        tokens, labels = rng.choice(data)
        # Pick a window that contains a supervised token; all-user crops have
        # undefined cross entropy and are rejected before entering the model.
        valid = [i for i in range(1, len(labels)) if labels[i] != -100]
        target = rng.choice(valid)
        start = max(0, target - rng.randrange(1, context + 1))
        # Retain the question/answer boundary in half the short examples. Pure
        # random crops often drop the question and train only text continuation.
        if valid[0] < context and rng.random() < 0.5:
            start = 0
        x, y = tokens[start:start + context], labels[start + 1:start + context + 1]
        n = min(len(x), len(y))
        x, y = x[:n], y[:n]
        if not any(v != -100 for v in y):
            start = max(0, target - context)
            x, y = tokens[start:target], labels[start + 1:target + 1]
        xs.append(x + [PAD] * (context - len(x)))
        ys.append(y + [-100] * (context - len(y)))
    return torch.tensor(xs, device=device), torch.tensor(ys, device=device)


@torch.inference_mode()
def evaluate(model, data, batch_size, seed, device):
    model.eval()
    rng = random.Random(seed)
    losses = [model(*batch(data, batch_size, model.cfg.context, rng, device))[1].item() for _ in range(8)]
    model.train()
    return sum(losses) / len(losses)


def save(path, model, optimizer, step, meta, rng):
    payload = {"format": 1, "origin": "random-initialization", "config": asdict(model.cfg),
               "weights": model.state_dict(), "optimizer": optimizer.state_dict(), "step": step,
               "metadata": meta, "rng": rng.getstate(), "torch_rng": torch.get_rng_state(),
               "tokenizer": model.tokenizer.state()}
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix(".tmp")
    torch.save(payload, temp)
    temp.replace(path)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--data", default="data/starter.jsonl")
    p.add_argument("--out", default="models/gi-tiny")
    p.add_argument("--steps", type=int, default=2000)
    p.add_argument("--batch", type=int, default=8)
    p.add_argument("--context", type=int, default=256)
    p.add_argument("--width", type=int, default=96)
    p.add_argument("--layers", type=int, default=3)
    p.add_argument("--heads", type=int, default=4)
    p.add_argument("--threads", type=int, default=4)
    p.add_argument("--seed", type=int, default=20261021)
    p.add_argument("--device", choices=["cpu", "cuda"], default="cpu")
    p.add_argument("--resume", type=Path)
    p.add_argument("--merges", type=int, default=0, help="Learn 0-4096 byte-pair merges on training text only")
    args = p.parse_args()
    if not (1 <= args.steps <= 1000000 and 1 <= args.batch <= 128 and 1 <= args.threads <= 64):
        p.error("Invalid steps/batch/threads")
    torch.set_num_threads(args.threads)
    torch.manual_seed(args.seed)
    rng = random.Random(args.seed)
    rows, digest = load_records(args.data)
    train_rows, validation_rows = split_records(rows, args.seed)
    old = torch.load(args.resume, map_location=args.device, weights_only=True) if args.resume else None
    tokenizer = Tokenizer.from_state(old.get("tokenizer")) if old else Tokenizer.train(
        [m["content"] for row in train_rows for m in row["messages"]], args.merges)
    train, validation = encode_rows(train_rows, tokenizer), encode_rows(validation_rows, tokenizer)
    cfg = ModelConfig(args.context, args.width, args.layers, args.heads, vocab=tokenizer.vocab)
    model = TinyGI(cfg, tokenizer).to(args.device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=0.001, weight_decay=0.01)
    start, best = 0, math.inf
    if args.resume:
        if old.get("origin") != "random-initialization" or ModelConfig(**old["config"]) != cfg:
            raise ValueError("Resume checkpoint has a different architecture or origin")
        if old["metadata"]["dataset_sha256"] != digest:
            raise ValueError("Resume requires exactly the same dataset; start a new run for changed data")
        model.load_state_dict(old["weights"])
        optimizer.load_state_dict(old["optimizer"])
        rng.setstate(old["rng"])
        torch.set_rng_state(old["torch_rng"].cpu())
        start = old["step"]
        best = old["metadata"].get("best_validation_loss", math.inf)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    meta = {"created_utc": datetime.now(timezone.utc).isoformat(), "seed": args.seed,
            "parameters": sum(p.numel() for p in model.parameters()), "dataset_sha256": digest,
            "training_records": len(train), "validation_records": len(validation),
            "training_bytes": sum(len(json.dumps(r, ensure_ascii=False).encode()) for r in train_rows),
            "sources": sorted({r["source"] for r in rows}), "licenses": sorted({r["license"] for r in rows}),
            "device": args.device, "torch_version": str(torch.__version__),
            "tokenizer_merges": len(tokenizer.merges), "context": cfg.context,
            "training_token_positions": sum(len(t) for t,_ in train),
            "training_recipe": "assistant-boundary crops v2; seed and weights alone do not reproduce old v1 runs",
            "limitations": "Tiny starter corpus; not a validated conversational assistant. No pretrained weights."}
    print(json.dumps(meta, ensure_ascii=False), flush=True)
    began = time.monotonic()
    model.train()
    step = start
    try:
        with (out / "metrics.jsonl").open("a", encoding="utf-8") as log:
            for step in range(start + 1, args.steps + 1):
                warmup = min(1.0, step / 50)
                lr = 0.001 * warmup * (0.15 + 0.85 * (1 + math.cos(math.pi * step / args.steps)) / 2)
                for group in optimizer.param_groups:
                    group["lr"] = lr
                x, y = batch(train, args.batch, cfg.context, rng, args.device)
                optimizer.zero_grad(set_to_none=True)
                loss = model(x, y)[1]
                if not torch.isfinite(loss):
                    raise RuntimeError("Non-finite training loss; checkpoint not overwritten")
                loss.backward()
                torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
                optimizer.step()
                if step == 1 or step % 100 == 0 or step == args.steps:
                    val = evaluate(model, validation, args.batch, args.seed, args.device)
                    record = {"step": step, "train_loss": loss.item(), "validation_loss": val,
                              "elapsed_seconds": round(time.monotonic() - began, 2), "lr": lr}
                    print(json.dumps(record), flush=True)
                    log.write(json.dumps(record) + "\n"); log.flush()
                    meta.update(record)
                    improved = val < best
                    best = min(best, val)
                    meta["best_validation_loss"] = best
                    save(out / "latest.pt", model, optimizer, step, meta, rng)
                    if improved:
                        save(out / "best.pt", model, optimizer, step, meta, rng)
                    (out / "training-report.json").write_text(json.dumps(meta, indent=2, ensure_ascii=False), encoding="utf-8")
    except KeyboardInterrupt:
        save(out / "interrupted.pt", model, optimizer, step, meta, rng)
        print("Stopped. Saved interrupted.pt; use --resume to continue.")


if __name__ == "__main__":
    main()
