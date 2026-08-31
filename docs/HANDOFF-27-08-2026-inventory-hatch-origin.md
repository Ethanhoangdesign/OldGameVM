# HANDOFF — Inventory Checkerboard Hatch Origin

## Context

- Repo: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`
- Branch: `feature/multi-edition-detector`
- Date: 2026-08-27
- Subject: checkerboard/black hatch visible over soldier inventory slots

## Finding

The checkerboard is **not a background asset** and is not part of `inventory_bottom_panel.sti`.

The engine generates it at runtime through `Blt16BPPBufferHatchRect()` in:

- `src/sgp/VObject_Blitters.cc:1969-1982`

The function applies an 8×8 alternating pattern, writing black (`0`) only to patterned pixels while leaving the other pixels unchanged. At Android scale/resolution, the remaining pixels are small and the result can look like a mostly solid black overlay.

This matches the reference screenshot: small background dots remain visible through the dark overlay.

## Rendering paths

### Full inventory hatch

When the selected soldier cannot interact with the current item source, the engine sets `gfSMDisableForItems` based on distance and line of sight:

- `src/game/Tactical/Interface_Panels.cc:369-453`

The full inventory area is then hatched:

- `src/game/Tactical/Interface_Panels.cc:1531-1540`

The area starts at `dx + 87`, ends at `dx + 536`, and covers the inventory panel vertically.

### Individual slot hatch

When carrying an item, incompatible inventory slots are marked in `gbInvalidPlacementSlot` and hatched individually:

- `src/game/Tactical/Interface_Panels.cc:689-728`
- `src/game/Tactical/Interface_Items.cc:712-732`

The same effect also marks shopkeeper inventory items already copied into an offer area:

- `src/game/Tactical/ShopKeeper_Interface.cc:3033-3045`
- `src/game/Tactical/Interface_Items.cc:722-725`

## Asset conclusion

`inventory_bottom_panel.sti` is genuine vanilla JA2 inventory-panel artwork loaded in:

- `src/game/Tactical/Interface_Panels.cc:960-1040`

It does not contain the checkerboard overlay. No replacement background file is required.

## Decision

No code or asset change made. Keep the existing hatch implementation. Converting it to a fully black rectangle would diverge from vanilla behavior.

## Follow-up if the effect appears incorrectly

Check the selected soldier’s distance and LOS to the item source. Relevant code:

- `src/game/Tactical/Interface_Panels.cc:375-453`

Only investigate a bug if a soldier is demonstrably in range with clear LOS but the full inventory remains hatched.
