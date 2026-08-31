# HANDOFF — Strategic-map unexplored-sector shading

## Context

- Date: 2026-08-31
- Branch: `feature/multi-edition-detector`
- Target: JA2 Wildfire on a physical Android device

Wildfire's `interface/b_map.sti` is 16-bit artwork. The strategic map renderer only applied airspace tint through the vanilla 8-bit palette path. On the Android `934x480` layout, the tint path therefore returned without shading, leaving unexplored sectors fully visible.

## Fix

Updated `src/game/Strategic/Map_Screen_Interface_Map.cc`:

- `DrawMap()` still decides explored state from `SF_ALREADY_VISITED`.
- `ShadeMapElem()` detects 16-bit map art through `guiBIGMAP->BPP() == 16`.
- Each map cell is tinted directly in the 16-bit destination buffer.
- Unexplored airspace cells retain dark green/dark red shading.
- Explored airspace cells retain light green/light red shading.
- Vanilla 8-bit `b_map.pcx` keeps the existing palette-based path.
- No new assets or dependencies.

## Verification

- `git diff --check`: passed.
- Desktop `ja2` build: passed (`Built target ja2`).
- Android debug build: passed.
- APK installed over USB: `Success`.
- `LauncherActivity` active after relaunch.
- Logcat: no fatal exception, SIGSEGV, or Android runtime crash.
- User visually confirmed unexplored map shading works on Android.

## Build/install

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella" && \
./tools/build-android-debug.sh && \
adb install -r android/app/build/outputs/apk/debug/app-debug.apk && \
adb shell am force-stop io.github.ja2stracciatella && \
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

## Remaining scope

- CTest reports no registered tests in the current build tree.
- No release APK generated.
- No additional changes to strategic exploration semantics.
