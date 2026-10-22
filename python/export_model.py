"""Export our checkpoint without optimizer/RNG state for inference and packaging."""
import argparse
from pathlib import Path
import hashlib
import json
import torch
from gi_model.model import load_model

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--checkpoint", default="models/gi-tiny/best.pt")
p.add_argument("--out", default="models/gi-tiny/model.pt")
args = p.parse_args()
_, saved = load_model(args.checkpoint)
payload = {k:saved[k] for k in ["format", "origin", "config", "weights", "step", "metadata"]}
if "tokenizer" in saved:
    payload["tokenizer"] = saved["tokenizer"]
payload["export"] = {"source_run":Path(args.checkpoint).parent.name, "source_file":Path(args.checkpoint).name}
dest = Path(args.out)
dest.parent.mkdir(parents=True, exist_ok=True)
torch.save(payload, dest.with_suffix(".tmp"))
dest.with_suffix(".tmp").replace(dest)
info = {k:payload[k] for k in ["format", "origin", "config", "step", "metadata", "export"]}
info["sha256"] = hashlib.sha256(dest.read_bytes()).hexdigest()
dest.with_name("model-info.json").write_text(json.dumps(info, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
print(f"Exported {dest.name}: {dest.stat().st_size} bytes; step {payload['step']}")
