# Handoff — Android Insurance popup final

**Date:** 2026-08-15  
**Branch:** `feature/multi-edition-detector`  
**Device:** Android physical device `R5GL31H83QX` (`SM_A175F`)  
**Status:** fixed, user verified OK dismisses popup

## Problem

Laptop → Insurance → `enter/review an insurance contract` showed an OK-only popup when no mercs were eligible for insurance.

On Android the OK button visibly depressed, but the popup appeared to require many taps before leaving.

## Root cause

Input did reach the message box button. Runtime trace showed repeated:

- `finger_down`
- `mouse_hook`
- `msgbox_button`
- `msgbox_handled_down ret=1`

`gMsgBox.bHandled` was set, and the message box did close. The apparent failure was caused by `RenderInsuranceContract()` immediately creating the same “no qualified mercs” message box again every render while `count_insurance_grids == 0`.

## Fix

### `src/game/Laptop/Insurance_Contract.cc`

Added `gfNoMercsPopupShown`.

- Reset it in `EnterInsuranceContract()`.
- Set it after the no-mercs popup is shown.
- Gate the popup on `count_insurance_grids == 0 && !gfNoMercsPopupShown`.

Result: popup appears once per visit to the insurance contract page; after OK, user lands on the underlying Insurance contract page instead of instantly reopening the same popup.

### `src/game/MessageBoxScreen.cc`

Kept Android OK-only fallback:

- For `MSG_BOX_FLAG_OK` on Android, `MSYS_CALLBACK_REASON_POINTER_DWN` sets `gMsgBox.bHandled`.
- Other message box types still use pointer-up behavior.

This avoids touch-up/anchor fragility for one-choice mobile popups without changing YES/NO semantics.

### Input/rendering changes retained

Retained previous Android input/presentation fixes:

- `InputAtom` snapshots pointer coordinates.
- `GameLoop()` dispatches queued coordinates into `MouseSystemHook()`.
- `MouseSystemHook()` updates logical mouse position from queued event before processing.
- Laptop fit scale is disabled while `gfInMsgBox`.
- Android laptop framebuffer/render clears black to avoid stale stretch/letterbox artifacts.
- Enemy turn-speed button only shows for relevant top-message types.

## Trace cleanup

Removed temporary `OGVM-POPUP` tracing from:

- `android/app/src/main/java/org/libsdl/app/SDLActivity.java`
- `src/sgp/Input.cc`
- `src/sgp/MouseSystem.cc`
- `src/game/MessageBoxScreen.cc`

Verified no remaining `OGVM-POPUP` or `SDL_LogWarn` in touched trace files.

## Verification

Completed:

```bash
git diff --check
./tools/build-android-debug.sh
```

Build result:

```text
BUILD SUCCESSFUL in 4s
```

Runtime:

- User installed latest APK.
- User tested Insurance popup on Android.
- User confirmed: `ok được rổi ấy`.

## Notes

`android/build.log` and root `build.log` are generated build artifacts; do not rely on them as source. Project instruction still applies: never read full `android/build.log` into context.
