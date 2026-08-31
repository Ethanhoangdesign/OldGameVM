---
name: enemy-turn-speed
description: In-game 1x/2x/3x/4x enemy animation speed toggle, visible during opponent turn
metadata:
  type: project
---

## Problem

Enemy turns feel slow, especially on Android, because every non-player soldier
still plays full-speed animations. There was no way to fast-forward them at
runtime without changing game settings or restarting.

## Feature Added

A **speed-cycle button** appears on the top-bar during `OPPONENTS' TURN` (and
militia/air-raid/interrupt turns). Each tap cycles:

```
1x → 2x → 3x → 4x → 1x
```

- Only non-player (enemy / militia / civilian AI) animations are sped up.
- Player soldier animations, AI decision-making, pathfinding, bullets, combat
  rules, interrupts — all unchanged.
- Resets to 1x each launch/session (runtime-only, no savegame or settings
  migration).
- Works on both desktop and Android (button is in the top-bar, visible when the
  team panel is hidden during opponent turn).

## Files Changed

| File | What changed |
|---|---|
| `src/game/Tactical/Soldier_Control.h` | Replaced `extern BOOLEAN gfFastEnemyTurnAnimations` with `extern UINT8 gubEnemyTurnAnimationSpeed` |
| `src/game/Tactical/Soldier_Control.cc` | Define `gubEnemyTurnAnimationSpeed = 1`; apply divisor in `CalculateSoldierAniSpeed()` for non-player soldiers in combat |
| `src/game/Tactical/Interface.cc` | Create `gEnemyTurnSpeedButton` (text button, top-right of top-bar); add `UpdateEnemyTurnSpeedButton()` / `EnemyTurnSpeedCallback()`; show/hide with `HandleTopMessages()` / `EndTopMessage()` |
| `src/game/Tactical/Interface_Panels.h` | Removed `TEAM_FAST_ENEMY_BUTTON` from team panel enum (feature moved to top-bar) |
| `src/game/Tactical/Interface_Panels.cc` | Removed old team-panel fast-enemy button, images, callback, and related enable/disable calls |

## Animation Delay Logic

In `CalculateSoldierAniSpeed()`:

```cpp
if (gubEnemyTurnAnimationSpeed > 1 && pSoldier->bTeam != OUR_TEAM)
{
    pSoldier->sAniDelay = std::max<INT16>(1, pSoldier->sAniDelay / gubEnemyTurnAnimationSpeed);
}
```

The `/2` combat speedup that already existed runs first; this divides the result
further. Clamped to 1 to prevent zero-delay edge cases.

## How to Test

1. Build: `cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)`
2. Load a tactical combat save.
3. End player turn. The `OPPONENTS' TURN` bar appears.
4. A small `1x` button appears at top-right of the bar.
5. Tap: cycles `1x → 2x → 3x → 4x → 1x`.
6. Confirm enemy animations visibly faster at 2x/3x/4x.
7. Confirm player animations unchanged.
8. Confirm interrupts, bullets, damage, dialogue still complete normally.
9. Confirm button disappears when player turn begins.
10. Confirm speed resets to 1x between sessions.

## Android Build

```sh
./tools/build-android-debug.sh
```

Button is in the top-bar, so it appears on both phone and desktop without any
separate mobile-specific code.
