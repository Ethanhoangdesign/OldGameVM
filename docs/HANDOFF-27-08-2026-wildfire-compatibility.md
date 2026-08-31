# HANDOFF — Wildfire compatibility fix

Date: 2026-08-27
Branch: `feature/multi-edition-detector`
Device: `R5GL31H83QX` (`SM_A175F`)

## Result

Android debug APK built, installed, launched. User confirmed the corrected 4.6mm Magazine artwork is now correct.

Later gameplay testing exposed two separate metadata/reload bugs. First, ID `77` used the `AMMO57` fallback, so the P90 accepted it; the legacy capacity-mismatch branch then stored adjacent ID `78`. The fix gives Wildfire MP7 ID `56` and magazine ID `77` a dedicated `AMMO46` calibre, requires exact capacity, and removes raw `item ID ± 1` conversion.

A second runtime test found P90 unload could still produce 9x39mm ID `105` or 7.62x54mm ID `78`. Wildfire maps embed the original gun object's `usGunAmmoItem`; Wildfire reuses those magazine IDs for different ammunition. Reload top-off previously preserved that stale ID, while unload trusted it. Gun ammo metadata is now normalized against the weapon's calibre, capacity, and ammo type before reload/unload. If the old ammo type has no Wildfire representation, the weapon's valid default magazine identity is used without changing the round count.

The earlier `138`-test suite missed this because `CreateItem(P90)` creates clean current-edition metadata; it did not simulate stale metadata embedded in a map object.

Artwork root cause: frame numbers are sheet-local. `73` on `interface/mdp1items.sti` is First Aid; the user-provided Wildfire guns contact sheet identifies 4.6mm Magazine at slot `73` on the guns sheet.

Final Wildfire mapping:

```text
ID 56  MP7
calibre: AMMO46
capacity: 20

ID 77  4.6mm Magazine
calibre: AMMO46
capacity: 20
small: interface/mdguns.sti, frame 73
big:   bigitems/gun73.sti

ID 15  P90
calibre: AMMO57
capacity: 50

ID 54  Machete
small: interface/mdguns.sti, frame 47
big:   bigitems/gun47.sti
```

## Implementation

- Added a three-argument `ItemModel::overrideInventoryGraphics()` so small path, small frame, and big path are repointed together.
- Added `smallPath` support to Wildfire magazine fixups.
- Added `AMMO46` plus an edition-gated MP7 weapon fixup; P90 remains `AMMO57`.
- `ValidAmmoType()` now uses the existing calibre-and-capacity `WeaponModel::matches(MagazineModel*)` overload across reload and compatibility UI paths.
- Removed `ReloadGun()`'s unsafe `ammo item ID ± 1` conversion; unload remains generic.
- Normalized stale map/save gun magazine IDs before reload and unload; no P90 or magazine-ID special case.
- `FindReplacementMagazine()` now requires an exact ammo type instead of silently substituting another type.
- Added regressions for Wildfire P90 objects carrying stale IDs `105` and `78`, plus a Vanilla P90 AP preservation check.
- Preserved the Wildfire resource gate:

```cpp
doesGameResExists("interface/b_map.sti") &&
    !doesGameResExists("interface/b_map.pcx")
```

- Kept Vanilla/Gold data unchanged.
- Kept renderer generic; no item-ID branch added to `INVRenderItem()`.
- Removed temporary runtime trace helper.
- No commercial `.slf`, `.sti`, or `.edt` files added.

## Verification

Passed or user-confirmed:

- `git diff --check`
- Wildfire audit self-check after updating its synthetic archive-member fixture
- Android `assembleDebug`
- APK install and launch on `R5GL31H83QX`
- Runtime visual verification of 4.6mm Magazine icon
- Focused P90/MP7 regressions: `2` tests passed
- Full desktop unit suite after stale magazine normalization: `139` tests passed
- MP7 accepts ID `77`; P90 rejects ID `77`; created MP7 stores ID `77`, never ID `78`
- Android debug APK rebuilt, installed, and launched after all ammo fixes
- User runtime verification: P90 now loads/unloads only 5.7mm; no generated 9x39mm or 7.62x54mm magazines
- User runtime verification: MP7 still loads/unloads its 4.6mm magazine correctly

The previous Android failure was a missing comma after the ID `77` initializer. Fixed before the successful builds.

## Files

```text
assets/externalized/calibres.json
src/externalized/DefaultContentManager.cc
src/externalized/DefaultContentManager.h
src/externalized/DefaultContentManagerUT.cc
src/externalized/DefaultContentManagerUT.h
src/externalized/DefaultContentManager_unittests.cc
src/externalized/ItemModel.cc
src/externalized/ItemModel.h
src/externalized/VanillaWeapons_unittests.cc
src/game/Laptop/BobbyRGuns.cc
src/game/Tactical/Interface_Items.cc
src/game/Tactical/Items.cc
src/game/Tactical/ShopKeeper_Interface.cc
src/game/Tactical/Weapons_unittest.cc
tools/audit_wildfire_items.py
docs/HANDOFF-27-08-2026-wildfire-compatibility.md
```

Do not add extracted Wildfire assets. Re-run the Android build after future graphics changes.
