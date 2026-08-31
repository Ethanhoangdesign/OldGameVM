# Handoff — Android laptop background and turn-speed button

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Target preset:** Android `934x480`
**Status:** Runtime visual verification passed by user

## Problem

When opening the laptop from the strategic map, the old History log and strategic map remained visible beside the laptop frame. The SDL renderer clear alone did not solve this because the laptop fit-scale source still came from the shared `934x480` framebuffer containing the previous map screen.

The tactical enemy-animation speed button also remained visible outside tactical enemy turns, including while the laptop was open.

## Implemented changes

### Laptop framebuffer background

Changed [src/game/Laptop/Laptop.cc](../src/game/Laptop/Laptop.cc).

`RenderLapTopImage()` now clears the Android framebuffer with `FRAME_BUFFER->Fill(0)` before drawing the laptop frame/background. This removes stale History/map pixels from the source texture before Android aspect-fit presentation.

Changed [src/sgp/Video.cc](../src/sgp/Video.cc).

`RefreshScreen()` clears the SDL renderer to opaque black before copying the texture. This keeps pillarbox/letterbox regions black outside the laptop destination rectangle.

### Enemy turn-speed button visibility

Changed [src/game/Tactical/Interface.cc](../src/game/Tactical/Interface.cc).

`UpdateEnemyTurnSpeedButton()` now shows the button only when:

- `guiCurrentScreen == GAME_SCREEN`; and
- a top message is active; and
- the top message is `COMPUTER_TURN_MESSAGE`, `COMPUTER_INTERRUPT_MESSAGE`, `MILITIA_INTERRUPT_MESSAGE`, or `AIR_RAID_TURN_MESSAGE`.

The button hides during player turns, strategic map, laptop, options, and other screens. Existing `1x`–`5x` cycling and `F8` behavior remain unchanged.

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

Build completed successfully. Existing warning remains:

```text
src/game/Tactical/Interface.cc:1787:10: warning: variable 'fTileBar' set but not used [-Wunused-but-set-variable]
```

User runtime confirmation:

- Laptop now has black background covering the old History/map areas.
- Speed button no longer appears outside tactical enemy turns.

Recommended Android follow-up:

```bash
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Regression targets:

- Android `934x480`: open laptop from map; verify all outside regions stay black.
- Laptop E-mail, Web, Files, History, help, and message box.
- Tactical enemy, militia-interrupt, air-raid, and player turns.
- Strategic map, laptop, options, and laptop exit/re-entry.
