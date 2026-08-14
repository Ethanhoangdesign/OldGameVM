# HANDOFF — 14/08/2026

## Android 934x480 — History text wrapping completed

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 target |
| Scope | Strategic-map history log text wrapping and clipping |
| Status | **Runtime visual verification passed by user** |

---

## Problem

At 934x480, history text was inside the frame but long lines lost their suffixes:

```text
SHADOW: "WE KNOW THEY'R
SHADOW: "WE'VE GOT COMP
```

The renderer clipped entries that had been wrapped for a wider viewport. The required behavior is word wrapping onto the next history row, not truncation.

---

## Implemented change

Changed [src/game/Utils/Message.cc](../src/game/Utils/Message.cc).

### Shared wrapping path

Added `AddWrappedMapScreenMessage()`.

- Uses existing `LineWrap(MAP_SCREEN_MESSAGE_FONT, MAP_LINE_WIDTH, ...)`.
- New messages wrap at the current history width.
- `fBeginningOfNewString` remains set only on the first generated line.
- Existing color and queue/scroll behavior preserved.

### Load-time rewrapping

`LoadMapScreenMessagesFromSaveGameFile()` now:

1. Reads serialized history entries into a temporary array.
2. Clears the current display ring.
3. Rebuilds valid entries through `AddWrappedMapScreenMessage()`.
4. Recalculates the display position with `MoveToEndOfMapScreenMessageList()`.

Old saves therefore use the current 934x480 width instead of retaining stale wider wrapping.

### Geometry retained

- `GetMapHistoryTextBounds()` remains the single source for widescreen wrap width and font clipping.
- Frame, scrollbar buttons, slider, wheel behavior, map, roster unchanged.
- `WordWrap.cc` unchanged.
- Vanilla/full-size layout formulas unchanged.

---

## Expected 934x480 behavior

```text
SHADOW: "WE KNOW
THEY'RE
```

Long messages continue on subsequent rows. No suffix disappears at the right edge. Text remains inside the history frame and clear of the scrollbar artwork.

---

## Verification

Passed:

```bash
git diff --check -- src/game/Utils/Message.cc
```

Runtime visual verification:

- User confirmed the history text now wraps correctly.
- Long lines no longer lose their suffixes.
- Text remains inside the frame.

Not completed in this session:

- Native C++ build. Build command was blocked by a temporary environment classifier outage.
- Android APK rebuild/install after the final load-time rewrap change.
- Regression runtime checks at 800x600 and 1024x768/1280x720+.

Recommended follow-up:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Regression targets:

- 934x480: new message wrapping, old-save loading, up/down scrolling, slider, wheel.
- 1024x600: widescreen history remains inside frame.
- 800x600 / 640x480: centered vanilla layout unchanged.
- 1024x768 / 1280x720+: full-size Wildfire history unchanged.
