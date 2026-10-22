"""A small causal Transformer. Every trainable tensor starts randomly.

PyTorch supplies tensor operations and automatic differentiation, NOT a model
or knowledge. Tokenization is UTF-8 bytes or BPE learned from our own data;
no downloaded vocabulary is used.
"""
from dataclasses import asdict, dataclass
import codecs
import math
import torch
from torch import nn
from torch.nn import functional as F
from .tokenizer import Tokenizer

PAD, BOS, EOS, USER, ASSISTANT, SYSTEM = range(256, 262)
VOCAB = 262


@dataclass
class ModelConfig:
    context: int = 256
    width: int = 96
    layers: int = 3
    heads: int = 4
    dropout: float = 0.1
    vocab: int = VOCAB

    def validate(self):
        if not (32 <= self.context <= 4096 and 32 <= self.width <= 2048):
            raise ValueError("Invalid context/width")
        if not (1 <= self.layers <= 32 and 1 <= self.heads <= 32 and self.width % self.heads == 0):
            raise ValueError("Invalid layer/head count")
        if not 0 <= self.dropout < 1:
            raise ValueError("Invalid dropout")
        if not VOCAB <= self.vocab <= 4358:
            raise ValueError("Invalid vocabulary")


def encode(text):
    return list(text.encode("utf-8"))


def dialogue(messages, training=False, tokenizer=None):
    tokens, labels = [BOS], [-100]
    for message in messages:
        role = message["role"]
        if role not in {"user", "assistant"}:
            continue
        body = (tokenizer.encode(message["content"]) if tokenizer else encode(message["content"])) + [EOS]
        tokens += [USER if role == "user" else ASSISTANT] + body
        labels += [-100] + (body if role == "assistant" else [-100] * len(body))
    if not training:
        tokens += [ASSISTANT]
    return tokens, labels


class Attention(nn.Module):
    def __init__(self, cfg):
        super().__init__()
        self.heads, self.dropout = cfg.heads, cfg.dropout
        self.qkv = nn.Linear(cfg.width, 3 * cfg.width, bias=False)
        self.output = nn.Linear(cfg.width, cfg.width, bias=False)

    def forward(self, x):
        batch, length, width = x.shape
        q, k, v = self.qkv(x).chunk(3, dim=-1)
        q, k, v = [t.view(batch, length, self.heads, width // self.heads).transpose(1, 2) for t in (q, k, v)]
        y = F.scaled_dot_product_attention(q, k, v, is_causal=True,
                                         dropout_p=self.dropout if self.training else 0.0)
        return self.output(y.transpose(1, 2).contiguous().view(batch, length, width))


class Block(nn.Module):
    def __init__(self, cfg):
        super().__init__()
        self.norm1, self.norm2 = nn.LayerNorm(cfg.width), nn.LayerNorm(cfg.width)
        self.attention = Attention(cfg)
        self.mlp = nn.Sequential(nn.Linear(cfg.width, 4 * cfg.width), nn.GELU(),
                                 nn.Linear(4 * cfg.width, cfg.width), nn.Dropout(cfg.dropout))

    def forward(self, x):
        x = x + self.attention(self.norm1(x))
        return x + self.mlp(self.norm2(x))


class TinyGI(nn.Module):
    def __init__(self, cfg, tokenizer=None):
        super().__init__()
        cfg.validate()
        self.cfg = cfg
        self.tokenizer = tokenizer or Tokenizer()
        if self.tokenizer.vocab != cfg.vocab:
            raise ValueError("Tokenizer/model vocabulary mismatch")
        self.tokens = nn.Embedding(cfg.vocab, cfg.width)
        self.positions = nn.Embedding(cfg.context, cfg.width)
        self.blocks = nn.Sequential(*[Block(cfg) for _ in range(cfg.layers)])
        self.norm = nn.LayerNorm(cfg.width)
        self.output = nn.Linear(cfg.width, cfg.vocab, bias=False)
        self.apply(self._init)
        self.output.weight = self.tokens.weight

    @staticmethod
    def _init(module):
        if isinstance(module, (nn.Linear, nn.Embedding)):
            nn.init.normal_(module.weight, mean=0.0, std=0.02)
        if isinstance(module, nn.Linear) and module.bias is not None:
            nn.init.zeros_(module.bias)

    def forward(self, ids, labels=None):
        if ids.shape[1] > self.cfg.context:
            raise ValueError("Context too long")
        x = self.tokens(ids) + self.positions(torch.arange(ids.shape[1], device=ids.device))
        logits = self.output(self.norm(self.blocks(x)))
        loss = None if labels is None else F.cross_entropy(logits.reshape(-1, self.cfg.vocab), labels.reshape(-1), ignore_index=-100)
        return logits, loss

    @torch.inference_mode()
    def generate(self, messages, max_tokens=400, temperature=0.7, seed=42, stats=None):
        self.eval()
        ids, _ = dialogue(messages, tokenizer=self.tokenizer)
        device = next(self.parameters()).device
        generator = torch.Generator(device=device).manual_seed(seed)
        decoder = codecs.getincrementaldecoder("utf-8")("replace")
        ended, generated = False, 0
        for _ in range(min(max_tokens, 2048)):
            x = torch.tensor([ids[-self.cfg.context:]], dtype=torch.long, device=device)
            logits = self(x)[0][0, -1].clone()
            end_score = logits[EOS].clone()
            logits[PAD:SYSTEM+1] = -math.inf
            logits[EOS] = end_score  # EOS is the only allowed special output.
            if temperature <= 0:
                token = int(logits.argmax())
            else:
                logits /= max(0.1, temperature)
                cutoff = torch.topk(logits, min(40, self.cfg.vocab)).values[-1]
                logits[logits < cutoff] = -math.inf
                token = int(torch.multinomial(logits.softmax(-1), 1, generator=generator))
            if token == EOS:
                ended = True
                break
            generated += 1
            ids.append(token)
            text = decoder.decode(self.tokenizer.pieces[token], final=False)
            if text:
                yield text
        final = decoder.decode(b"", final=True)
        if final:
            yield final
        if stats is not None:
            stats.update(finish_reason="stop" if ended else "length", generated_tokens=generated)


def load_model(path, device="cpu"):
    # Restricted tensor/dict deserialization. Never load arbitrary pickle code.
    saved = torch.load(path, map_location=device, weights_only=True)
    if saved.get("format") != 1 or saved.get("origin") != "random-initialization":
        raise ValueError("Not a Geometry Intelligence from-scratch checkpoint")
    model = TinyGI(ModelConfig(**saved["config"]), Tokenizer.from_state(saved.get("tokenizer")))
    model.load_state_dict(saved["weights"], strict=True)
    return model.to(device).eval(), saved
