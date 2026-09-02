# HANDOFF — Wildfire 5.45mm magazine artwork

## Context

- Date: 2026-08-31
- Branch: `feature/multi-edition-detector`
- Target: JA2 Wildfire on a physical Android device

Wildfire's magazine IDs are shifted relative to the vanilla externalized table. The 5.45mm and 5.56mm rows were briefly duplicated by an incorrect fixup, so both labels displayed the same-looking 5.45mm icon.

## Fix

Updated `src/externalized/DefaultContentManager.cc`:

- Wildfire IDs `90–91` remain the 5.45mm AP/HP magazines.
- Both use the correct 5.45mm big artwork:
  - ID `90`: `bigitems/p1item09.sti`
  - ID `91`: `bigitems/p1item10.sti`
- Small inventory graphics now use the corresponding BigItems art directly, avoiding the incorrect `Mdp1Items.sti` frame mapping.
- IDs `92–95` use the corrected 5.56mm definitions:
  - ID `92`: `5.56mm Magazine`, AP → `bigitems/p1item29.sti`
  - ID `93`: `5.56mm Box, 100 AP` → `bigitems/p1item29.sti`
  - ID `94`: `5.56mm Magazine, HP` → `bigitems/p1item30.sti`
  - ID `95`: `5.56mm Box, 100 HP` → `bigitems/p1item30.sti`
- 5.45mm and 5.56mm no longer share the same artwork mapping.

Updated `src/game/Tactical/Interface_Items.cc`:

- Wildfire magazine small-art handling covers IDs `90–95`.
- Direct transparent scaling prevents oversized or unrelated artwork in inventory slots.
- Other editions and non-Wildfire items keep the existing renderer.

Updated `src/externalized/DefaultContentManager_unittests.cc`:

- Regression expectations cover the corrected 5.45mm and 5.56mm definitions and artwork paths.

## Verification

- `git diff --check`: passed.
- Desktop `ja2` build: passed (`Built target ja2`).
- Android debug build: passed (`BUILD SUCCESSFUL`).
- APK installed over USB: `Success`.
- `LauncherActivity` started successfully on the connected Android device.
- No `FATAL EXCEPTION`, `SIGSEGV`, or `AndroidRuntime` crash in launch log.

## Follow-up: F8 Cambria underground crash

- Device `crash-report` captured `kind=cpp_exception` with `Unexpected character in format string`.
- `ja2.log.last` immediately showed `Tactical_Save.cc` failing to find `uiTimeCurrentSectorWasLastLoaded`, followed by the `Queen_Command.cc:540` underground-sector assertion.
- F8 is Cambria hospital on the surface; Wildfire also ships `f8_b1.dat`, but the externalized underground list had no F8 level-1 node.
- Added the Wildfire F8 hospital basement node. Preserved H8/H9 as Cambria mine sectors.
- Removed the underground battle assertion crash path; missing nodes now log and return safely.
- Corrected the earlier PBI transition path to pass the normalized surface sector into `SetCurrentWorldSector()`.
- Save backup created before testing. Android rebuild/install completed after this follow-up; LauncherActivity and StracciatellaActivity remained running.
- Added save-load migration that seeds current underground definitions before loading serialized records, then appends missing definitions such as F8_B1.
- Added guards for missing underground records during floor entry, world loading, PBI calculation, battle preparation, battle cleanup, and enemy-death accounting.
- Fresh post-install log had no new F8-1 assertion or formatter-crash entry; historical entries remain in `ja2.log`.
- Reproduced save crash in F8_B1: `Tactical_Save.cc:332` asserted because the Wildfire F8_B1 map has no entry points. Replaced that assertion with a warning and skipped only the reachable-item scan; other save data continues.
- Rebuilt and reinstalled Android APK after the save fix; process remained alive.
- User confirmed saving inside F8_B1 now completes without crashing.
- Save backup created before testing: `/tmp/ja2-save-backup-20260902.tar.gz`.

## Save crash follow-up

The F8_B1 map is valid for loading but has no tactical entry point. Saving invoked `HandleAllReachAbleItemsInTheSector()`, whose assertion treated that map layout as fatal. The fix logs and returns when no fallback entry grid exists, preserving the rest of the save path.

Affected file:

- [Tactical_Save.cc](../src/game/Tactical/Tactical_Save.cc): skip reachable-item scan for entry-point-less maps instead of aborting.

Verification:

- Android debug build: passed.
- APK install: `Success`.
- Android game process remained running after launch.
- Existing assertion only appears in historical log records.
- User confirmed the F8_B1 save completes without crashing after the fix.

No known follow-up remains for the reported F8_B1 save crash.

## Scope

- No release APK generated.
- CTest has no registered tests in the current build tree.
- No changes to magazine gameplay semantics, capacities, or exploration logic beyond the listed Wildfire metadata corrections.
- No changes to the F8_B1 map binary.

## Working tree

- Changes committed and pushed on `feature/multi-edition-detector`.
- Do not run `adb shell pm clear` without a verified save backup.
- Wildfire archive artwork inspected directly from `BigItems.slf`.
- User confirmed the corrected Android build was running.

## Build/install

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella" && \
./tools/build-android-debug.sh && \
adb install -r android/app/build/outputs/apk/debug/app-debug.apk && \
adb shell am force-stop io.github.ja2stracciatella && \
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

## Remaining scope

- No release APK generated.
- CTest has no registered tests in the current build tree.
- No changes to magazine gameplay semantics, capacities, or exploration logic beyond the listed Wildfire metadata corrections.
