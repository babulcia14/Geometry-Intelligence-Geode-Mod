"""Reproducible, original synthetic starter corpus, not a chat reply database.

Only train.py reads this file's output. The server does not look up answers in
the training set. This corpus is too small to teach general Polish or English.
"""
import json
from pathlib import Path

# Each item is a topic group plus English and Polish examples written for this
# project. Splitting happens by group, so translations cannot leak into validation.
PAIRS = [
    ("hello", "Hello", "Hello! I am Geometry Intelligence, your experimental Geometry Dash assistant.", "Cześć", "Cześć! Jestem Geometry Intelligence, eksperymentalny asystent Geometry Dash."),
    ("identity", "What are you?", "I am a small language model trained from scratch for this project. My answers can be wrong. Live data comes from separate tools.", "Czym jesteś?", "Jestem małym modelem językowym trenowanym od zera. Mogę się mylić. Aktualne dane pobierają osobne narzędzia."),
    ("thanks", "Thank you", "You are welcome. Tell me which part you want to try next.", "Dziękuję", "Nie ma za co. Napisz, którą część chcesz teraz sprawdzić."),
    ("uncertainty", "Do you know everything?", "No. My training set is small, and I do not know every level or player. I should say when information is unavailable.", "Czy wiesz wszystko?", "Nie. Mam mały zbiór treningowy i nie znam wszystkich leveli. Brak danych trzeba uczciwie zaznaczyć."),
    ("game", "What is Geometry Dash?", "Geometry Dash is a game by RobTop Games. You guide an icon through obstacles, and players can build their own levels.", "Co to jest Geometry Dash?", "Geometry Dash to gra RobTop Games. Sterujesz ikoną między przeszkodami i możesz budować własne poziomy."),
    ("history", "When did Geometry Dash begin?", "The original mobile release was in 2013. Historical dates and the current game version are different kinds of information.", "Kiedy powstało Geometry Dash?", "Pierwsza mobilna wersja ukazała się w 2013 roku. Historia gry to co innego niż jej aktualna wersja."),
    ("cube", "How does cube mode work?", "The cube jumps when you press. Orbs, pads, gravity and speed portals change the timing of a section.", "Jak działa cube?", "Kostka skacze po naciśnięciu. Orby, pady, grawitacja i portale prędkości zmieniają rytm sekcji."),
    ("ship", "How do I control the ship?", "Holding raises the ship relative to its gravity. Releasing lets it fall. Practise short controlled presses.", "Jak sterować statkiem?", "Przytrzymanie unosi statek względem grawitacji, a puszczenie pozwala mu opadać. Ćwicz krótkie naciśnięcia."),
    ("wave", "What is wave mode?", "The wave alternates diagonal movement when you hold and release. Speed and mini portals change the feel.", "Jak działa wave?", "Wave porusza się po przekątnych zależnie od przytrzymania. Zmiana prędkości i rozmiaru wpływa na sterowanie."),
    ("ball", "What does the ball do?", "The ball switches gravity when activated on a surface. Plan the landing and the next switch.", "Co robi ball?", "Ball zmienia grawitację po aktywacji na powierzchni. Zaplanuj lądowanie i kolejną zmianę."),
    ("ufo", "How does UFO move?", "Each press gives the UFO a small jump. Repeated presses control height through the section.", "Jak porusza się UFO?", "Każde naciśnięcie daje UFO mały skok. Kolejne naciśnięcia kontrolują wysokość."),
    ("robot", "How does robot jumping work?", "Robot jump height depends on how long you hold the input. Test both short and long jumps.", "Jak skacze robot?", "Wysokość skoku robota zależy od długości przytrzymania. Sprawdź krótki i długi skok."),
    ("spider", "What is the spider?", "The spider switches to the opposite surface instantly when activated. Check the landing surface before the input.", "Czym jest spider?", "Spider natychmiast przenosi się na przeciwną powierzchnię. Sprawdź, gdzie wyląduje."),
    ("swing", "What is swing mode?", "Swing changes gravity in flight. Momentum means you need to anticipate the next obstacle.", "Jak działa swing?", "Swing zmienia grawitację w locie. Bezwładność wymaga przewidywania kolejnej przeszkody."),
    ("practice", "How can I practise a hard part?", "Use practice mode checkpoints near the difficult section. Learn the transition, then try longer runs.", "Jak ćwiczyć trudną sekcję?", "Użyj checkpointów w practice mode blisko trudnego miejsca. Naucz się przejścia, potem wydłużaj próby."),
    ("difficulty", "What are demon difficulties?", "Demon levels have Easy, Medium, Hard, Insane and Extreme subcategories. The Demonlist is a separate ranking.", "Jakie są trudności demonów?", "Demony dzielą się na Easy, Medium, Hard, Insane i Extreme. Demonlista jest osobnym rankingiem."),
    ("rank", "What is the current Top 1?", "A current rank must be checked with Pointercrate. Training memories are not a live ranking.", "Jaki jest obecnie Top 1?", "Aktualną pozycję trzeba sprawdzić w Pointercrate. Pamięć treningowa nie jest rankingiem na żywo."),
    ("creator", "Is a verifier the creator?", "No. The verifier completes the level for verification. The publisher uploads it, and the creators may be several people.", "Czy verifier jest twórcą?", "Nie musi nim być. Verifier przechodzi poziom, publisher go publikuje, a autorów może być wielu."),
    ("groups", "What is a Group ID?", "A Group ID selects a set of objects. A trigger can affect that group. Its own group and target group have different roles.", "Co to Group ID?", "Group ID wybiera zestaw obiektów. Trigger może działać na tę grupę. Własna grupa triggera i grupa docelowa mają inne role."),
    ("move", "How does Move Trigger work?", "Assign objects to a group. Set that group as the Move target, then choose displacement, duration and easing. Test activation.", "Jak działa Move Trigger?", "Nadaj obiektom grupę. W Move ustaw tę grupę jako cel, potem przesunięcie, czas i easing. Sprawdź aktywację."),
    ("rotate", "How do I rotate an object?", "Use Rotate with the object's target group and a separate centre group for the pivot. Choose angle and duration.", "Jak obrócić obiekt?", "Użyj Rotate z grupą obiektu i osobną grupą środka obrotu. Ustaw kąt oraz czas."),
    ("loop", "How do I repeat a rotation?", "Use a delayed Spawn loop with Spawn Triggered and Multi Trigger on the repeating triggers. The delay must be positive. Provide a stop condition.", "Jak zapętlić obrót?", "Użyj pętli Spawn z dodatnim opóźnieniem. Powtarzane triggery potrzebują Spawn Triggered i Multi Trigger. Dodaj warunek zatrzymania."),
    ("spawn", "What does Spawn Trigger do?", "Spawn activates spawn-triggered objects in its target group after a chosen delay. It does not create new level objects.", "Co robi Spawn Trigger?", "Spawn uruchamia triggery w grupie docelowej po opóźnieniu. Nie tworzy nowych obiektów poziomu."),
    ("alpha", "How do I hide decoration?", "Use Alpha on its group. An opacity of zero hides it, while one makes it visible. Invisible objects can still collide.", "Jak ukryć dekorację?", "Użyj Alpha dla jej grupy. Opacity zero ukrywa, a jeden pokazuje. Niewidoczne obiekty mogą nadal mieć kolizję."),
    ("pulse", "What is Pulse Trigger?", "Pulse temporarily changes a group or colour channel. Set fade in, hold and fade out, and check the target mode.", "Co robi Pulse Trigger?", "Pulse tymczasowo zmienia kolor grupy albo kanału. Ustaw fade in, hold i fade out oraz odpowiedni tryb celu."),
    ("camera", "How do I use Static Camera?", "Choose the target group for the camera position. Plan an exit from static mode so the view follows gameplay again.", "Jak użyć Static Camera?", "Wybierz grupę docelową pozycji kamery. Zaplanuj wyjście z trybu static, aby widok ponownie śledził rozgrywkę."),
    ("keyframes", "What are keyframes for?", "Keyframes describe successive transformation states. Check animation ID, target group, timing and interpolation together.", "Do czego są keyframes?", "Keyframes opisują kolejne stany animacji. Sprawdź razem ID animacji, grupę celu, czas i interpolację."),
    ("collision", "Are Block IDs the same as Group IDs?", "No. Collision block IDs identify collision blocks. Group IDs select objects and triggers. Do not interchange them.", "Czy Block ID to Group ID?", "Nie. Block ID identyfikuje bloki kolizji. Group ID wybiera obiekty i triggery. Nie zamieniaj ich."),
    ("shader", "How should I use shader effects?", "Keep effects readable and short. Reset them after the section and check that hazards remain visible.", "Jak używać shaderów?", "Zachowaj czytelność i ogranicz czas efektów. Zresetuj je po sekcji i sprawdź widoczność przeszkód."),
    ("optimization", "How can I reduce lag?", "Measure the demanding section. Reduce overlapping transparent decoration, excess particles and unnecessary continuous loops.", "Jak zmniejszyć lagi?", "Zmierz wydajność trudnej sekcji. Ogranicz nakładające się przezroczystości, cząsteczki i zbędne pętle."),
    ("geode", "What is Geode?", "Geode is a mod loader and SDK for Geometry Dash. Use its official index to inspect mod descriptions and declared compatibility.", "Co to Geode?", "Geode to loader modów i SDK dla Geometry Dash. Oficjalny indeks pokazuje opisy i deklarowaną kompatybilność."),
    ("install", "How do I install a Geode mod?", "Open Geode in the game, find the mod in the index, inspect its requirements and install it. Restart when requested.", "Jak zainstalować mod Geode?", "Otwórz Geode w grze, znajdź mod w indeksie, sprawdź wymagania i zainstaluj. Uruchom grę ponownie, jeśli trzeba."),
    ("crash", "Why does my mod crash?", "I need the exact error, GD version and mod versions. Test conflicts one at a time. Remove private paths before sharing logs.", "Dlaczego mod się crashuje?", "Potrzebny jest dokładny błąd, wersja GD i wersje modów. Sprawdzaj konflikty pojedynczo. Usuń prywatne dane z logów."),
    ("compatibility", "Does compatible mean no crashes?", "No. Declared version compatibility does not prove that every combination of mods works. Runtime testing is still needed.", "Czy kompatybilny oznacza brak crashy?", "Nie. Deklaracja wersji nie dowodzi działania każdej kombinacji modów. Potrzebne są testy w grze."),
    ("music", "Can every Newgrounds song be used in GD?", "No. A visible audio page is not proof of GD availability. The artist must meet Newgrounds and GD approval requirements.", "Czy każdy utwór Newgrounds działa w GD?", "Nie. Sama strona utworu nie potwierdza dostępności. Autor musi spełniać wymagania Newgrounds i zatwierdzenia w GD."),
    ("songid", "Where is the Song ID?", "It is the number in a Newgrounds audio/listen page URL. Never invent an ID; verify it against the actual source.", "Gdzie znaleźć Song ID?", "To liczba w adresie strony Newgrounds audio/listen. Nie zgaduj ID. Sprawdź prawdziwe źródło."),
    ("license", "Does GD availability give me a music licence?", "No. Availability in a game and permission to reuse music elsewhere are separate. Read the artist's stated terms.", "Czy dostępność w GD daje licencję?", "Nie. Działanie w grze i prawo do użycia muzyki poza nią to osobne sprawy. Sprawdź warunki autora."),
    ("coins", "What are Secret Coins and User Coins?", "Secret Coins and User Coins are different collectibles. User Coins belong to community levels; their validation matters.", "Czym różnią się Secret Coins i User Coins?", "To różne przedmioty do zbierania. User Coins znajdują się w poziomach społeczności i wymagają zatwierdzenia."),
    ("packs", "What is a Map Pack?", "A Map Pack groups community levels into a set with completion rewards. Check the in-game pack for its exact levels.", "Co to Map Pack?", "Map Pack łączy poziomy społeczności w zestaw z nagrodami. Dokładny skład sprawdź w grze."),
    ("gauntlets", "What is a Gauntlet?", "A Gauntlet is an ordered collection of community levels with a completion reward. Its contents are shown in the game.", "Co to Gauntlet?", "Gauntlet to uporządkowany zestaw poziomów społeczności z nagrodą za ukończenie. Skład znajdziesz w grze."),
    ("paths", "What are Paths?", "Paths are progression tracks introduced with 2.2. Check the current in-game panel for unlock requirements and rewards.", "Czym są Paths?", "Paths to ścieżki postępu dodane w 2.2. Wymagania i nagrody sprawdź w aktualnym panelu gry."),
    ("daily", "What is today's Daily level?", "That is time-sensitive information. Check the current in-game Daily panel; I must not answer from old training data.", "Jaki jest dzisiejszy Daily?", "To zmienna informacja. Sprawdź aktualny panel Daily w grze. Nie wolno odpowiadać ze starego treningu."),
    ("icons", "How do I unlock this icon?", "Select the icon in the icon kit and read its unlock condition. Tell me the exact icon and message if you need help.", "Jak odblokować tę ikonę?", "Wybierz ikonę w icon kit i odczytaj warunek. Podaj dokładną ikonę i komunikat, jeśli potrzebujesz pomocy."),
    ("achievements", "How do I get this achievement?", "Tell me its exact name. Achievement requirements vary, so guessing a number would be misleading.", "Jak zdobyć osiągnięcie?", "Podaj jego dokładną nazwę. Wymagania są różne, więc nie będę zgadywać liczby."),
    ("privacy", "Do you send my chats to the internet?", "The local model runs on this computer. External searches need permission. A remote model would receive the conversation you send it.", "Czy wysyłasz rozmowy do internetu?", "Lokalny model działa na tym komputerze. Wyszukiwanie wymaga zgody. Model zewnętrzny otrzymywałby wysłany kontekst rozmowy."),
    ("memory", "Can you remember our conversation?", "The app can keep several recent messages as context. Saving history to disk is a separate setting you can disable.", "Czy pamiętasz rozmowę?", "Aplikacja może przekazywać ostatnie wiadomości jako kontekst. Zapis historii na dysku jest osobnym ustawieniem."),
    ("cost", "Do I need to pay for this local model?", "There is no API fee for our local model. It uses your processor, memory and electricity. Training takes time.", "Czy muszę płacić za lokalny model?", "Nie ma opłaty za API naszego modelu. Używa procesora, pamięci i prądu. Trening zajmuje czas."),
    ("training", "Did you use pretrained weights?", "This project's scratch model starts with random weights. Training changes those weights using the provided dataset.", "Czy użyto gotowych wag?", "Model scratch zaczyna od losowych wag. Trening zmienia je na podstawie przygotowanego zbioru danych."),
    ("safety", "Build the whole level for me", "I can explain editor settings, but I do not place, modify or delete level objects. You remain the creator.", "Zbuduj za mnie poziom", "Mogę wyjaśnić ustawienia edytora, ale nie stawiam, nie zmieniam ani nie usuwam obiektów. Ty tworzysz level."),
    ("sources", "Why show sources?", "Sources let you check where information came from. A retrieval time says when the app fetched it, not when the site changed it.", "Po co podawać źródła?", "Źródła pozwalają sprawdzić informację. Czas pobrania mówi, kiedy aplikacja pobrała dane, nie kiedy strona je zmieniła."),
]


def main():
    out = Path(__file__).parent / "data" / "starter.jsonl"
    out.parent.mkdir(parents=True, exist_ok=True)
    records = []
    for group, enq, ena, plq, pla in PAIRS:
        for q, a, language in [(enq, ena, "en"), (plq, pla, "pl")]:
            records.append({"group": group, "source": "project-authored-synthetic-2026-10-21",
                            "license": "project training permission; AI-authored factual paraphrases",
                            "language": language, "messages": [{"role": "user", "content": q},
                                                                  {"role": "assistant", "content": a}]})
    out.write_text("".join(json.dumps(r, ensure_ascii=False) + "\n" for r in records), encoding="utf-8")
    print(f"Wrote {len(records)} original synthetic examples to {out}")


if __name__ == "__main__":
    main()
