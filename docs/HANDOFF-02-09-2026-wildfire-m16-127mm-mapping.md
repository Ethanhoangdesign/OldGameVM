# HANDOFF — Wildfire M16 / 12.7mm mapping

Date: 2026-09-02
Branch: `feature/multi-edition-detector`

## Result

Fixed Wildfire item-ID reuse that exposed the M16 as an M24 and allowed 12.7mm HE in the wrong weapon.

## Root cause

Wildfire changes the meaning of several vanilla item IDs:

- ID `19`: Wildfire `M16`; base profile was `M24`, `SN_RIFLE`, `AMMO762N`, 5-round, range 800.
- IDs `111–113`: Wildfire `12.7mm AP/HE/HEAP`; base profiles shared `AMMO762N` and capacity 5.
- ID `67`: Wildfire `V-94`; it must share 12.7mm ammunition, not 7.62mm NATO.

Magazine matching compares calibre index and capacity. The old shared `AMMO762N` + capacity `5` mapping therefore made the M16/M24 accept 12.7mm magazines.

## Fix

Wildfire-only fixups in `src/externalized/DefaultContentManager.cc`:

- ID `19` → `ASRIFLE`, `AMMO556`, 30-round magazine, range 400, M16 assault-rifle stats.
- IDs `111–113` → dedicated `AMMO127`.
- ID `67` → dedicated `AMMO127`, capacity 5.
- Added calibre index `18` (`AMMO127`) in `assets/externalized/calibres.json`.
- Added index-18 `12.7mm` labels to all calibre translation tables.

Vanilla/Gold base profiles remain unchanged. Wildfire detection gates the overlay.

## Tests / verification

- Desktop unit suite: `[  PASSED  ] 140 tests.`
- `git diff --check`: passed.
- All 16 calibre translation tables: 19 entries, index 18 = `12.7mm`.
- Android debug build: `BUILD SUCCESSFUL`.
- USB install: `Success` on `R5GL31H83QX`.
- Android launch: successful.

Manual in-game confirmation remains useful: M16 accepts 5.56mm/30-round magazines, rejects 12.7mm HE; V-94 accepts 12.7mm ammunition.

## Rebuild

```bash
./tools/build-android-debug.sh
adb -s R5GL31H83QX install -r android/app/build/outputs/apk/debug/app-debug.apk
adb -s R5GL31H83QX shell am force-stop io.github.ja2stracciatella
adb -s R5GL31H83QX shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

## Commit / push

Changes are on `feature/multi-edition-detector`. Do not merge into `master` without review.
