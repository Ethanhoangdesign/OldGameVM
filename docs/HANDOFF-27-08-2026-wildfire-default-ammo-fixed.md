# HANDOFF — Wildfire default-ammo crash fixed

Date: 2026-08-27
Branch: `feature/multi-edition-detector`

## Result

Wildfire Android crash fixed. User rebuilt/reinstalled the APK and confirmed the game loads without:

```text
Found no default ammo for gun
```

`Error.sav` was valid. The failure came from Wildfire item metadata, not save corruption.

## Root cause

Wildfire reuses item IDs with different identities:

- Weapon ID `56` (`AUTOMAG_III` / MP7) requires `AMMO46`, capacity `20`.
- Magazine ID `77` (`CLIP38_6`) is the Wildfire 4.6mm magazine, capacity `20`.
- Base data described those IDs as other calibres, so `DefaultMagazine()` found no exact calibre/capacity match.

## Fix

Runtime fixups in `src/externalized/DefaultContentManager.cc` are gated to Wildfire:

```cpp
doesGameResExists("interface/b_map.sti") &&
    !doesGameResExists("interface/b_map.pcx")
```

They correct:

- ID `56` → `AMMO46`, capacity `20`.
- ID `77` → `AMMO46`, capacity `20`, `AMMO_REGULAR`.

`AMMO46` is defined in `assets/externalized/calibres.json`. Vanilla/Gold base data remains unchanged.

A direct regression assertion was added in `src/externalized/VanillaWeapons_unittests.cc`:

```cpp
EXPECT_EQ(DefaultMagazine(ITEMDEFINE::AUTOMAG_III), ITEMDEFINE::CLIP38_6);
```

Existing Wildfire reload/unload and stale-magazine regressions remain in place.

## Verification

- `git diff --check` passed.
- Focused Wildfire MP7/P90 regressions already passed in the prior compatibility run.
- Android APK rebuilt, installed, launched.
- User confirmed Wildfire MP7 loads/unloads correctly without the crash.
- Missing `intro/splashscreen.smk` remains a non-fatal Wildfire intro-video warning.

## Rebuild

```bash
./android/gradlew -p android clean
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

Inspect packaged calibre data before install:

```bash
unzip -p android/app/build/outputs/apk/debug/app-debug.apk \
  assets/externalized/calibres.json | grep AMMO46
```
