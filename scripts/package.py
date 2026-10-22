"""Create a reviewable local release using an explicit file allowlist; never publish."""
import hashlib
import json
from pathlib import Path
import shutil
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
DIST.mkdir(exist_ok=True)
manifest = json.loads((ROOT / "mod.json").read_text(encoding="utf-8"))
mod = ROOT / "build" / (manifest["id"] + ".geode")
model = ROOT / "python/models/gi-tiny/model.pt"
info = ROOT / "python/models/gi-tiny/model-info.json"
if not mod.is_file() or not model.is_file() or not info.is_file():
    raise SystemExit("Build the mod and export inference weights before packaging.")
model_info = json.loads(info.read_text(encoding="utf-8"))
if hashlib.sha256(model.read_bytes()).hexdigest() != model_info["sha256"]:
    raise SystemExit("Inference model hash differs from model-info.json.")
with zipfile.ZipFile(mod) as package:
    if package.testzip() is not None:
        raise SystemExit("Corrupt .geode archive")
    packed = json.loads(package.read("mod.json"))
    if packed["id"] != manifest["id"] or packed["gd"] != manifest["gd"] or packed["version"] != manifest["version"] or packed["name"] != manifest["name"]:
        raise SystemExit("Stale .geode manifest")
    for resource in ["knowledge.json", "gi-font.fnt", "gi-font.png", "OFL.txt", "rub-logo.png"]:
        if not any(Path(name).name == resource for name in package.namelist()):
            raise SystemExit(f"Missing resource in .geode: {resource}")
    dlls = [n for n in package.namelist() if n.lower().endswith(".dll")]
    if len(dlls) != 1:
        raise SystemExit("Expected one Windows mod DLL")
    binary = package.read(dlls[0])
    pe = struct.unpack_from("<I", binary, 0x3C)[0]
    if binary[:2] != b"MZ" or binary[pe:pe+4] != b"PE\0\0" or struct.unpack_from("<H", binary, pe+4)[0] != 0x8664:
        raise SystemExit("Expected an x64 PE binary")

mod_out = DIST / mod.name
shutil.copy2(mod, mod_out)
allowed_extensions = {".hpp", ".cpp", ".py", ".ps1", ".json", ".jsonl", ".md", ".txt", ".fnt", ".png", ".mit"}
files = {}
for folder in ["include", "src", "resources", "python", "tests", "scripts", "docs", "third_party"]:
    for p in (ROOT / folder).rglob("*"):
        if not p.is_file() or any(part in {"__pycache__", "models", ".pytest_cache"} for part in p.relative_to(ROOT).parts):
            continue
        if p.suffix.lower() in allowed_extensions:
            files[p.relative_to(ROOT).as_posix()] = p
for name in ["CMakeLists.txt", "logo.png", "mod.json", "about.md", "changelog.md", "README.md", "LICENSE", "NOTICE.md", ".gitignore", "Start_AI.cmd", "Setup_AI.cmd", "Check_AI.cmd", "Train_AI.cmd"]:
    files[name] = ROOT / name
for p in [model, info, mod_out]:
    files[p.relative_to(ROOT).as_posix()] = p
build_info = DIST / "BUILD_INFO.json"
build_info.write_text(json.dumps({"mod_id":manifest["id"], "version":manifest["version"],
    "geode_sdk":manifest["geode"], "gd_target":manifest["gd"], "architecture":"x64",
    "mod_sha256":hashlib.sha256(mod_out.read_bytes()).hexdigest(), "model_sha256":model_info["sha256"],
    "model_origin":model_info["origin"], "model_step":model_info["step"],
    "mod_crc_and_resources":"PASS", "game_runtime":"NOT TESTED", "neural_conversation_quality":"FAIL"}, indent=2)+"\n", encoding="utf-8")
files["dist/BUILD_INFO.json"] = build_info
run = model_info["export"]["source_run"]
if run not in {"gi-tiny", "gi-domain", "gi-bpe"}:
    raise SystemExit("Add the reviewed training run to the package allowlist first.")
for experiment in {run, "gi-tiny", "gi-domain", "gi-bpe"}:
    for name in ["training-report.json", "metrics.jsonl"]:
        p = ROOT / "python/models" / experiment / name
        if p.is_file():
            files[p.relative_to(ROOT).as_posix()] = p
# Only explicitly reviewed public/fixture test results, never histories/caches.
for name in ["core-tests.txt", "python-tests.txt", "live-api-final.jsonl", "live-chat-final.jsonl",
             "neural-samples.json", "neural-samples-latest.json", "neural-domain-best.json", "neural-domain-latest.json",
             "neural-bpe-best.json", "neural-bpe-latest.json", "local-server-final.json"]:
    p = ROOT / "test-output" / name
    if p.is_file():
        files[p.relative_to(ROOT).as_posix()] = p

archive = DIST / "Geometry-Intelligence-Rub-1.0.0-alpha.2.zip"
file_list = DIST / "FILES.txt"
file_list.write_text("\n".join(sorted(files)) + "\ndist/FILES.txt\n", encoding="utf-8")
files["dist/FILES.txt"] = file_list
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for name, path in sorted(files.items()):
        z.write(path, name)
with zipfile.ZipFile(archive) as z:
    if z.testzip() is not None:
        raise SystemExit("Release ZIP integrity check failed")
hashes = "\n".join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}" for p in [mod_out, archive]) + "\n"
(DIST / "SHA256SUMS.txt").write_text(hashes, encoding="ascii")
print(json.dumps({"mod": mod_out.name, "mod_bytes": mod_out.stat().st_size,
                  "archive": archive.name, "archive_bytes": archive.stat().st_size,
                  "files": len(files), "model_step": model_info["step"],
                  "target": manifest["gd"], "zip_integrity":"PASS", "pe_architecture":"x64"}, indent=2))
