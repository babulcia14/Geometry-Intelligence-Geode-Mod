"""Byte-pair tokenizer learned only from our training partition, without downloads."""
from collections import Counter

BASE = 262  # Byte IDs and reserved role/control tokens, matching model.py.


class Tokenizer:
    def __init__(self, merges=()):
        self.merges = [tuple(p) for p in merges]
        self.pieces = {n: bytes([n]) for n in range(256)}
        for i, (left, right) in enumerate(self.merges):
            if left not in self.pieces or right not in self.pieces:
                raise ValueError("Invalid tokenizer merge dependency")
            self.pieces[BASE+i] = self.pieces[left] + self.pieces[right]

    @property
    def vocab(self):
        return BASE + len(self.merges)

    @staticmethod
    def merge(sequence, pair, token):
        out, i = [], 0
        while i < len(sequence):
            if i+1 < len(sequence) and sequence[i] == pair[0] and sequence[i+1] == pair[1]:
                out.append(token); i += 2
            else:
                out.append(sequence[i]); i += 1
        return out

    def encode(self, text):
        ids = list(text.encode("utf-8"))
        for i, pair in enumerate(self.merges):
            ids = self.merge(ids, pair, BASE+i)
        return ids

    def state(self):
        return {"type":"byte-bpe", "merges":[list(p) for p in self.merges]}

    @classmethod
    def from_state(cls, data):
        if data is None:
            return cls()
        if not isinstance(data, dict) or data.get("type") != "byte-bpe" or not isinstance(data.get("merges"), list) or len(data["merges"]) > 4096:
            raise ValueError("Invalid tokenizer state")
        return cls(data["merges"])

    @classmethod
    def train(cls, texts, count):
        if not 0 <= count <= 4096:
            raise ValueError("Use 0-4096 BPE merges")
        # Deduplicate texts so repeated templates do not dominate vocabulary.
        sequences = [list(t.encode("utf-8")) for t in sorted(set(texts))]
        merges = []
        for i in range(count):
            pairs = Counter((a,b) for seq in sequences for a,b in zip(seq, seq[1:]))
            if not pairs:
                break
            pair, occurrences = pairs.most_common(1)[0]
            if occurrences < 2:
                break
            merges.append(pair)
            sequences = [cls.merge(seq, pair, BASE+i) for seq in sequences]
        return cls(merges)
