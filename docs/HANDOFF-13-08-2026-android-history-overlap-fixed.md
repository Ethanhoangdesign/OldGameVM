# HANDOFF — 13/08/2026

## Android 934x480 — History overlap fixed

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 |
| Scope | Strategic-map history log, soldier panel, map anchor |
| Status | Code change completed; runtime verification pending |

---

## User request

The red-circled history text/panel overlapped the soldier area at 934x480. Fix the overflow without shifting the strategic map inaccurately, especially underground maps.

---

## Root cause

At 934x480:

```text
history strip:       x=0..170
bottom panel base:   x=934-763=171
soldier column:      x=147  <-- overlap
map start:           x=147+270=417
```

The history frame correctly reserved the left strip, but the soldier column and vanilla map still used the centered 640px offset (`m_stdScreenOffsetX`).

---

## Implemented fix

Changed [src/game/UILayout.cc](../src/game/UILayout.cc):

- `get_MAP_LEFT_COL_X()` now returns `get_MAP_BOTTOM_BASE_X()` for `isWidescreenLayout()`.
- `get_MAP_VIEW_START_X()` now returns `get_MAP_BOTTOM_BASE_X() + 270` for `isWidescreenLayout()`.
- Full-size Wildfire and normal centered layouts remain unchanged.

At 934x480:

```text
history strip:       x=0..170
soldier column:      x=171
map start:           x=441
```

The map keeps the original vanilla relative offset (`+270`). Grid size, map Y position, map rectangles, cursor regions, level markers, surface map, and underground map all consume the shared `g_ui` coordinates. No underground-specific offset was added.

Updated documentation comment in [src/game/UILayout.h](../src/game/UILayout.h).

---

## Files changed

```text
src/game/UILayout.cc
src/game/UILayout.h
```

---

## Verification

Passed:

```text
git diff --check -- src/game/UILayout.cc src/game/UILayout.h
```

Expected coordinate checks:

```text
934x480:  MAP_BOTTOM_BASE_X = 171
934x480:  MAP_LEFT_COL_X    = 171
934x480:  MAP_VIEW_START_X  = 441
```

Not completed in this session:

- Desktop C++ build. The build command was blocked by a temporary environment classifier outage.
- Android build.
- APK install/runtime screenshot.
- Visual check of surface and underground map alignment.

Recommended next steps:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Check at 934x480:

- History text remains inside x=0..170.
- Soldier panel does not cover the history frame.
- Surface map and sector selection remain aligned.
- Underground map/grid/icons remain aligned with the same map anchor.
- Scrollbar and history buttons remain usable.

Regression resolutions:

- 800x600: centered vanilla layout.
- 1024x600: widescreen history strip and right-anchored bottom panel.
- 1024x768 / 1280x720+: full-size Wildfire map layout.
