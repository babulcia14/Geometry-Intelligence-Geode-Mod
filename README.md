# Geometry Intelligence AI — 1.0.0-alpha.1

**Your AI Companion for Geometry Dash.** Projekt C++/Geode z osobnym, lokalnym silnikiem Python i własnymi wagami małego Transformera trenowanego od losowej inicjalizacji. Bez obowiązkowego klucza API i bez pobierania gotowego modelu językowego.

To **wersja rozwojowa do testów**, a nie deklaracja ukończenia wszystkich funkcji stabilnego 1.0. Rdzeń, integracje, trening, serwer i paczka moda są rzeczywiste. Jakość swobodnej rozmowy małego modelu jest obecnie niewystarczająca. Fakty z API są wyświetlane jako oznaczony odczyt narzędzi; wybór narzędzi w domyślnym silniku opiera się na regułach. Nie przypisujemy tych zdolności sieci neuronowej.

## Najpierw: wersja gry

Paczka jest zbudowana dla **Windows x64, GD 2.2081, Geode 5.11.2**. Geode 5.11.2 był najnowszym oficjalnym wydaniem sprawdzonym 21.10.2026. Jego opis nadal wskazuje 2.2081 jako główny cel i jednocześnie wymienia poprawki trybu zgodności dla nowszego GD 2.209: [opis wydania](https://github.com/geode-sdk/geode/releases/tag/v5.11.2).

**Nie potwierdzono działania tego moda w GD 2.209 ani jego wyglądu w uruchomionej grze.** Nie zmieniaj numeru `gd` w `mod.json`, żeby ominąć sprawdzanie zgodności. Sam numer nie dostosuje adresów funkcji i struktur gry. Nie włączaj wymuszonej zgodności na podstawie tego README.

## Uruchomienie na tym komputerze

1. Otwórz folder projektu. W środowisku, w którym przygotowano projekt, `.venv`, biblioteki CPU i wytrenowane wagi są już przygotowane.
2. Kliknij **Start_AI.cmd**. Pozostaw okno otwarte. Po załadowaniu wag zobaczysz `GI ready: http://127.0.0.1:18765/v1`.
3. Kliknij **Check_AI.cmd**. Powinien pokazać `status: ready`, `origin: random-initialization` i model `gi-tiny`. To sprawdzenie serwera, nie ocena jego inteligencji.
4. Jeśli masz zgodne GD/Geode, zamknij grę i skopiuj `dist/aer.geometry-intelligence.geode` do folderu `geode/mods` w katalogu gry. Domyślna ścieżka Steam: `C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\geode\mods`. Projekt nie instaluje moda automatycznie i nie zmienia zapisów gry.
5. Uruchom grę. Mod dodaje przycisk **GI** w menu głównym, pauzie poziomu i ekranie własnego poziomu. Wygląd oraz interakcje tych ekranów nadal wymagają testu w grze.
6. W oknie GI wybierz **Połączenie**. Domyślne ustawienia to adres `http://127.0.0.1:18765/v1` i model `gi-tiny`.
7. Aby włączyć dane internetowe, w **Ustawienia → Privacy** zaznacz **Allow Public APIs / Zezwól na publiczne API**. **Allow Remote AI** pozostaw wyłączone dla lokalnego serwera.
8. Wypróbuj `What is the current Top 1 Demon?`, a następnie `Who created it?`. Spróbuj też `Jak działa Move Trigger?` oraz `What is BetterEdit?`.

Zamknięcie okna czatu lub **Stop** anuluje bieżące zadanie klienta. `Ctrl+C` w oknie serwera zatrzymuje AI. Serwer uruchamiasz osobno przed użyciem moda; mod nie instaluje Pythona ani nie uruchamia programów w tle.

## Na innym komputerze

Rozpakuj pełne archiwum `Geometry-Intelligence-1.0.0-alpha.1.zip`. Zawiera kod, paczkę `.geode`, własne wagi do inferencji, dane treningowe, skrypty i dokumentację. Nie zawiera Geode SDK, kompilatora, PyTorch ani gry.

Zainstaluj **Python 3.11–3.14 x64** z [python.org](https://www.python.org/downloads/windows/), zaznaczając dodanie do PATH. Kliknij **Setup_AI.cmd**, a po zakończeniu **Start_AI.cmd**. Setup pobiera PyTorch CPU i zależności z oficjalnego indeksu PyTorch. Nie pobiera cudzych wag. Pierwsza instalacja wymaga internetu i miejsca na biblioteki. Nie trzeba płacić za API; komputer zużywa energię.

Skrypty `.cmd` ustawiają `ExecutionPolicy Bypass` wyłącznie dla własnego procesu PowerShell. Nie zmieniają globalnej polityki Windows.

## Co jest zaimplementowane

| Część | Rzeczywiste zachowanie |
|---|---|
| GI Core | Pętla model → narzędzie → dowody → odpowiedź, limity narzędzi i kontekstu, anulowanie, błędy |
| GI Language Engine | Własny Transformer PyTorch, tokenizer UTF-8, trening od zera, checkpointy, serwer i prawdziwy streaming tokenów |
| GI Search | Reguły PL/EN dla najczęstszych pytań; przy opcjonalnym większym silniku wybór przez jego tool calling |
| GI Knowledge | 22 dwujęzyczne dokumenty referencyjne; mechaniki, oficjalne poziomy bazowego 2.2, edytor i zakresy niezweryfikowane |
| Pointercrate | Ranking, wyszukiwanie, twórcy/weryfikator/publikujący, gracze, odczyt rekordów, datowane lokalne obserwacje |
| Geode | Wyszukiwanie oficjalnego indeksu z filtrem GD/Geode/Windows, autorzy, opisy i wersje |
| Music | Darmowe metadane MusicBrainz; sprawdzanie podanego Song ID przez GD, jeśli serwer pozwala; opcjonalny Brave dla odkrywania stron Newgrounds |
| Level | Parser metadanych publicznego protokołu GD, wyszukiwanie nazwy/ID; dobrowolny odczyt danych widocznego poziomu |
| GI Memory | Kontekst rozmowy, historia w RAM, opcjonalny zapis SQLite-free w JSON szyfrowanym Windows DPAPI, TTL cache |
| GI Interface | Ciemne okno, polski font, historia z przewijaniem stron, ustawienia Geode, kopiowanie, źródła, stop, usuwanie danych |

**Ograniczenia:** nie ma ogólnego rozumienia języka na poziomie współczesnych dużych modeli, wyszukiwania podobieństwa audio, kompletnej tabeli ikon/nagród ani pełnej encyklopedii społeczności. Darmowy web search obejmuje Wikipedię, a wyszukiwanie muzyki MusicBrainz. Przyjęcie przez oficjalny indeks Geode nie jest potwierdzone. Zobacz [STATUS.md](docs/STATUS.md) i [TEST_REPORT.md](docs/TEST_REPORT.md).

## Przykłady pytań

- `Show me the Top 10 hardest demons.`
- `Who created Tidal Wave?` / `Kto stworzył Tidal Wave?`
- `What position is Bloodbath?`
- `Show me information about player "Zoink".`
- `Find me a mod for improving the level editor.`
- `How do I make an object rotate forever?`
- `Jak działa Static Camera?`
- `Find a calm song for a space level.` — to wyszukiwanie tekstowych metadanych, nie odsłuch ani gwarancja użycia w GD.
- `Tell me about this level.` — włącz Game Context i otwórz GI z pauzy/ekranu własnego poziomu.
- `Find level "Nine Circles".` — rzeczywiste zapytanie; w testach serwer GD odmówił dostępu kodem 403.

W małym silniku nietypowe sformułowanie może nie trafić do reguły. Wtedy odpowiedź generuje model, którego tekst może być niespójny. Podawanie dokładnej nazwy w cudzysłowie pomaga narzędziom. Rankingu nie zapisano na stałe w kodzie.

## Dokumentacja i rozwój

- [Architektura i mapa plików](docs/ARCHITECTURE.md)
- [Własne wagi, dane, sprzęt i koszty](docs/TRAINING.md)
- [Karta modelu i wyniki eksperymentów](docs/MODEL_CARD.md)
- [Dokładna kompilacja C++ i .geode](docs/BUILD.md)
- [API, źródła i ograniczenia dostępu](docs/SOURCES.md)
- [Prywatność i ustawienia](docs/PRIVACY.md)
- [Wykonane testy](docs/TEST_REPORT.md)
- [Etapy i brakujące elementy stabilnego wydania](docs/STATUS.md)

Najważniejszy następny krok dla jakości AI to większy, legalnie uzyskany i sprawdzony korpus PL/EN oraz rzetelna ewaluacja. Wielokrotne uczenie tych samych kilkuset przykładów nie zastąpi tych danych.
