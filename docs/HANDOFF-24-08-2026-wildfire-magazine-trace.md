# HANDOFF — Wildfire Magazine Trace Audit

## Context

- Date: 2026-08-24
- Branch: `feature/multi-edition-detector`
- Android device: `SM_A175F` (`R5GL31H83QX`)
- Scope: Wildfire 6.08 Sector Inventory magazine names and icon mappings.

The user reported that `4.6mm Magazine` and `5.7mm Magazine, HP` are distinct native Wildfire ammunition types and should not be conflated merely because the engine uses a fallback calibre.

## Established facts

### Native item identities

Read-only decoding of local Wildfire `BinaryData.slf/ITEMDESC.edt` established:

```text
ID 77   short: 4.6mm mag       name: 4.6mm Magazine
ID 105  short: 9*39mm 20       name: 9*39mm Magazine 20
ID 106  short: 5.7mm mag HP    name: 5.7mm Magazine, HP
```

The prior conclusion that ID `77` should be renamed to `5.7mm` was wrong. That temporary override was reverted. No current source change renames magazine text.

### Engine fallback limitation

The externalized engine has no native `4.6mm`, `9x39`, or `12.7mm` calibre:

```text
MP7 / native 4.6mm  → AMMO57 fallback, 20 rounds
AS Val / VSS / 9x39 → AMMO762W fallback
V-94 / native 12.7  → AMMO762N fallback
P90 / native 5.7    → AMMO57, 50 rounds
```

`AMMO57` alone is not evidence that two magazine items have the same native identity.

Relevant baseline:

- `assets/externalized/calibres.json`
- `assets/externalized/weapons.json`
- `src/externalized/WeaponModels.cc`

Current engine magazine compatibility compares calibre and, in some paths, capacity. It does not encode a native magazine-family relationship. This is a broader future design issue; no speculative compatibility refactor was committed.

## Android runtime trace

A temporary `WF-MAG-TRACE` was inserted in:

```text
src/game/Strategic/Map_Screen_Interface_Map_Inventory.cc
RenderItemInPoolSlot()
```

It logged only Sector Inventory ammunition rows. The Android logger writes to the app cache file, not `adb logcat`.

Correct retrieval command:

```sh
adb -s R5GL31H83QX exec-out run-as io.github.ja2stracciatella cat cache/ja2.log \
  | grep 'WF-MAG-TRACE' \
  | tail -100
```

Captured runtime evidence:

```text
id=106 short='5.7mm mag HP' small='interface/mdp1items.sti' frame=21 big='bigitems/p1item21.sti'
id=77  short='4.6mm mag'    small='interface/mdp1items.sti' frame=20 big='bigitems/p1item20.sti'
id=87  short='.357 mag AP'  small='interface/mdp1items.sti' frame=18 big='bigitems/p1item18.sti'
id=72  short='9mm SMG mag AP' small='interface/mdp1items.sti' frame=36 big='bigitems/p1item36.sti'
```

This proves the currently running game does **not** draw 4.6mm and 5.7mm HP from the same small frame or same big-item file:

```text
4.6mm     → frame 20 / p1item20
5.7mm HP  → frame 21 / p1item21
```

It does **not** prove that frame `20` is the correct native Wildfire artwork for 4.6mm. A visual reference/native asset association is still required before changing ID `77` artwork.

## Local read-only asset comparison

The installed user-owned Wildfire resources were inspected read-only; no asset was copied into the repository.

- Wildfire `Mdp1Items.sti` has 138 frames, same count as vanilla, but its frames are redrawn.
- Wildfire frame `24` occupies the original ID `77` inventory-graphic slot and has the magazine silhouette; frame `20` matches the 5.7mm/P90-family silhouette.
- Wildfire `BigItems.slf` contains no separate 4.6mm-named asset. `p1item20.sti` is byte-identical to vanilla's 5.7mm big art. `p1item24.sti` is the vanilla .38 speed-loader art and is not a valid Wildfire 4.6mm replacement.

Current working hypothesis:

```text
ID 77  small frame 24  candidate Wildfire small-art slot
ID 77  big p1item20    existing 5.7mm-family fallback; native 4.6mm big art unverified
```

User-provided labeled Wildfire reference now confirms small frame `24` for ID `77`; source fixup and unit expectation use `20 → 24`. Big art remains `p1item20.sti` fallback; native association is unverified.

The big-art fallback remains unchanged. Focused build/test and Android re-verification remain pending because the local build command was blocked by the command safety classifier.

The temporary trace was removed.

At this handoff, the relevant working-tree changes were:

```text
M  src/externalized/DefaultContentManager.cc
M  src/externalized/DefaultContentManager_unittests.cc
?? docs/HANDOFF-24-08-2026-wildfire-magazine-trace.md
?? docs/HANDOFF-25-08-2026-wildfire-compatibility-audit.md
?? docs/PLAN-wildfire-compatibility-audit.md
?? tools/audit_wildfire_items.py
```

Do not commit/push the unverified ID `77` mapping as final. Next action: obtain a direct original-Wildfire screenshot or lawful frame association, then compare frame `24`, frame `20`, and the required big art before changing again.

## Superseding evidence (2026-08-26)

The labeled Wildfire contact sheet `/private/tmp/wildfire-guns-labeled-contact.png` identifies:

```text
Machete       → slot/frame 47
4.6mm mag     → guns slot/frame 73 → gun73.sti
```

This supersedes the earlier runtime-only candidates above. Source mappings use the exact Wildfire gun-family members `gun47.sti` and `gun73.sti`.

This document is historical. The complete next-step plan is in:

```text
docs/PLAN-wildfire-compatibility-audit.md
docs/HANDOFF-25-08-2026-wildfire-compatibility-audit.md
```

## Existing source state

Committed prior Wildfire magazine fixup is in:

```text
src/externalized/DefaultContentManager.cc
```

It edition-gates the correction using:

```cpp
doesGameResExists("interface/b_map.sti") &&
    !doesGameResExists("interface/b_map.pcx")
```

Current relevant fixups include:

```text
ID 77   AMMO57 / 20 / regular / frame 20 / p1item20
ID 86–91 .357 and 5.45 verified art mapping
ID 105  AMMO762W / 20 / AP / frame 22 / p1item22
```

Existing C++ synthetic coverage:

```text
src/externalized/DefaultContentManager_unittests.cc
WildfireMagazineFixups.CorrectAffectedDefinitions
```

Focused test passed when previously run. The broader built-in suite has pre-existing environment/resource failures unrelated to this mapping work.

## Correct next step

Read-only archive/STCI audit is complete. It validates the archives and decodes all candidate frames, but still does not establish the native big-art association. Keep ID `77` on the explicit AMMO57 fallback. Obtain an authoritative visual reference for native Wildfire 4.6mm art before changing mappings:

1. A screenshot from original Wildfire showing the 4.6mm icon clearly, or
2. A public Wiki image/page identifying that icon, or
3. A lawful read-only item-art/frame association from the installed game resources.

Then compare the reference to runtime trace:

```text
native name → item ID → current frame/path → required frame/path
```

Only change the disproven row. Do not infer art from `AMMO57`, item comments, or vanilla item numbering.

## Constraints

- Never commit/copy commercial Wildfire assets (`.slf`, `.sti`, `.edt`, etc.).
- No `WF6.exe` reverse engineering.
- Do not change `INVRenderItem()` outline-mask behaviour. Palette index `254` remains semantic outline mask; `BltVideoObjectOutline()` is required.
- Preserve Vanilla/Gold behaviour.
- Remove all diagnostic logging before final build/commit.
