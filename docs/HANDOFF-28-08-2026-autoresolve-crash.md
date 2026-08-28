# HANDOFF — Autoresolve native crash fixed

Date: 2026-08-28
Branch: `feature/multi-edition-detector`

## Result

Android Autoresolve no longer exits with a native SIGSEGV. The user rebuilt, installed, and confirmed the fix on Samsung `SM-A175F` / Android 16.

## Crash chain

The final native stack was:

```text
AutoResolveScreenHandle()
CreateAutoResolveInterface()
MakeEnemyTroops()
```

`TacticalCreateEnemySoldier()` returned `nullptr` when Autoresolve tried to reserve an existing tactical enemy that was not present. `MakeEnemyTroops()` then dereferenced the null pointer at `s->ubBodyType`.

The earlier reported exception was separate and also fixed:

```text
Found no default ammo for gun 5 (SW38, calibre=AMMO38, capacity=6)
```

Wildfire item ID `5` is Colt 1991, while the vanilla externalized profile was `SW38`. Wildfire item ID `56` is MP7 and needs its dedicated 4.6mm metadata.

## Source fixes

- `src/game/Tactical/Soldier_Create.cc`
  - If no tactical soldier can be reserved for Autoresolve, create a strategic-only soldier instead.
  - Preserve and log creation exceptions during Autoresolve instead of returning a null soldier.
- `src/externalized/DefaultContentManager.cc`
  - Wildfire weapon ID `5` → `AMMO45`, capacity `7`.
  - Wildfire weapon ID `56` → `AMMO46`, capacity `20`.
- `src/game/Tactical/Items.cc`
  - Missing default-ammo errors now include gun ID, name, calibre, and capacity.
- `src/game/Strategic/Auto_Resolve.cc`
  - Ignore stale Autoresolve requests for sectors with no hostiles.
- `src/game/Strategic/PreBattle_Interface.cc`
  - Ignore stale pre-battle locators for sectors with no hostiles.
- `src/game/TileEngine/SysUtil.cc`
  - Added paired teardown for tactical save/extra video surfaces.
- `src/game/Tactical/Overhead.cc`
  - Tear down tactical video objects before the generic video-surface manager.
- `src/game/TileEngine/SysUtil.h`
  - Declared the tactical video-object teardown function.
- `src/sgp/VSurface.cc`
  - Validate wrapped SDL surfaces before `BltVideoSurface()` accesses them.

## Verification

- `git diff --check` passed.
- Android debug APK built successfully.
- APK installed successfully with `adb install -r`.
- Autoresolve retest completed without the previous crash.
- Earlier historical crashes remain in Android DropBox; they are not new failures:
  - MediaTek `RenderThread` graphics crash.
  - Old `BltVideoSurface()` `SDLThread` crash.

## Build and install

```sh
./android/gradlew -p android assembleDebug
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

## Notes

Do not treat historical `dumpsys dropbox` entries as current crashes. For a fresh native test, clear logcat before reproducing and inspect the current PID/timestamp.
