---
name: android-attachment-touch-fix
description: Wildfire P90 silencer compatibility and touch attachment fix
metadata:
  type: project
---

## Problem
- Symptom: `Silencer for FN P90` would not attach to `FN P90` on Android.
- Desktop reproduced the same failure.
- The item text on Wildfire says it is designed only for the FN P90, but the engine reported the pair as incompatible.

## Final Root Cause
- **Wildfire item IDs differ from vanilla.**
- In Wildfire 6.08 `itemdesc.edt`:
  - `15` = `FN P90`
  - `207` = `Silencer 9mm`
  - `269` = `Silencer P90`
- The codebase only knew the vanilla silencer ID (`207`) in the attachment validation / silenced-fire paths, so the Wildfire P90 silencer was not treated as a real silencer.
- The earlier `SetSafeTouchPosition` build issue was real, but it was only a separate build blocker. It was **not** the attachment bug.

## Fix Applied
Files:
- `src/game/Tactical/Items.cc`
- `src/game/Tactical/Items.h`
- `src/game/Tactical/Weapons.cc`

### What changed
- Added `FindSilencerAttachment(const OBJECTTYPE*)`.
- Added a Wildfire-only compatibility branch for the P90 silencer:
  - treat `CIGARS` / item `269` as the P90 silencer when the game has Wildfire interface art (`interface/b_map.sti` present, `interface/b_map.pcx` absent).
- Updated weapon handling to use the new helper for:
  - gun range penalty
  - silenced burst sound selection
  - silenced shot sound selection
  - silencer volume reduction
- `ValidAttachment()` now accepts the Wildfire P90 silencer against P90.

### Why this is minimal
- Vanilla stays unchanged.
- The special-case is gated to Wildfire only.
- No broad attachment-table rewrite.

## Evidence Collected
### Wildfire data
Decoded from `Data/BinaryData.slf` / `itemdesc.edt`:
- `15` — `FN P90`
- `207` — `Silencer 9mm`
- `269` — `Silencer P90`

### Code evidence
- `src/externalized/WeaponModels.cc`
  - `WeaponModel::canBeAttached()` only checks the vanilla silencer ID for normal data.
- `src/game/Tactical/Items.cc`
  - attachment validation flows through `ValidAttachment()` and `ValidItemAttachment()`.
- `src/game/Tactical/Weapons.cc`
  - silencer logic used `FindAttachment(..., SILENCER)` directly.

## Verification
- Desktop build passes:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)
```

- Regression test for vanilla still exists:
  - `Items.P90SilencerCompatibility`
  - validates `SILENCER (207) -> P90 (15)` in the vanilla data set.
- Manual desktop verification passed on 2026-08-12 using Wildfire 6.08: `Silencer P90` attaches to `FN P90`.
- Android debug APK rebuilt successfully and installed on Samsung `R5GL31H83QX`; user verified the same attachment works in-game.

### Android delivery

```bash
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

- APK: `android/app/build/outputs/apk/debug/app-debug.apk` (~49 MB).
- Build completed successfully; native `libja2.so` includes the shared tactical fix.

## Notes
- The earlier touch-dispatch hypothesis was a red herring.
- Keep the Wildfire special-case narrow until the full Wildfire item table is modelled.

## Related Code
- `src/game/Tactical/Items.cc`
- `src/game/Tactical/Items.h`
- `src/game/Tactical/Weapons.cc`
- `src/externalized/VanillaWeapons_unittests.cc`
- `src/externalized/DefaultContentManager.cc`
- `src/externalized/editiondetector/EditionDetector.cc`
