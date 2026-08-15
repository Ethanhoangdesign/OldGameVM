# Handoff — Android Insurance popup dismissal + laptop background + turn-speed button

**Date:** 2026-08-15  
**Branch:** `feature/multi-edition-detector`  
**Target preset:** Android `934x480`  
**Status:** Runtime visual verification passed by user

## Problem summary

1. **Insurance popup dismissal**: Opening Laptop → Insurance → "enter/review an insurance contract" with no eligible mercs shows the expected popup. Tapping visible `OK` did not dismiss it reliably; user reported approximately 10–15 taps required.

2. **Laptop background**: When opening the laptop from the strategic map, the old History log and strategic map remained visible beside the laptop frame. The SDL renderer clear alone did not solve this because the laptop fit-scale source still came from the shared `934x480` framebuffer containing the previous map screen.

3. **Turn-speed button visibility**: The tactical enemy-animation speed button remained visible outside tactical enemy turns, including while the laptop was open.

## Root cause (Insurance popup)

The first fix was incomplete. Moving `GetMousePos()` inside each dequeue loop still read global pointer state. `InputAtom` stored only event type/button, while SDL touch handlers updated that global position before `GameLoop()` drained the queue. Multiple events queued in one frame could therefore all be dispatched with the last event's position.

That breaks the button anchor path: touch `DOWN` may anchor a different region, then touch `UP` is rejected instead of delivering `MSYS_CALLBACK_REASON_POINTER_UP` to `OK`.

A second issue compounded the mismatch: Android Laptop fit-scale mapping remained active while the MessageBox was modal.

## Implemented changes

### 1. Per-event pointer coordinates (Insurance popup fix)

Changed [src/sgp/Input.h](../src/sgp/Input.h), [src/sgp/Input.cc](../src/sgp/Input.cc), and [src/game/GameLoop.cc](../src/game/GameLoop.cc).

`QueuePointerEvent()` now snapshots the mapped/clamped pointer coordinates into each `InputAtom`. `GameLoop()` passes those stored coordinates to `MouseSystemHook()`. Every `DOWN`, `UP`, move, repeat, and mouse event keeps the position it had when queued.

The superseded `GetMousePos()`-during-dequeue workaround was removed.

### 2. Disable Laptop fit-scale while modal

Changed [src/game/Laptop/Laptop.cc](../src/game/Laptop/Laptop.cc).

`AndroidLaptopScaleActive()` now returns false while `gfInMsgBox` is true. MessageBox rendering and touch hit-testing therefore use the same full-screen logical coordinate space.

### 3. Guard Android input remapping

Changed [src/sgp/Input.cc](../src/sgp/Input.cc).

`SetSafeMousePosition()` applies Laptop screen-to-logical mapping only when:
- `gfInMsgBox == false`;
- `guiCurrentScreen == LAPTOP_SCREEN`; and
- Android Laptop fit-scale is active.

Desktop code remains unchanged through `#ifdef __ANDROID__` guards.

### 4. Laptop framebuffer background

Changed [src/game/Laptop/Laptop.cc](../src/game/Laptop/Laptop.cc).

`RenderLapTopImage()` now clears the Android framebuffer with `FRAME_BUFFER->Fill(0)` before drawing the laptop frame/background. This removes stale History/map pixels from the source texture before Android aspect-fit presentation.

Changed [src/sgp/Video.cc](../src/sgp/Video.cc).

`RefreshScreen()` clears the SDL renderer to opaque black before copying the texture. This keeps pillarbox/letterbox regions black outside the laptop destination rectangle.

### 5. Enemy turn-speed button visibility

Changed [src/game/Tactical/Interface.cc](../src/game/Tactical/Interface.cc).

`UpdateEnemyTurnSpeedButton()` now shows the button only when:
- `guiCurrentScreen == GAME_SCREEN`; and
- a top message is active; and
- the top message is `COMPUTER_TURN_MESSAGE`, `COMPUTER_INTERRUPT_MESSAGE`, `MILITIA_INTERRUPT_MESSAGE`, or `AIR_RAID_TURN_MESSAGE`.

The button hides during player turns, strategic map, laptop, options, and other screens. Existing `1x`–`5x` cycling and `F8` behavior remain unchanged.

## Existing callback path preserved (Insurance)

No Insurance-specific dismissal workaround was added.

- [src/game/Laptop/Insurance_Contract.cc](../src/game/Laptop/Insurance_Contract.cc) still opens the popup with `DoLapTopMessageBox()`.
- `InsContractNoMercsPopupCallBack()` still sets `guiCurrentLaptopMode = LAPTOP_MODE_INSURANCE` after `MSG_BOX_RETURN_OK`.
- `MessageBoxScreenHandle()` remains the sole popup cleanup/exit path.

## Behavior preserved

- Laptop frame, pages, scaling, cursor, touch mapping, and transitions unchanged.
- Strategic map and History render normally after closing the laptop.
- Speed value still affects enemy animations only.
- Speed button remains centered and cycles `1x → 2x → 3x → 4x → 5x → 1x`.

## Verification

Passed:

```bash
git diff --check
cmake --build build -j2
```

Build output was filtered according to `CLAUDE.md`; no error or warning diagnostics remained in the filtered output.

Not yet confirmed:
- Android runtime one-tap dismissal on the target device.
- Touch-up after slight finger movement.
- Other Laptop-originated MessageBoxes.

## Android follow-up

```bash
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Test sequence:
1. Open Laptop.
2. Open Insurance.
3. Select "enter/review an insurance contract".
4. Tap visible `OK` once.
5. Confirm popup closes immediately and Insurance home appears.
6. Repeat five times.
7. Test another Laptop MessageBox.
8. Verify laptop background is black (no History/map bleed).
9. Verify turn-speed button only appears during enemy/militia/air-raid turns.

## Working-tree note

Existing unrelated changes remain untouched:

- `src/game/Laptop/Laptop.cc` also contains the Android laptop framebuffer background change.
- `src/game/Tactical/Interface.cc` contains enemy turn-speed button changes.
- `src/sgp/Video.cc` contains Android black renderer-clear changes.
- `docs/HANDOFF-15-08-2026-android-laptop-black-background-speed-button.md` is an earlier handoff.

Do not commit or revert these changes without explicit instruction.