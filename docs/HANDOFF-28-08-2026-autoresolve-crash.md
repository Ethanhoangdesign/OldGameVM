# HANDOFF — Autoresolve, Go To Sector, and tactical-turn crashes

Date: 2026-08-29
Branch: `feature/multi-edition-detector`

## Current result

Android debug APK rebuilt and installed. The user confirmed the crash during the enemy turn no longer occurs after the formatter fixes.

Confirmed command results:

```text
./android/gradlew -p android assembleDebug
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
Success
```

`git diff --check` passes.

## Crash history

### Autoresolve native crash

Original native crash chain:

```text
AutoResolveScreenHandle()
CreateAutoResolveInterface()
MakeEnemyTroops()
```

`TacticalCreateEnemySoldier()` returned `nullptr` while Autoresolve reserved an existing tactical enemy that was absent. `MakeEnemyTroops()` dereferenced the null pointer.

Fixes:

- [Soldier_Create.cc](../src/game/Tactical/Soldier_Create.cc): create a strategic-only soldier when no tactical slot is available; preserve/log Autoresolve creation exceptions.
- [Auto_Resolve.cc](../src/game/Strategic/Auto_Resolve.cc): ignore stale Autoresolve requests for sectors without hostiles.
- [PreBattle_Interface.cc](../src/game/Strategic/PreBattle_Interface.cc): ignore stale PBI locators without opposition.

### Wildfire default-ammo mismatch

Wildfire item IDs differ from vanilla externalized definitions:

- Item `5`: Colt 1991, `AMMO45`, capacity `7`.
- Item `56`: MP7, `AMMO46`, capacity `20`.

Fixes:

- [DefaultContentManager.cc](../src/externalized/DefaultContentManager.cc): correct Wildfire weapon metadata.
- [Items.cc](../src/game/Tactical/Items.cc): include gun ID/name/calibre/capacity in missing-ammo errors.

### Tactical enemy-turn formatter crash

Crash report:

```text
kind=cpp_exception
message=Game has been terminated due to an unrecoverable error: Unexpected character in format string
```

The String Theory formatter rejects unsupported format syntax. The crash happened immediately after tactical AI cover evaluation:

```text
AI_TIMING cover_deep ...
Game has been terminated due to an unrecoverable error: Unexpected character in format string
```

Fixed malformed strings:

- [FindLocations.cc:72](../src/game/TacticalAI/FindLocations.cc#L72): `%Better` → `percent better`.
- [FindLocations.cc:1045](../src/game/TacticalAI/FindLocations.cc#L1045): `%Better` → `percent better`.
- [DecideAction.cc:3419](../src/game/TacticalAI/DecideAction.cc#L3419): `{}% better` → `{} percent better`.
- [LOS.cc:2943](../src/game/Tactical/LOS.cc#L2943): `{:.1f}` → `{.1f}`; String Theory does not accept the colon syntax.
- [Structure.cc:1384](../src/game/TileEngine/Structure.cc#L1384): remove one extra closing brace.

User retest after rebuild/install: enemy-turn crash no longer reproduced.

## Go To Sector state fix

Original behavior:

- PBI opened.
- `Go To Sector` showed no enemy soldiers.
- Returning to the map removed the battle warning.
- Sector showed `?` and allowed ordinary squad selection while strategic enemies remained.

Diagnostic state:

```text
InitPBI: group=false persistent=false world=D13 locator=1 pbi_sector=D13 encounter=1 strategic_hostiles=13
GoToSector: target=D13 world=D13 persistent=0 encounter=1 locator=1 group=true
SetCurrentWorldSector same-sector: tactical_enemies_before=0 strategic_enemies=13
PrepareEnemy: world=D13 persistent=0 map_settings=false counts=0/0/0 in_battle=0/0/0
SetCurrentWorldSector same-sector: tactical_enemies_after=0 strategic_enemies=13
```

Source changes:

- [PreBattle_Interface.h](../src/game/Strategic/PreBattle_Interface.h) and [PreBattle_Interface.cc](../src/game/Strategic/PreBattle_Interface.cc): `gubExplicitEnemyEncounterCode` changed from `BOOLEAN` to `UINT8`; codes `8–10` no longer truncate.
- [SaveLoadGame.cc](../src/game/SaveLoadGame.cc): use `INJ_U8`/`EXTR_U8`; on-disk size remains one byte.
- [PreBattle_Interface.cc](../src/game/Strategic/PreBattle_Interface.cc): load the normalized surface target in `GoToSectorCallback()`; restore the persistent locator after loading.
- [PreBattle_Interface.cc](../src/game/Strategic/PreBattle_Interface.cc): recalculate non-persistent hostility after live state changes; clear the locator only when the current sector has no opposition.
- [StrategicMap.cc](../src/game/Strategic/StrategicMap.cc): retain the locator during world teardown while army hostiles, hostile civilians, or bloodcats remain.

Temporary diagnostics were removed.

Go To Sector still needs a clean end-to-end gameplay confirmation after the formatter crash fix:

```text
PBI → Go To Sector → verify enemies appear → return to map → verify locator remains → click locator → verify PBI reopens
```

## Existing related fixes

- Tactical video-object/surface teardown ordering fixed.
- Wrapped SDL surfaces validated before blitting.
- Wildfire item-art and magazine corrections applied.
- Autoresolve stale-state guards applied.

## Android tracking

Package:

```text
io.github.ja2stracciatella
```

Game-private files:

```text
files/ja2.log
files/ja2.log.last
files/.ja2/crash-report
files/.ja2/crash-signal
files/.ja2/SavedGames
```

After a crash, copy logs before launching again:

```sh
PKG=io.github.ja2stracciatella
adb exec-out run-as "$PKG" cat files/ja2.log > crash-ja2.log
adb exec-out run-as "$PKG" cat files/.ja2/crash-report > crash-report.txt
```

Do not run `adb shell pm clear` without a verified backup. The previous `pm clear` deleted the old Android save; Android Backup Manager had no restore set.

Backup before destructive operations:

```sh
PKG=io.github.ja2stracciatella
adb exec-out run-as "$PKG" tar -C files -czf - .ja2 > "ja2-save-backup-$(date +%Y%m%d-%H%M%S).tar.gz"
```

## Verification checklist

1. `git diff --check`.
2. `./android/gradlew -p android assembleDebug`.
3. `adb install -r android/app/build/outputs/apk/debug/app-debug.apk`.
4. New-game or verified save test: enemy turn does not crash.
5. Go To Sector test: tactical enemies appear and map locator persists.
6. Cleared-sector test: locator disappears only after all opposition is gone.
7. Capture `ja2.log` immediately if any failure returns.
