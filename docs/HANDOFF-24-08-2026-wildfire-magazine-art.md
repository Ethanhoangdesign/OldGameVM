# HANDOFF — Wildfire Magazine Metadata and Inventory Masks

## Context

- Date: 2026-08-24
- Branch: `feature/multi-edition-detector`
- Target: JA2 Wildfire 6.08, Android physical device `SM_A175F` (`R5GL31H83QX`)

Wildfire keeps vanilla item IDs but changes magazine identity/order. The externalized vanilla magazine table therefore selected incorrect small/big art and semantics. Sector Inventory could show apparel silhouettes for magazine IDs.

## Changes

### Wildfire magazine fixups

`DefaultContentManager::loadMagazines()` now detects Wildfire from its interface replacement:

```cpp
doesGameResExists("interface/b_map.sti") &&
    !doesGameResExists("interface/b_map.pcx")
```

Before `MagazineModel::deserialize()`, `applyWildfireMagazineFixup()` corrects the affected IDs' calibre, capacity, ammo type, small-sheet frame, and big item art. The table covers IDs `71–79`, `86–105`, and `111–113`.

Key verified mappings:

- ID `77`: `4.6mm Magazine`, `AMMO57`, 20 rounds, frame `20`
- ID `89`: `.357 Magazine, HP`, `AMMO357`, 9 rounds, frame `19`
- IDs `86–91`: corrected `.357`/`5.45` mappings

Magazine artwork uses the closest existing family art where Wildfire has no distinct artwork. No commercial game assets are committed.

### Shared artwork override

`ItemModel::overrideInventoryGraphics()` replaces the prior split big/small mutators. Existing Wildfire gun artwork repoints use it after item models load.

### Green icon masks: root cause and final fix

Wildfire `InterFace.slf` contains `InterFace/Mdp1Items.sti`. Palette index `254` is pure green (`0,255,0`) but is a semantic outline-mask pixel, not baked item art.

`BltVideoObject()` renders palette index `254` as green. `BltVideoObjectOutline(..., SGP_TRANSPARENT)` hides it, while a real outline colour renders legitimate compatibility/new-item highlighting.

`INVRenderItem()` must therefore always use `BltVideoObjectOutline()` with its existing `outline_colour` argument. Do not replace the transparent path with `BltVideoObject()`.

Relevant implementation:

- `src/game/Tactical/Interface_Items.cc`
- `src/sgp/VObject_Blitters.cc` (`BltOutline`, source index `254`)

## Tests and runtime verification

- Added synthetic JSON regression coverage in `DefaultContentManager_unittests.cc` for ID `77`, ID `89`, IDs `86–91`, and one unchanged ID.
- `git diff --check`: passed.
- Android debug APK: built successfully.
- APK installed and launched over USB on `R5GL31H83QX`.
- Real Wildfire Sector Inventory confirmed:
  - magazine images no longer show pants/apparel;
  - green masks are gone;
  - IDs `77` and `89` display expected magazine art.
- Desktop startup confirmed logs:
  - `WF-MAGAZINES: corrected 32 Wildfire magazine definitions`
  - `WF-ITEMART: repointed 14 Wildfire item pictures`

## Build/install

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android" && \
./gradlew :app:assembleDebug && \
adb -s R5GL31H83QX install -r app/build/outputs/apk/debug/app-debug.apk && \
adb -s R5GL31H83QX shell am force-stop io.github.ja2stracciatella && \
adb -s R5GL31H83QX shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

## Remaining scope

- No full Wildfire economy/item-table migration.
- No native executable reverse engineering.
- 9×39 and 12.7 mappings retain existing engine calibre fallbacks where no lawful authoritative table is available.
- Manually regression-test Vanilla/Gold and compatibility/new-item highlight states before a release build.

## Local data hygiene

`.gitignore` explicitly excludes user-owned Android game data/runtime directories:

```text
android/GOG/
android/Zeus + Poseidon/
android/ezeus/
```
