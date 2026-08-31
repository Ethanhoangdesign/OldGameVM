# HANDOFF — 13/08/2026

## Android 934x480 — History wrapping and inset

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 target |
| Scope | Strategic-map history log text |
| Status | Code change completed; runtime verification pending |

---

## User request

History text was too close to the frame and overflowed into the soldier area. Required:

- Wrap long history messages.
- Keep approximately 8 px from the frame.
- Use 8 px top margin.
- Keep the history aligned near the upper-left corner.
- Do not disturb map or underground-map alignment.

---

## Implemented change

Changed [src/game/Utils/Message.cc](../src/game/Utils/Message.cc):

- Added `MAP_HISTORY_MARGIN = 8`.
- Widescreen `MAP_LINE_WIDTH` now uses `BASE_X - 16`, preserving 8 px left/right insets.
- Widescreen history text rectangle now starts at `(8, 8)`.
- Widescreen text clip height uses `SCREEN_HEIGHT - 16`, preserving 8 px bottom inset.
- Widescreen first text line starts at `y=8` instead of the previous `MapScreenLogTop() + 11` offset.
- Text draw x-coordinate changed from `lx + 3` to `lx`.
- Existing `LineWrap()` path remains responsible for line wrapping.
- Vanilla and full-size history geometry remains unchanged.

No map anchor, soldier-column coordinate, surface-map coordinate, or underground-map coordinate changed in this step.

---

## Expected 934x480 geometry

```text
history frame: x=0..170, y=0..479
text area:     x=8..162,  y=8..471
wrap width:    154 px
first line:    x=8, y=8
```

The exact right edge depends on the font renderer's clip semantics, but the configured rectangle keeps the intended 8 px inset.

---

## Verification

Passed:

```bash
git diff --check -- src/game/Utils/Message.cc src/game/Strategic/Map_Screen_Interface_Bottom.cc
```

Desktop build was attempted with:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
```

The command was blocked by a temporary environment classifier outage. No compiler result available.

Not completed:

- Desktop build confirmation.
- Android build.
- APK install/runtime screenshot.
- Visual check at 934x480.
- Surface/underground map regression check.

---

## Recommended next steps

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Check at 934x480:

- Long history lines wrap inside the left strip.
- First line begins 8 px from the top and left frame edges.
- Text does not touch or cross the frame.
- Text does not overlap the soldier panel.
- Scroll arrows and scroll region remain usable.
- Surface and underground maps remain aligned.

Regression resolutions:

- 800x600: centered vanilla layout unchanged.
- 1024x600: widescreen history layout remains inset.
- 1024x768 / 1280x720+: full-size history layout unchanged.
