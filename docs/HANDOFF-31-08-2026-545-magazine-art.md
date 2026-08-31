# HANDOFF — Wildfire 5.45mm magazine artwork

## Context

- Date: 2026-08-31
- Branch: `feature/multi-edition-detector`
- Target: JA2 Wildfire on a physical Android device

Wildfire's magazine IDs are shifted relative to the vanilla externalized table. The 5.45mm rows were resolving to the wrong inventory artwork, so the item labeled `5.45mm Magazine` displayed an unrelated image.

## Fix

Updated `src/externalized/DefaultContentManager.cc`:

- Wildfire IDs `90–91` remain the 5.45mm AP/HP magazines.
- Both use the correct 5.45mm big artwork:
  - ID `90`: `bigitems/p1item09.sti`
  - ID `91`: `bigitems/p1item10.sti`
- Small inventory graphics now use the corresponding BigItems art directly, avoiding the incorrect `Mdp1Items.sti` frame mapping.
- IDs `94–95` retain the corrected 5.56mm AP magazine/box definitions.

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
- `LauncherActivity` started successfully.
- No crash observed during launch.
- Wildfire archive artwork inspected directly from `BigItems.slf`.

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
