"""Extend the tiny starter with ORIGINAL project-authored notes, not scraped text.

Related starter topics and notes share split groups. The corpus is still tiny,
synthetic and unreviewed by a human; no broad language competence is implied.
"""
import json
from pathlib import Path

root = Path(__file__).resolve().parent
rows = [json.loads(line) for line in (root / "data/starter.jsonl").read_text(encoding="utf-8").splitlines()]
groups = ["groups", "move", "rotate", "spawn", "colour", "camera", "keyframes", "collision", "optimisation",
          "mechanics", "difficulty", "history", "official", "practice", "icons", "coins", "collections", "paths", "daily", "community", "geode", "music"]
aliases = {"alpha":"colour", "pulse":"colour", "shader":"optimisation", "decoration":"optimisation",
           "cube":"mechanics", "ship":"mechanics", "ball":"mechanics", "ufo":"mechanics", "wave":"mechanics",
           "robot":"mechanics", "spider":"mechanics", "swing":"mechanics", "platformer":"mechanics",
           "rank":"community", "creator":"community", "player":"community", "demonlist":"community",
           "song":"music", "licence":"music", "songid":"music", "newgrounds":"music"}
for row in rows:
    row["group"] = aliases.get(row["group"], row["group"])
notes = json.loads((root.parent / "resources/knowledge.json").read_text(encoding="utf-8"))
if len(notes) != len(groups):
    raise ValueError("Update topic grouping when reference documents change")
for group, note in zip(groups, notes):
    for language, facts in [("en", note["facts"]), ("pl", note["facts_pl"])]:
        title = note["title"]
        q = f"Explain {title}." if language == "en" else f"Wyjaśnij temat: {title}."
        followup = "What else should I check?" if language == "en" else "Co jeszcze warto sprawdzić?"
        examples = [[{"role":"user", "content":q}, {"role":"assistant", "content":" ".join(facts)}]]
        for i, fact in enumerate(facts):
            examples.append([{"role":"user", "content":q}, {"role":"assistant", "content":fact}])
            if i + 1 < len(facts):
                examples.append([{"role":"user", "content":q}, {"role":"assistant", "content":fact},
                                 {"role":"user", "content":followup}, {"role":"assistant", "content":facts[i+1]}])
        for messages in examples:
            rows.append(dict(group=group, language=language, source="project-authored-reference-paraphrases-2026-10-21",
                             license="project training permission; original AI-authored notes, not source documents",
                             references=[note["url"]], messages=messages))
out = root / "data/domain.jsonl"
out.write_text("".join(json.dumps(r, ensure_ascii=False)+"\n" for r in rows), encoding="utf-8")
print(f"{len(rows)} synthetic records, {len({r['group'] for r in rows})} groups, {out.stat().st_size} bytes")
