# AI Context

Project: JA2 Stracciatella Multi-Edition

Goal:
Fork JA2-Stracciatella to support multiple JA2-based editions via detector/profile/importer, without distributing commercial game assets.

MVP:
- Keep Vanilla/Gold stable.
- Add folder detector for Vanilla, Gold, Wildfire5, Wildfire6, Unknown.
- Do not implement full Wildfire gameplay yet.

Rules:
- Do not add commercial assets.
- Do not commit real game files.
- Do not hard-code personal paths.
- Do not refactor broadly.
- Make small commits.
- Explain files to change before editing.
- Prefer tests with dummy fixtures.

Sprint 1:
- Add Multi-Edition Detector.
- Return edition, confidence, missing files, mismatched files, reasons.
- Do not connect deeply to engine yet.
