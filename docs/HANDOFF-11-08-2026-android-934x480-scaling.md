# HANDOFF — 11/08/2026

**Fix touch scaling for Android `934x480` preset**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | SDL logical viewport mapping + Android touch input |
| Status | Native build passed; live device test pending |

---

## 1. Problem

Android SDL touch events arrive as normalized surface coordinates (`0..1`). SDL renders the selected internal resolution with aspect-preserving logical scaling. On aspect-mismatched surfaces, letterbox offsets were ignored, causing taps to miss visible controls—especially with `934x480`.

Edge touches could also become exactly `SCREEN_WIDTH` / `SCREEN_HEIGHT`, outside valid pixel bounds.

---

## 2. Changes

### `src/sgp/Video.cc`

Added `VideoMapWindowToLogical()`, delegating window-pixel to logical-coordinate conversion to SDL:

```cpp
SDL_RenderWindowToLogical(GameRenderer, windowX, windowY, logicalX, logicalY);
```

### `src/sgp/Video.h`

Added the corresponding declaration.

### `src/sgp/Input.cc`

- Added Android-only `SetSafeTouchPosition(float, float)`.
- Converts normalized touch coordinates to SDL window pixels.
- Uses `VideoMapWindowToLogical()` to remove letterbox offsets and preserve aspect ratio.
- Clamps normalized input to `0..1`.
- Clamps logical coordinates to valid screen pixels.
- Reused by `FingerMove()`, `FingerDown()`, and `FingerUp()`.
- Added `PadInjectMousePos()` for controller-generated logical pointer movement.
- Changed general mouse bounds from `SCREEN_WIDTH/HEIGHT` to `SCREEN_WIDTH - 1` / `SCREEN_HEIGHT - 1`.

### `src/sgp/Input.h`

Added `PadInjectMousePos()` declaration.

### Existing behavior preserved

- `Resolution.PRESETS` unchanged.
- Tactical viewport unchanged.
- Android laptop-specific scaling unchanged.
- Renderer remains aspect-preserving; no stretch mode introduced.

---

## 3. Verification

Passed:

```sh
cd android && ./gradlew testDebugUnitTest
cd android && ./gradlew assembleDebug
git diff --check
```

`assembleDebug` result:

```text
BUILD SUCCESSFUL in 4s
```

APK:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

Initial build exposed an unrelated missing definition for `PadInjectMousePos(int, int)`; definition added in `Input.cc`, rebuild passed.

---

## 4. Live test TODO

Install APK:

```sh
cd android && ./gradlew installDebug
```

Test checklist:

1. Select `934x480`.
2. Confirm centered, aspect-correct rendering.
3. Tap visible controls at left/right game edges; confirm alignment.
4. Test bottom UI bar controls.
5. Recheck `800x600`, `1024x600`, and `1280x600`.
6. If crash occurs:

```sh
adb logcat | grep -i "fatal\|backtrace\|libja2\|SIGSEGV"
```

No commit or push created.
