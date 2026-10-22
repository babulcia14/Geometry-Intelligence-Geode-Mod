# Geometry Intelligence AI - 1.0.0-alpha.1

**Your AI companion for Geometry Dash.** A C++/Geode project with spatial, electrical Python and external weights for a small Transformer trained from random initialization. No API key required and no download of a pre-built language model.

This is a **development version for testing**, not a declaration of completion of all 1.0 functionalities. The core, integrations, training, server, and modpack are real. The quality of free communication of the small model is available. API facts are marked as read tool; tool selection is in default rule-based operation. We do not attribute these capabilities to the neural network.

## first: game version

The package is built for **Windows x64, GD 2.2081, Geode 5.11.2**. Geode 5.11.2 was last reviewed on October 21, 2026. Its description is still released as a primary goal and is subject to compatibility regulations for the newer GD 2.209: [release review](https://github.com/geode-sdk/geode/releases/tag/v5.11.2).

**This mod has not been confirmed to work in GD 2.209 or be used in a running game.** Do not change the `gd` number in `mod.json` to bypass compatibility checks. The number alone will not adjust the addresses of game functions and structures. Do not rule out forced compatibility based on this README.

## Running on this computer

1. Open the project folder. In the environment where the project was prepared, `.venv`, the CPU library, and the trained weights are already prepared.
2. Click **Start_AI.cmd**. The window opens. After revealing the weights, `GI is ready: http://127.0.0.1:18765/v1`. 3. Click **Check_AI.cmd**. It should have status: ready, origin: random-initialization, and model 'gi-tiny'. This is a server usage, not an assessment of its intelligence.
4. If you have a GD/Geode-compatible server, close the game and copy 'dist/aer.geometry-intelligence.geode' to the 'geode/mods' folder in your game directory. The default Steam path is: 'C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\geode\mods'. The project does not automatically install the mod and does not change saved games.
5. Launch the game. The mod has added a **GI** button to the menu, level pause, and custom levels. The appearance and interactions of these screens still require in-game testing.
6. In the GI window, select **Connect**. Default settings are at `http://127.0.0.1:18765/v1` and the `gi-tiny` model.
7. To enable internet data, in **Settings → Privacy**, select **Allow public APIs / Allow public APIs**. **Allow remote intelligence** leaves communication to the local server.
8. Try `What is the current Top 1 Demon?`, then `Who created it?`. Others also include `How does Move Trigger work?` and `What is BetterEdit?`.

Closing the chat window or pressing **Stop** cancels the immediate client task. `Ctrl+C` in the server window removes the AI. The server starts before using the mod; the mod does not install Python or run programs in the background.

## On another computer

Unzip the full `Geometry-Inteligencja-1.0.0-alfa.1.zip` archive. It contains the code, the `.geode` package, custom inference weights, training data, scripts, and documentation. It does not include the Geode SDK, compiler, PyTorch, or the game.

I installed **Python 3.11–3.14 x64** from [python.org](https://www.python.org/downloads/windows/), marking the comment in the PATH. Click **Setup_AI.cmd**, and after selecting **Start_AI.cmd**. Configure the PyTorch CPU download based on the official PyTorch index. Do not download third-party weights. The initial installation requires internet access and library space. There is no need to pay for the API; the computer takes over the user experience.

The `.cmd` scripts set `ExecutionPolicy Bypass` exclusively for the native PowerShell process. They do not change the global Windows policy.

## What is implemented

| Part | Actual behavior |

|---|---|
| GI Core | Model → tool → action → response loop, tool and context limits, cancellation, errors |
| GI Language Engine | Custom PyTorch Transformer, UTF-8 tokenizer, training from scratch, checkpoints, server, and real token streaming |
| Search GI | PL/EN rules for future questions; with optional larger engine selection by its invoker |
| GI Knowledge | 22 bilingual reference documents; mechanics, base level 2.2 documentation, editor, and unverified scopes |
| Pointer Box | Ranking, search, creators/verifiers/publishers, reading, reading records, local data observations |
| Geoda | Search the official index with GD/Geode/Windows filtering, authors, description, and versions |
| Music | Free MusicBrainz metadata; GD-enabled track ID lookup, server-enabled; optional Brave for discovering Newgrounds sites |
| Level | Metadata parser questioning GD, name/ID lookup; reading registration data |
| GI memory | Conversation context, history in RAM, optional SQLite-free storage in Windows DPAPI-encrypted JSON, TTL cache |
| GI interface | Dark window, Polish font, history with page scrolling, Geode settings
