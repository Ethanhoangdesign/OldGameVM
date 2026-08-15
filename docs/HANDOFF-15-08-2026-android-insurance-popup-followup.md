# Handoff — Android Insurance popup follow-up

**Date:** 2026-08-15  
**Branch:** `feature/multi-edition-detector`  
**Target:** physical Android device `R5GL31H83QX` (`SM_A175F`)  
**Status:** input fix completed, desktop/Android builds passed, APK installed and launched; one-tap runtime retest pending user

## Trigger

Prior build did **not** fix Insurance popup dismissal. User reported the visible `OK` button still required about **16 taps** to close. Holding it visibly depresses the button every time.

This narrows failure scope:

- Touch-down reaches the visible button/hit region.
- Existing Insurance callback is not the problem.
- Failure is between repeated touch events, anchored button state, and touch-up delivery.

## Existing changes retained

From [HANDOFF-15-08-2026-android-insurance-popup-dismissal.md](HANDOFF-15-08-2026-android-insurance-popup-dismissal.md):

- `InputAtom` snapshots pointer coordinates.
- `GameLoop()` dispatches queued pointer coordinates.
- Laptop fit-scale disables while `gfInMsgBox`.
- Laptop mapping is bypassed while modal.
- Android laptop framebuffer/pillarbox black clears.
- Enemy turn-speed button visibility guard.

Do not revert unrelated Laptop/Video/turn-speed work.

## New change

Changed [src/sgp/Input.cc](../src/sgp/Input.cc).

SDL declares `SDL_MOUSE_TOUCHID` as a macro, not a preprocessor symbol. The old code used:

```cpp
#ifdef SDL_MOUSE_TOUCHID
if (event->touchId == SDL_MOUSE_TOUCHID) return;
#endif
```

Therefore its guard was omitted at compile time. The code now always filters those synthetic touch events.

`FingerDown()` also ignores another `DOWN` while `gMainFingerState` is already down. `FingerUp()` resets `gMainFingerId` after queueing the up event. This prevents duplicate/interleaved down sequences from re-anchoring a button before the real touch-up is processed.

Queued pointer events now retain their own coordinates in `InputAtom`; `GameLoop()` passes those coordinates into `MouseSystemHook()`. `MouseSystemHook()` calls `SetSafeMousePositionLogical(x, y)` before `ReleaseAnchorMode()`, ensuring anchor release and the `POINTER_UP` callback evaluate the same position as the queued touch-up rather than a later global position.

No Insurance-specific logic added. Existing MessageBox `MSYS_CALLBACK_REASON_POINTER_UP` and `InsContractNoMercsPopupCallBack()` remain unchanged.

## Verification completed

```bash
git diff --check
cmake --build build -j2
./tools/build-android-debug.sh
adb -s R5GL31H83QX install -r android/app/build/outputs/apk/debug/app-debug.apk
adb -s R5GL31H83QX shell am force-stop io.github.ja2stracciatella
adb -s R5GL31H83QX shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

- `git diff --check`: clean.
- Desktop build: passed; no filtered errors/warnings.
- Android build: `BUILD SUCCESSFUL`.
- APK install: `Success`.
- App restart: launched `LauncherActivity`.
- APK: `android/app/build/outputs/apk/debug/app-debug.apk`.
- Device: `R5GL31H83QX`.

## Required runtime retest

1. Open Laptop → Insurance → `enter/review an insurance contract`.
2. Tap visible `OK` once.
3. Confirm immediate close to Insurance home.
4. Reopen popup and repeat 10 times.
5. Repeat with a slight finger move before release.
6. Test one other Laptop-originated MessageBox.
7. Confirm physical mouse/touchpad click still dismisses once, no duplicate callback.

## Current working tree

Modified tracked files include pre-existing unrelated work:

- `CLAUDE.md`
- `android/build.log`
- `src/game/GameLoop.cc`
- `src/game/Laptop/Laptop.cc`
- `src/game/Tactical/Interface.cc`
- `src/sgp/Input.cc`
- `src/sgp/Input.h`
- `src/sgp/MouseSystem.cc`
- `src/sgp/Video.cc`

Untracked:

- `build.log`
- prior handoff docs

Do not commit, revert, or clean working-tree files without explicit instruction.
