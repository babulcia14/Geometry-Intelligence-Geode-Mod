"""Lossless fact rendering for a model too small to reliably copy identifiers.

Clearly marked tool output; never represented as neural language generation.
There are no stored rankings or hardcoded song IDs here.
"""
import json
from .routing import sources_since_user


def render(messages, polish=False):
    results = sources_since_user(messages)
    if not results:
        return None
    lines = ["Dane ze źródeł (odczyt narzędzi):" if polish else "Source data (tool readout):"]
    for result in results:
        if isinstance(result, dict) and "error" in result:
            lines.append(("Nie udało się sprawdzić: " if polish else "Could not verify: ") + str(result["error"]))
            continue
        if not isinstance(result, list):
            continue
        if not result:
            lines.append("Brak pasujących notatek." if polish else "No matching reference notes.")
        for source in result:
            if not isinstance(source, dict):
                continue
            lines.append(f"\n[{source.get('source_index', '?')}] {source.get('title', '')}")
            data = source.get("data")
            if isinstance(data, dict) and "facts" in data:
                lines.extend(f"{i}. {fact}" for i, fact in enumerate(data["facts"], 1))
            elif isinstance(data, list):
                for row in data[:20]:
                    lines.extend(describe(row, polish))
                if not data:
                    lines.append("Brak wyników. To nie dowodzi nieistnienia." if polish else "No results. This does not prove non-existence.")
            elif isinstance(data, dict) and "mods" in data:
                for mod in data["mods"]:
                    for v in mod.get("versions", [])[:1]:
                        authors = ", ".join(str(a.get("display_name") or a.get("username", "?")) for a in mod.get("developers", []))
                        lines += [f"{v.get('name', mod.get('id'))} — {v.get('version', '?')} — {authors}",
                                  str(v.get("description", "")), str(mod.get("url", "")),
                                  f"GD: {json.dumps(v.get('gd'), ensure_ascii=False)}; Geode: {v.get('geode', '?')}"]
                lines.append(str(data.get("compatibility_note", "")))
                if not data["mods"]:
                    lines.append("Brak wyników dla filtrów wersji." if polish else "No results for these version filters.")
            elif isinstance(data, dict) and "levels" in data:
                for level in data["levels"]:
                    lines += [f"{level.get('name')} — ID {level.get('id')}",
                              f"Creator: {level.get('creator')}; {level.get('difficulty')} {level.get('demon_difficulty')}",
                              f"Stars: {level.get('stars')}; moons: {level.get('moons')}; rating: {level.get('rating')}",
                              f"Song ID: {level.get('song_id')}; official song index: {level.get('official_song_index')}"]
                lines.append(str(data.get("note", "")))
            elif isinstance(data, dict) and "song_id" in data:
                if "screen" in data:
                    lines.append("Zrzut danych ekranu przy otwarciu asystenta; bez aktualizacji serwerowej." if polish else "Screen metadata captured when the assistant opened; not a live server refresh.")
                    lines.append(json.dumps(data, ensure_ascii=False, indent=2))
                    continue
                for key in ["title", "author", "song_id", "url", "gd_available", "genre", "mood", "licence", "note"]:
                    lines.append(f"{key}: {data.get(key) if data.get(key) is not None else ('brak danych' if polish else 'not supplied')}")
            elif isinstance(data, dict) and "results" in data:
                for item in data["results"]:
                    lines.append(str(item.get("title", "?")))
                    for key in ["author", "song_id", "genre", "mood", "gd_available", "catalogue_tags", "snippet", "url"]:
                        if key in item:
                            value = item[key]
                            if value is None:
                                value = "brak danych" if polish else "not supplied"
                            elif isinstance(value, (list, dict)):
                                value = json.dumps(value, ensure_ascii=False)
                            lines.append(f"{key}: {value}")
                    lines.append("")
                if not data["results"]:
                    lines.append("Brak wyników." if polish else "No results.")
                lines.append(str(data.get("note", "")))
            elif isinstance(data, dict) and "creators" in data:
                lines.extend(describe(data, polish))
            else:
                # Preserve schema without guessing what unknown fields mean.
                lines.append(json.dumps(data, ensure_ascii=False, indent=2)[:16000])
    return "\n".join(lines)[:40000]


def describe(row, polish):
    if not isinstance(row, dict):
        return [str(row)]
    if "position" not in row:
        return [json.dumps(row, ensure_ascii=False)[:3000]]
    name = row.get("name", "?")
    position = row.get("position")
    out = [f"#{position} {name}" if position is not None else f"{name} — position: null"]
    for key, label in [("level_id", "Level ID"), ("id", "Pointercrate ID")]:
        if row.get(key) is not None:
            out.append(f"{label}: {row[key]}")
    for key in ["publisher", "verifier"]:
        if isinstance(row.get(key), dict):
            out.append(f"{key}: {row[key].get('name', '?')}")
    if "creators" in row:
        out.append(("Twórcy: " if polish else "Creators: ") + ", ".join(str(c.get("name", "?")) for c in row["creators"]))
    return out
