# Third-party notices and provenance

- nlohmann/json: MIT; included header and license in `third_party/nlohmann`.
- Noto Sans glyph atlas: derived from Noto Sans, SIL Open Font License 1.1; `resources/OFL.txt`. Atlas generator: `scripts/generate_font.py`. The full upstream font is not required at runtime.
- Geode SDK and its dependencies: downloaded for compilation, not bundled as source in the release archive. Their respective licenses apply: [Geode repository](https://github.com/geode-sdk/geode).
- PyTorch and NumPy: installed separately by Setup_AI, with their respective upstream licenses. No pre-trained language-model weights are downloaded.
- Geometry Dash and its assets belong to their respective rights holders. The archive does not contain the game, SDK binaries, level strings or the complete RobTop editor guide. Native UI sprites are referenced from the user's installed game.
- Retrieved Pointercrate, Geode, MusicBrainz, Wikipedia and other service data retain their source terms. API access is not a grant of ownership or training rights. Wikipedia snippets are attributed to contributors with CC BY-SA in provider metadata.
- Project code was produced with AI assistance. Training examples and reference notes are AI-authored original project text with explicit provenance. No independent human audit or Geode index approval is asserted.

See `docs/SOURCES.md`, `docs/MODEL_CARD.md` and `python/data/README.md` for scope and limitations.

- Rub logo: deterministic image composition requested by the user, combining their supplied 8284-gd-auto.png with a circular reconstruction of GJ_button_04-hd.png. No image generation model was used. Source artwork rights remain with the respective holders; this artwork is not covered by the project code MIT license.
