# HANDOFF — Wildfire artwork audit: failed mapping and next evidence

## 1. Current result

Date: 2026-08-26
Branch: `feature/multi-edition-detector`
Android device: `R5GL31H83QX`

Android debug build, install, launch: **succeeded**.

APK:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

Runtime log:

```text
2026-08-26T07:14:14.573104997Z [INFO] externalized/DefaultContentManager.cc: WF-ITEMART: repointed 15 Wildfire item pictures
2026-08-26T07:21:56.551247717Z [DEBUG] (1) stracciatella::vfs: opened file bigitems/gun42.sti in layer 19
```

Log capture:

```text
/tmp/ja2-android-machete.log
```

User runtime evidence:

- Previous mapping rendered incorrect artwork.
- New labeled contact sheet evidence supersedes that mapping: Machete = slot 47; 4.6mm Magazine = slot 73.

The `WF-ITEMART` line proves only that the Wildfire overlay ran. Opening `bigitems/gun42.sti` proves only that the requested resource was opened. Neither proves semantic identity of the artwork.

## 2. What was done wrong

### Machete

The following unproven assumption was implemented:

```text
Wildfire ID 54 Machete
small: interface/mdguns.sti frame 42
big:   bigitems/gun42.sti
```

The source used the user-labelled interpretation “frame 42 = Machete” without an independent item-to-frame/member association. Android opened `gun42.sti`, but the screenshot still showed incorrect Machete art. The mapping is now **disproved by runtime** and must not be treated as confirmed.

There is also a baseline ownership conflict: `assets/externalized/items.json` associates `bigitems/gun42.sti` with `ROBOT_REMOTE_CONTROL`, item ID `259`. An ID `54 → gun42.sti` override therefore needs explicit Wildfire evidence; the filename number alone is not an identity key.

The faulty implementation is in:

```text
src/externalized/DefaultContentManager.cc:774-796
src/externalized/DefaultContentManager.cc:1040-1073
```

### 4.6mm Magazine

The following assumption was also implemented:

```text
Wildfire ID 77 4.6mm Magazine
small: interface/mdp1items.sti frame 24
big:   bigitems/p1item20.sti
```

Frame `24` was selected from a visual interpretation of the labelled reference. The user’s runtime screenshot shows trousers, so frame `24` is not accepted as the correct runtime artwork. `p1item20.sti` was always only an `AMMO57`/5.7mm-family fallback; it was never proven as native Wildfire 4.6mm big art.

The faulty/current implementation is in:

```text
src/externalized/DefaultContentManager.cc:123
src/externalized/DefaultContentManager.cc:753-771
src/externalized/DefaultContentManager.cc:798-825
```

### Audit conclusion that was too strong

`tools/audit_wildfire_items.py` validates archive structure, STCI headers, frame bounds, and ETRLE decodability. It does **not** identify the semantic owner of a frame or prove that a `gunNN`/`p1itemNN` member belongs to a named Wildfire item.

The audit therefore cannot justify “frame 42 is Machete” or “frame 24 is 4.6mm Magazine” by itself.

### Regression test conclusion that was too strong

The test currently locks in the same unproven Machete mapping:

```text
src/externalized/DefaultContentManager_unittests.cc:188-199
```

It verifies the implementation’s selected paths, not the original Wildfire artwork identity. It must not be described as runtime validation. Any future source correction must update this test with authoritative evidence, not merely change expected numbers.

## 3. Superseding labeled-art evidence

The labeled Wildfire contact sheet `/private/tmp/wildfire-guns-labeled-contact.png` identifies the required slots:

```text
Machete       → slot/frame 47 → gun47.sti
4.6mm mag     → guns slot/frame 73 → gun73.sti
```

This direct labeled evidence supersedes the earlier runtime-only candidates. It remains temporary and is not copied into the repository.

## 4. Where the artwork is stored

The user-owned Wildfire resources remain local only. No commercial archive or extracted artwork belongs in the repository.

The user-owned Wildfire resources remain local only. No commercial archive or extracted artwork belongs in the repository.

Wildfire Data directory:

```text
android/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)/Data/
```

Archives:

```text
android/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)/Data/BinaryData.slf
android/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)/Data/InterFace.slf
android/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)/Data/BigItems.slf
```

Relevant members inside those archives, addressed through the game VFS rather than copied files:

```text
interface/Mdp1Items.sti
interface/Mdguns.sti
bigitems/p1item*.sti
bigitems/gun*.sti
```

Case and exact member presence must be checked against the archive index. The repository contains no authoritative extracted Machete/4.6mm image.

Externalized baseline mappings:

```text
assets/externalized/weapons.json
assets/externalized/magazines.json
assets/externalized/items.json
```

Read-only audit/decoder:

```text
tools/audit_wildfire_items.py
```

Current audit now reports structural art risks only:

```text
collisions.numberedArtCollisions
collisions.smallBigNumberMismatches
collisions.missingArchiveArtReferences
```

These keys identify mdguns/gun and mdp*items/p*item disagreements. They still do not prove semantic owner identity.

Temporary outputs, not source of truth:

```text
/tmp/wildfire-art-audit.json
/private/tmp/wildfire-art-candidates.png
```

Android runtime output:

```text
/tmp/ja2-android-machete.log
```

The screenshots supplied in chat are evidence only. They are not stored in the repository and must not be copied into it.

## 4. Current implementation state

Wildfire detection gate:

```cpp
doesGameResExists("interface/b_map.sti") &&
    !doesGameResExists("interface/b_map.pcx")
```

Current model overlay runs after item models load and before containers are built:

```text
loadItems
→ loadMagazines
→ loadWeapons
→ loadExplosives
→ loadArmours
→ Wildfire artwork overlay
→ construct containers
```

Current mappings, all **implementation state rather than confirmed native associations**:

| Item | Current small art | Current big art | Status |
|---|---|---|---|
| ID `54` Machete | `interface/mdguns.sti`, frame `42` | `bigitems/gun42.sti` | disproved by runtime |
| ID `66` VSS Vintorez | `interface/mdguns.sti`, frame `41` | `bigitems/gun41.sti` | not independently revalidated in this run |
| ID `77` 4.6mm Magazine | `interface/mdp1items.sti`, frame `24` | `bigitems/p1item20.sti` | disproved/unknown; screenshot shows trousers |

Relevant source locations:

```text
src/externalized/DefaultContentManager.cc:123
src/externalized/DefaultContentManager.cc:774-796
src/externalized/DefaultContentManager.cc:798-825
src/externalized/DefaultContentManager.cc:1026-1074
src/externalized/DefaultContentManager_unittests.cc:111-199
src/externalized/DefaultContentManagerUT.h:20-21
```

Vanilla/Gold baseline files were not intentionally changed:

```text
assets/externalized/weapons.json
assets/externalized/magazines.json
```

## 5. Evidence status

### Confirmed

- Wildfire native item name ID `77`: `4.6mm Magazine`, from `BinaryData.slf/ITEMDESC.edt`.
- Wildfire native item name ID `54`: Machete, based on the project’s existing item identity mapping.
- Wildfire archives exist at the local Data path above.
- The overlay executes and reports 15 repointed items.
- The VFS opens `bigitems/gun42.sti` for the current Machete mapping.

### Disproved

- Current ID `54` Machete artwork is correct.
- Current ID `77` frame `24` artwork is correct for the user’s runtime; it renders trousers.
- `WF-ITEMART: repointed 15` is sufficient artwork verification.

### Unknown

- Correct Wildfire Machete small-art frame.
- Correct Wildfire Machete big-art member.
- Correct Wildfire Machete tile artwork.
- Correct ID `77` small-art frame.
- Correct ID `77` big-art member.
- Whether the displayed wrong image comes from a wrong member/frame association, a different runtime surface, or another resource-layer collision.

## 6. Stop condition

Do **not** select another frame/member by number proximity, visual similarity, Vanilla ID, or `AMMO57`/gun-family fallback.

No new source mapping until one of these supplies a direct association:

1. Original Wildfire screenshot with item label and icon clearly visible.
2. Lawful public reference explicitly identifying the frame/member.
3. Read-only archive evidence connecting the named item record to the exact STCI frame/member.

Required association chain:

```text
Wildfire display name
→ ITEMDESC item ID
→ exact small-art archive/member/frame
→ exact big-art archive/member/frame
→ tile-art member/frame, if used
→ runtime surface verification
```

Until then, mark both Machete artwork and ID `77` artwork as unknown/disproved. Do not broaden the overlay. Do not patch `INVRenderItem()`. Do not add item-ID branches to the renderer.

## 7. Next work

1. Inspect the archive index for exact `Mdguns.sti`, `Mdp1Items.sti`, `gun*.sti`, and `p1item*.sti` members.
2. Build a temporary, read-only labelled comparison outside the repository.
3. Trace which graphic path/frame each runtime surface actually requests for IDs `54`, `77`, and `78`.
   Current temporary trace hooks:
   - [Interface_Items.cc](../src/game/Tactical/Interface_Items.cc): `INVRenderItem()`, `GetSmallInventoryGraphicForItem()`, `GetBigInventoryGraphicForItem()`, `GetTileGraphicForItem()`.
   - [Handle_Items.cc](../src/game/Tactical/Handle_Items.cc): `AddItemGraphicToWorld()`.
   Search log token: `WF-ITEMART-TRACE`.
4. Remove or revise the unproven source fixups only after deciding whether fallback or no override is safer.
5. Update regression tests to assert evidence-backed associations, not guessed frame numbers.
6. Rebuild desktop, run unit tests, then rebuild/install Android.

## 8. Guardrails

- Never read `android/build.log`; use `tail` or filtered `grep`.
- Never read `*.bak` or `*.ogvm-bak`.
- Filter CMake output with `grep -E "error:|warning:" | tail -30`.
- Do not copy or commit commercial `.slf`, `.sti`, `.edt`, or generated artwork.
- Do not reverse engineer `WF6.exe`.
- Do not hard-code personal absolute paths in source.
- Do not commit or push unless explicitly requested.
