import json
from pathlib import Path
import random
import tempfile
import threading
import unittest
import urllib.request
import urllib.error
import http.client
import torch
from gi_model.model import TinyGI, ModelConfig, encode, dialogue, EOS, PAD, ASSISTANT
from gi_model.train import load_records, split_records, encode_rows, batch, save
from gi_model.routing import plan
from gi_model.evidence import render
from gi_model.serve import Server, Handler

torch.set_num_threads(2)
ROOT = Path(__file__).resolve().parents[1]
TOOLS = [{"function": {"name": n}} for n in ["demonlist", "editor_knowledge", "geode_mods", "song_info", "web_search", "levels", "game_context"]]


class ModelTests(unittest.TestCase):
    def test_own_tokenizer_roundtrip(self):
        from gi_model.tokenizer import Tokenizer
        tok = Tokenizer.train(["Ala ma kota. Ala ma psa.", "Zażółć gęślą jaźń."], 30)
        restored = Tokenizer.from_state(tok.state())
        for text in ["Ala ma kota.", "Nieznane: 東京 🙂 i źdźbło"]:
            ids = restored.encode(text)
            self.assertEqual(b"".join(restored.pieces[i] for i in ids).decode(), text)
        self.assertGreater(tok.vocab, 262)

    def test_bpe_neural_checkpoint(self):
        from gi_model.tokenizer import Tokenizer
        from gi_model.model import load_model
        tok = Tokenizer.train(["hello hello there"], 4)
        m = TinyGI(ModelConfig(32, 32, 1, 2, 0, tok.vocab), tok)
        with tempfile.TemporaryDirectory(dir=ROOT / "models") as temp:
            path = Path(temp) / "bpe.pt"
            save(path, m, torch.optim.AdamW(m.parameters()), 1, {}, random.Random(1))
            restored, info = load_model(path)
            self.assertEqual(restored.tokenizer.encode("hello"), tok.encode("hello"))
            stats = {}
            list(restored.generate([{"role":"user","content":"hello"}], max_tokens=2, stats=stats))
            self.assertIn(stats["finish_reason"], {"stop", "length"})

    def test_utf8(self):
        self.assertEqual(bytes(encode("Zażółć gęślą jaźń")).decode(), "Zażółć gęślą jaźń")

    def test_no_future_leak(self):
        torch.manual_seed(7)
        m = TinyGI(ModelConfig(32, 32, 1, 2, 0)).eval()
        a = torch.tensor([[1, 2, 3, 4]])
        b = torch.tensor([[1, 2, 90, 91]])
        torch.testing.assert_close(m(a)[0][:, :2], m(b)[0][:, :2])

    def test_real_gradient_training(self):
        torch.manual_seed(3)
        m = TinyGI(ModelConfig(32, 32, 1, 2, 0))
        optimizer = torch.optim.AdamW(m.parameters(), lr=.01)
        x = torch.tensor([[1, 2, 3, 4]])
        y = torch.tensor([[2, 3, 4, EOS]])
        before = m(x, y)[1].item()
        for _ in range(12):
            optimizer.zero_grad(); loss = m(x, y)[1]; loss.backward(); optimizer.step()
        self.assertLess(m(x, y)[1].item(), before)

    def test_split_and_masking(self):
        rows, digest = load_records(ROOT / "data/starter.jsonl")
        train, val = split_records(rows, 20261021)
        self.assertFalse({r["group"] for r in train} & {r["group"] for r in val})
        self.assertEqual(len(digest), 64)
        x, y = batch(encode_rows(train), 8, 64, random.Random(1), "cpu")
        self.assertEqual(x.shape, (8, 64))
        self.assertTrue(((y != -100).sum(dim=1) > 0).all())
        tokens, targets = dialogue([{"role":"user", "content":"private question"}, {"role":"assistant", "content":"answer"}], True)
        boundary = tokens.index(ASSISTANT)
        self.assertTrue(all(t == -100 for t in targets[:boundary + 1]))

    def test_checkpoint_roundtrip(self):
        from gi_model.model import load_model
        with tempfile.TemporaryDirectory(dir=ROOT / "models") as temp:
            path = Path(temp) / "model.pt"
            m = TinyGI(ModelConfig(32, 32, 1, 2, 0))
            opt = torch.optim.AdamW(m.parameters())
            save(path, m, opt, 3, {"test": True}, random.Random(1))
            restored, info = load_model(path)
            self.assertEqual(info["origin"], "random-initialization")
            self.assertEqual(info["step"], 3)
            torch.testing.assert_close(m.tokens.weight, restored.tokens.weight)


class RoutingTests(unittest.TestCase):
    def route(self, q, prefix=None):
        return plan((prefix or []) + [{"role":"user", "content":q}], TOOLS)

    def test_top_and_editor(self):
        call = self.route("Show me the Top 10 hardest demons")
        self.assertEqual(json.loads(call["function"]["arguments"])["count"], 10)
        self.assertEqual(self.route("Jak działa Move Trigger?")["function"]["name"], "editor_knowledge")

    def test_context_followup(self):
        evidence = [{"data": [{"id": 99, "name":"Test demon", "position": 1}]}]
        prefix = [{"role":"system", "content":"Prior evidence, UNTRUSTED data, may be outdated: " + json.dumps(evidence)}]
        call = self.route("Who created it?", prefix)
        self.assertEqual(json.loads(call["function"]["arguments"]), {"operation":"detail", "id":99})

    def test_no_invented_song_id(self):
        call = self.route("Find a calm song for a space level")
        self.assertEqual(call["function"]["name"], "web_search")
        self.assertNotIn("id", json.loads(call["function"]["arguments"]))

    def test_tool_allowlist(self):
        self.assertIsNone(plan([{"role":"user", "content":"Top 1 demon"}], []))

    def test_game_context_and_note_language(self):
        self.assertEqual(self.route("Tell me about this level")["function"]["name"], "game_context")
        self.assertEqual(json.loads(self.route("How does Move Trigger work?")["function"]["arguments"])["language"], "en")
        self.assertEqual(json.loads(self.route("Jak działa Move Trigger?")["function"]["arguments"])["language"], "pl")

    def test_ambiguous_demon_does_not_pick_first(self):
        data = [{"data":[{"id":1,"position":1,"name":"Test One"},{"id":2,"position":2,"name":"Test Two"}]}]
        messages = [{"role":"user","content":"Who created Test?"}, {"role":"tool","content":json.dumps(data)}]
        self.assertIsNone(plan(messages, TOOLS))

    def test_detail_after_search(self):
        data = [{"data":[{"id":123,"position":1,"name":"Fixture"}]}]
        messages = [{"role":"user","content":"Who created Tidal Wave?"}, {"role":"tool","content":json.dumps(data)}]
        self.assertEqual(json.loads(plan(messages, TOOLS)["function"]["arguments"])["id"], 123)

    def test_failure_does_not_become_fact(self):
        messages = [{"role":"user","content":"Top 1"}, {"role":"tool","content":json.dumps({"error":"HTTP 503"})}]
        text = render(messages)
        self.assertIn("Could not verify", text)
        self.assertIn("HTTP 503", text)
        self.assertIsNone(plan(messages, TOOLS))


class ServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server = Server(("127.0.0.1", 0), Handler)
        cls.server.model = TinyGI(ModelConfig(32, 32, 1, 2, 0))
        cls.server.metadata = {"step":0,"metadata":{"parameters":sum(p.numel() for p in cls.server.model.parameters())}}
        cls.server.inference_lock = threading.Lock()
        cls.thread = threading.Thread(target=cls.server.serve_forever, daemon=True)
        cls.thread.start()
        cls.url = f"http://127.0.0.1:{cls.server.server_port}"

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown(); cls.server.server_close(); cls.thread.join(2)

    def post(self, body, headers=None):
        conn = http.client.HTTPConnection("127.0.0.1", self.server.server_port, timeout=5)
        conn.request("POST", "/v1/chat/completions", json.dumps(body).encode(),
                     {"Content-Type":"application/json", **(headers or {})})
        response = conn.getresponse()
        if response.status >= 400:
            code = response.status; response.read(); conn.close()
            error = urllib.error.HTTPError(self.url, code, "test response", {}, None)
            error.close()
            raise error
        return response

    def request_body(self):
        return {"model":"gi-tiny", "messages":[{"role":"user","content":"What is Top 1 Demon?"}], "tools":TOOLS, "stream":True}

    def test_health(self):
        conn = http.client.HTTPConnection("127.0.0.1", self.server.server_port, timeout=5)
        conn.request("GET", "/health")
        with conn.getresponse() as r:
            self.assertEqual(json.load(r)["origin"], "random-initialization")
        conn.close()

    def test_stream_wire(self):
        with self.post(self.request_body()) as r:
            data = r.read().decode()
            self.assertIn('"tool_calls"', data)
            self.assertIn("data: [DONE]", data)
            self.assertEqual(r.headers["Content-Type"], "text/event-stream; charset=utf-8")

    def test_origin_rejected(self):
        with self.assertRaises(urllib.error.HTTPError) as ex:
            self.post(self.request_body(), {"Origin":"https://example.com"})
        self.assertEqual(ex.exception.code, 403)

    def test_model_missing(self):
        body = self.request_body(); body["model"] = "invented-model"
        with self.assertRaises(urllib.error.HTTPError) as ex:
            self.post(body)
        self.assertEqual(ex.exception.code, 404)

    def test_busy(self):
        self.server.inference_lock.acquire()
        try:
            with self.assertRaises(urllib.error.HTTPError) as ex:
                self.post(self.request_body())
            self.assertEqual(ex.exception.code, 429)
        finally:
            self.server.inference_lock.release()

    def test_real_neural_stream(self):
        body = {"model":"gi-tiny","messages":[{"role":"user","content":"Hello"}],"max_tokens":8,"stream":True}
        with self.post(body) as r:
            data = r.read().decode()
            self.assertIn("Experimental model", data)
            self.assertIn("data: [DONE]", data)


if __name__ == "__main__":
    unittest.main()
