# Pochodzenie danych

`starter.jsonl` to 100 oryginalnych syntetycznych przykładów (50 par PL/EN), utworzonych w `build_starter.py`. `domain.jsonl` to 384 przykłady z 42 grup, utworzone w `build_domain.py` przez dołożenie oryginalnych notatek projektu. Nie są zbiorem pobranych prywatnych rozmów ani gotową tabelą odpowiedzi dla serwera.

Teksty zostały napisane przez AI w ramach zlecenia stworzenia i wytrenowania tego projektu. Są jawnie oznaczone jako syntetyczne. Rekordy zawierają `source`, opis zgody w `license`, `group`, `language`, a notatki także `references`. Zgoda na trening tych przykładów w projekcie nie jest licencją na pełne materiały pod linkami referencyjnymi. Nie dołączono pełnego tekstu podręcznika RobTop ani cudzych wag.

Teksty nie są sprawdzonym przez człowieka korpusem eksperckim. Przed rozszerzeniem treningu popraw błędy, skontroluj prawa do każdego nowego źródła i deduplikację. Nie oznaczaj bez sprawdzenia losowych danych jako legalnych przez wpisanie samego pola `license`.

SHA256 oraz liczebność konkretnych podzbiorów znajdują się w metadanych checkpointu i raporcie treningu. Baza runtime `resources/knowledge.json` oraz dataset treningowy są osobnymi plikami. Edycja bazy runtime nie aktualizuje automatycznie wag.
