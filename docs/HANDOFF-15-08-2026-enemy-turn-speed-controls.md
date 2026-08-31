# Handoff — Turn speed control

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Target preset:** Android `934x480`
**Status:** Player-turn visibility and `1x`–`5x` cycling runtime verified by user

## Problem

The original enemy-turn control was difficult to use on Android. A later four-button version occasionally remained visible after the enemy turn ended.

## Current implementation

### `src/game/Tactical/Interface.cc`

- One centered button appears while any tactical top-turn message is active, including player turns.
- Tapping cycles `1x → 2x → 3x → 4x → 5x → 1x`.
- The label and tooltip update immediately; tooltip documents `F8`.
- `MSYS_PRIORITY_HIGHEST` keeps touch input above the full-screen turn lock region.
- `EndTopMessage()` hides the button.

### `src/game/Tactical/Turn_Based_Input.cc`

- `F8` performs the same `1x`–`5x` cycle while a tactical top-turn message is active.
- Handling runs before the locked-interface input guard.
- Existing `F8` behavior remains unchanged outside top-turn messages.

## Behavior preserved

- The speed value still accelerates only non-player soldier animations.
- Player animations, AI decisions, pathfinding, bullets, damage, interrupts, dialogue, and combat rules remain unchanged.
- Speed remains runtime-only and resets to `1x` each session.
- Android RMB behavior remains unchanged.
- Desktop and other resolutions retain responsive horizontal centering.

## Verification

Android `934x480` runtime verification confirmed:

- centered control and touch interaction;
- visibility during player and enemy turns;
- `1x`–`5x` cycling and wrap to `1x`.

Static check passed:

```sh
git diff --check
```

Local build was not rerun because the execution harness blocked the build command.

## Modified files

- `src/game/Tactical/Interface.cc`
- `src/game/Tactical/Turn_Based_Input.cc`
- `docs/HANDOFF-15-08-2026-enemy-turn-speed-controls.md`

## Build output

- `android/app/build/outputs/apk/debug/app-debug.apk`
