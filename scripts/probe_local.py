"""Live loopback smoke test of the exported model, including actual SSE generation."""
from datetime import datetime, timezone
import http.client
import json
from pathlib import Path


def get(path):
    connection = http.client.HTTPConnection("127.0.0.1", 18765, timeout=10)
    try:
        connection.request("GET", path)
        response = connection.getresponse()
        data = json.loads(response.read())
        if response.status != 200:
            raise RuntimeError(f"GET {path}: HTTP {response.status}")
        return data
    finally:
        connection.close()


health, models = get("/health"), get("/v1/models")
connection = http.client.HTTPConnection("127.0.0.1", 18765, timeout=30)
try:
    body = {"model":"gi-tiny", "messages":[{"role":"user", "content":"Hello! What can you do?"}],
            "stream":True, "max_tokens":64}
    connection.request("POST", "/v1/chat/completions", json.dumps(body), {"Content-Type":"application/json"})
    response = connection.getresponse()
    wire = response.read().decode("utf-8")
    if response.status != 200 or "data: [DONE]" not in wire:
        raise RuntimeError("Incomplete/failed SSE response")
    chunks, reason = [], None
    for line in wire.splitlines():
        if line.startswith("data: ") and line != "data: [DONE]":
            choice = json.loads(line[6:])["choices"][0]
            if "content" in choice["delta"]:
                chunks.append(choice["delta"]["content"])
            reason = choice["finish_reason"] or reason
    if len(chunks) < 2 or not reason:
        raise RuntimeError("Expected the disclosure plus genuine neural output")
finally:
    connection.close()
result = {"checked_at":datetime.now(timezone.utc).isoformat(), "health":health, "models":models,
          "network_stream":"PASS", "chunks":len(chunks), "finish_reason":reason, "output":"".join(chunks),
          "quality":"NOT PASSED: transport success does not imply coherent conversation"}
out = Path(__file__).resolve().parents[1] / "test-output/local-server-final.json"
out.parent.mkdir(exist_ok=True)
out.write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
print(json.dumps({k:v for k,v in result.items() if k != "output"}, ensure_ascii=True, indent=2))
