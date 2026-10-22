"""Record real free-form model output; no tool routing, corpus lookup or scoring fiction."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import time
import torch
from gi_model.model import load_model

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--checkpoint", default="models/gi-tiny/best.pt")
p.add_argument("--out", default="../test-output/neural-samples.json")
args = p.parse_args()
torch.set_num_threads(4)
model, info = load_model(args.checkpoint)
samples = []
for q in ["Hello! What can you do?", "Cześć, kim jesteś?", "Explain what a group is in a level editor.",
          "Napisz dwa zdania o tworzeniu poziomów.", "I am frustrated because this level is hard. Can you help?",
          "What is the current Top 1 Demon?"]:
    stats = {}
    start = time.perf_counter()
    answer = "".join(model.generate([dict(role="user", content=q)], max_tokens=300, temperature=.65, seed=42, stats=stats))
    samples.append(dict(question=q, answer=answer, seconds=round(time.perf_counter()-start,3), **stats))
result = dict(timestamp=datetime.now(timezone.utc).isoformat(), origin=info["origin"], step=info["step"],
              parameters=sum(p.numel() for p in model.parameters()), samples=samples,
              evaluation="Raw observations, no automatic claim of correctness. Ranking question deliberately bypasses tools to expose model limits.")
dest = Path(args.out); dest.parent.mkdir(parents=True, exist_ok=True)
dest.write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
print(json.dumps(result, ensure_ascii=True, indent=2))
