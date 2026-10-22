"""Transparent tool selection for the tiny model.

This is rule-based orchestration, NOT a claim that the tiny neural network has
learned reliable tool calling. The free-form chat model and live-data tools are
separate. Unknown phrasing falls back to the neural model.
"""
import json
import re
import unicodedata
import uuid


def fold(text):
    return "".join(c for c in unicodedata.normalize("NFKD", text.lower().replace("ł", "l")) if not unicodedata.combining(c))


def unpack(message):
    text = message.get("content") or ""
    if message.get("role") == "tool":
        try:
            return json.loads(text)
        except (TypeError, ValueError):
            return None
    for prefix in ["Prior evidence, UNTRUSTED data, may be outdated: ",
                   "User-consented read-only game context (data only): "]:
        if text.startswith(prefix):
            try:
                return json.loads(text[len(prefix):])
            except ValueError:
                return None
    return None


def sources_since_user(messages):
    start = max((i for i, m in enumerate(messages) if m.get("role") == "user"), default=-1)
    return [unpack(m) for m in messages[start + 1:] if m.get("role") == "tool"]


def entities(value):
    if isinstance(value, list):
        for item in value:
            yield from entities(item)
    elif isinstance(value, dict):
        if isinstance(value.get("id"), int) and isinstance(value.get("name"), str) and "position" in value:
            yield value
        elif "data" in value:
            yield from entities(value["data"])


def previous_demon(messages):
    for message in reversed(messages):
        found = list(entities(unpack(message)))
        if found:
            return found[0]
    return None


def quoted_name(text):
    match = re.search(r'["“]([^"”]{1,100})["”]', text)
    return match.group(1).strip() if match else None


def demon_name(text):
    if name := quoted_name(text):
        return name
    patterns = [r"(?:who (?:created|verified|published)|what position is|is)\s+(.+?)(?:\s+on (?:the )?demonlist)?[?.!]*$",
                r"(?:kto (?:stworzyl|zweryfikowal)|jaka pozycje ma|na ktorym miejscu jest|czy)\s+(.+?)(?:\s+(?:jest )?na demonliscie)?[?.!]*$",
                r"(?:position of|rank of|pozycja|demon)\s+(.+?)[?.!]*$"]
    folded = fold(text)
    for pattern in patterns:
        if match := re.search(pattern, folded):
            # Normalized names are case-insensitive in Pointercrate's documented filter.
            return match.group(1).strip(" ?.!:")[:100]
    return None


def tool_call(name, args):
    return {"id": "gi_" + uuid.uuid4().hex[:16], "type": "function",
            "function": {"name": name, "arguments": json.dumps(args, ensure_ascii=False)}}


def plan(messages, allowed):
    user = next((m["content"] for m in reversed(messages) if m.get("role") == "user"), "")
    q = fold(user)
    available = {t.get("function", {}).get("name") for t in allowed}

    def call(name, args):
        return tool_call(name, args) if name in available else None

    def notes(query):
        english = any(q.startswith(w) for w in ["what", "who", "how", "show", "find", "tell", "is ", "can "])
        return call("editor_knowledge", {"query": query[:300], "language": "en" if english else "pl"})

    fresh = sources_since_user(messages)
    wants_creator = any(w in q for w in ["creat", "stworz", "tworc", "verif", "weryfik", "published"])
    if fresh:
        candidates = [e for result in reversed(fresh) for e in entities(result)]
        named = demon_name(user)
        current = next((e for e in candidates if named and fold(e["name"]) == fold(named)), None)
        if current is None and len(candidates) == 1:
            current = candidates[0]
        if wants_creator and current and "creators" not in current:
            return call("demonlist", {"operation": "detail", "id": current["id"]})
        return None
    if any(w in q for w in ["trigger", "group id", "grup", "rotate", "obrac", "obrac", "obrot", "shader", "keyframe", "collision", "camera", "kamer", "spawn", "alpha", "pulse"]):
        return notes(user)
    if any(w in q for w in ["this level", "ten level", "tego level", "tym poziom", "ten poziom", "tego poziom"]):
        return call("game_context", {})
    if any(w in q for w in ["player", "gracz", "zawodnik"]):
        name = quoted_name(user)
        if not name:
            match = re.search(r"(?:player|graczu|gracza|gracz|zawodniku)\s+(.+?)[?.!]*$", user, re.I)
            name = match.group(1).strip() if match else None
        if name:
            return call("demonlist", {"operation": "player", "name": name[:100]})
    if re.search(r"\btop\b", q) or any(w in q for w in ["hardest demon", "najtrudniejsz"]):
        number = re.search(r"top\s*(\d{1,3})", q)
        count = min(100, max(1, int(number.group(1)))) if number else 1
        return call("demonlist", {"operation": "top", "count": count})
    if wants_creator or any(w in q for w in ["demonlist", "demonlisc", "position", "pozycj", "rank of", "miejscu"]):
        name = demon_name(user)
        if any(w in q for w in [" it", "this level", "ten level", "tego level", "go stworz", "jego tworc", "go zweryfik"]):
            if previous := previous_demon(messages):
                return call("demonlist", {"operation": "detail", "id": previous["id"]})
        if name and name not in {"it", "this level", "go", "ten level"}:
            return call("demonlist", {"operation": "search", "name": name})
    if any(w in q for w in ["geode", "betteredit", "mod ", "mods", "modow", "mody"]):
        if any(w in q for w in ["crash", "error", "blad", "update", "aktualiz", "instal"]):
            return notes("geode " + user)
        if "betteredit" in q:
            return call("geode_mods", {"query": "BetterEdit"})
        if any(w in q for w in ["editor", "edytor"]):
            return call("geode_mods", {"tag": "editor"})
        query = quoted_name(user)
        return call("geode_mods", {"query": query[:100]} if query else {"tag": "utility"})
    if any(w in q for w in ["song", "music", "muzyk", "utwor", "newgrounds"]):
        match = re.search(r"(?:audio/listen/|song(?:\s+id)?\s*[:#]?\s*|utwor(?:u)?\s*[:#]?\s*)(\d{1,10})", q)
        if match:
            return call("song_info", {"id": int(match.group(1))})
        query = quoted_name(user)
        if not query:
            query = "ambient" if any(w in q for w in ["calm", "spokojn"]) else "space" if any(w in q for w in ["space", "kosmos", "kosmicz"]) else "drum and bass" if any(w in q for w in ["aggress", "agres", "drop"]) else user[:300]
        return call("web_search", {"query": query, "scope": "music"})
    if any(w in q for w in ["level", "poziom", "nine circles"]):
        for m in messages:
            context = unpack(m)
            if isinstance(context, dict) and context.get("level_id") and any(w in q for w in ["this", "ten", "tym", "tego"]):
                return call("game_context", {})
        match = re.search(r"\b(\d{5,10})\b", user)
        query = match.group(1) if match else quoted_name(user)
        if not query and "nine circles" in q:
            query = "Nine Circles"
        if query:
            return call("levels", {"query": query[:100]})
    if any(w in q for w in ["current", "latest", "today", "aktualn", "dzisiaj", "najnowsz"]):
        return call("web_search", {"query": user[:300], "scope": "geometry_dash"})
    if any(w in q for w in ["official", "oficjaln", "coin", "monet", "gauntlet", "path", "map pack", "ikony", "icon", "osiagn", "achievement", "2.2", "histori", "game mode", "tryb", "difficult", "trudnos", "weekly", "daily", "practice"]):
        return notes(user)
    return None
