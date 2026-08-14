# HANDOFF — 13/08/2026

## Android 934x480 — History text inset inside frame

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 target |
| Scope | Strategic-map history log text |
| Status | Code change completed; runtime verification pending |

---

## User request

History text touched the red frame. Required:

- Keep text inside the frame.
- Leave approximately 4 px between text and the frame's inner edge.
- Wrap long lines before they touch the frame.
- Keep the frame and map/roster layout unchanged.

---

## Implemented change

Changed [src/game/Utils/Message.cc](../src/game/Utils/Message.cc):

- Added `MAP_HISTORY_FRAME_BORDER = 13` for the frame's measured inner border.
- Added `MAP_HISTORY_TEXT_INSET = 17` (`13px` frame border + `4px` text padding).
- Widescreen history text now starts at `(17, 17)`.
- Widescreen clip rectangle uses the same 17 px inset on all sides.
- Widescreen `MAP_LINE_WIDTH` subtracts `2 * MAP_HISTORY_TEXT_INSET`, so `LineWrap()` breaks lines before the inner frame edge.
- Widescreen first line starts at `y=17`.
- Vanilla and full-size history geometry remains unchanged.

Changed [src/game/Strategic/Map_Screen_Interface_Bottom.cc](../src/game/Strategic/Map_Screen_Interface_Bottom.cc):

- `MapScreenLogTop()` now returns `17` for widescreen layouts, matching the text origin.
- Frame remains rendered from `(0, 0)` and keeps its original outer position.
- Scroll controls continue to use `MapScreenLogTop()`.

---

## Expected 934x480 geometry

```text
history frame: x=0..170, y=0..479
frame inner border: approximately 13 px
text inset: 17 px from outer frame
text origin: x=17, y=17
wrap width: 171 - 2*17 = 137 px
clip area: x=17..154, y=17..462
```

The exact visible right/bottom edge depends on the font renderer's clip semantics.

---

## Verification

Passed:

```bash
git diff --check -- src/game/Utils/Message.cc src/game/Strategic/Map_Screen_Interface_Bottom.cc
```

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

- Text begins visibly inside the red frame, not at the outer edge.
- Long lines wrap before touching the frame.
- Text does not overlap the soldier panel.
- Scroll arrows and scroll region remain usable.
- Surface and underground maps remain aligned.

Regression resolutions:

- 800x600: centered vanilla layout unchanged.
- 1024x600: widescreen history layout remains inset.
- 1024x768 / 1280x720+: full-size history layout unchanged.
