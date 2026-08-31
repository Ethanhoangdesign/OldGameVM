# HANDOFF — Drassen head miner fallback

Date: 2026-08-30
Branch: `feature/multi-edition-detector`

## Result

Fixed missing Drassen head miner when a map lacks a matching detailed civilian profile slot. User confirmed the head miner appears after installing the new Android APK.

Canonical locations remain unchanged:

- Drassen airport: `B13`
- Drassen mine entrance: `D13`
- Underground mine: `D13_B1`

The head miner has no guaranteed name at Drassen. `Fred` appears when Drassen is the first mine entered; otherwise `Oswald`, `Calvin`, or `Carl` may be assigned. `Matt` is fixed to Alma.

## Source changes

- [Soldier_Init_List.cc](../src/game/Tactical/Soldier_Init_List.cc): `AddProfilesUsingProfileInsertionData()` now has a narrow fallback for a live, unassigned head-miner profile at a non-abandoned surface mine entrance. It creates the NPC as `CIV_TEAM`, forces `SOLDIER_CLASS_MINER`, inserts at `INSERTION_CODE_CENTER`, and avoids duplicates.
- [SaveLoadGame.cc](../src/game/SaveLoadGame.cc): reruns profile insertion after saved-game soldier restoration, allowing an absent head miner to be repaired when loading an old save.

No changes to mine definitions, mine order, `B13`, save format, or `FACT_MINERS_PLACED`.

## Verification

Passed:

```sh
cmake --build build --target run-ja2-unittests -j2
./android/gradlew assembleDebug
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell monkey -p io.github.ja2stracciatella 1
```

Android package: `io.github.ja2stracciatella`

User retest: head miner found in D13 at 17:00. Time of day does not hide Fred.

## Remaining limitation

Stock Wildfire map data is loaded from external `Data/Maps.slf`; binary map files are not tracked in this repository. The fallback handles missing/mismatched D13 profile slots without modifying the external archive.

## Git handoff

Working tree should contain only this handoff and the two source changes. Review with:

```sh
git status --short
git diff --check
git diff --stat
```
