# Handoff — Strategic-map question mark

**Date:** 2026-08-27  
**Branch:** `feature/multi-edition-detector`

## Fix

Wildfire strategic-map enemy markers could show only half of the red question mark because some Wildfire layouts used the legacy `boxes.sti` question-mark frame.

Updated `src/game/Strategic/Map_Screen_Interface_Map.cc`:

- Wildfire full-size and widescreen layouts now use the embedded complete glyph through `DrawQuestionMarkInCell()`.
- Vanilla `640x480` keeps the legacy sprite path.
- Cell coordinates remain unchanged; no layout or enemy-state logic changes.
- No new assets or dependencies.

## Verification

- User verified the corrected marker on the Android phone.
- Android run command:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella" && \
./tools/build-android-debug.sh && \
adb install -r android/app/build/outputs/apk/debug/app-debug.apk && \
adb shell am force-stop io.github.ja2stracciatella && \
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

- `git diff --check`: passed.

## Follow-up

No follow-up required unless visual regression appears on vanilla `640x480` or another map resolution.
