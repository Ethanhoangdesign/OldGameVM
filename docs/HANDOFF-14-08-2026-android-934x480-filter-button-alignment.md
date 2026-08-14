# HANDOFF — 14/08/2026

## Android 934x480 — Strategic-map filter button alignment

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 |
| Scope | Strategic-map six-button filter block |
| Status | **Visual verification passed** |

## Context

The six strategic-map filter buttons used the correct Wildfire 3-column × 2-row arrangement, but the lower row did not sit inside its three artwork recesses. The target was to move only the lower row vertically while preserving the upper row, click regions, callbacks, and non-wide layouts.

## Final layout

Changed `BTN_SECOND_ROW_Y` in `src/game/Strategic/Map_Screen_Interface_Border.cc`:

```cpp
#define BTN_SECOND_ROW_Y (g_ui.isWidePanel() ? (g_ui.get_MAP_BOTTOM_BASE_Y() + 423) : BTN_ROW_Y)
```

Final wide-panel coordinates, relative to the map-screen origin:

```text
Row 1 — Town, Mine, Teams:    x=base+10, base+69, base+128; y=base+369
Row 2 — Militia, Air, Items:  x=base+10, base+69, base+128; y=base+423
```

At 934x480, `get_MAP_BOTTOM_BASE_Y() == 0`, therefore the lower row uses `y=423`.

Adjustment history:

```text
Initial: y=413
First visual attempt: y=436 — 13px too low
Final: y=423 — visually confirmed
```

## Files changed

- `src/game/Strategic/Map_Screen_Interface_Border.cc`
  - Lower-row wide-panel Y offset only.
  - Upper row unchanged.
  - Vanilla/non-wide branch unchanged.

## Verification

- User screenshot confirmed all three lower buttons now occupy the intended recesses.
- `git diff --check -- src/game/Strategic/Map_Screen_Interface_Border.cc` passed.
- Native build passed before the final coordinate-only correction; existing unrelated warnings remained in `Tactical/Interface.cc` and `sgp/Video.cc`.

## Regression checks

When touching this layout again, verify:

- Android 934x480: both rows remain inside all six recesses.
- Button click regions move with their images.
- 1024x768 / 1280x720+ Wildfire layouts remain aligned.
- 800x600 and 640x480 vanilla layouts remain unchanged.

Do not move the upper row or alter the shared non-wide coordinates without new visual evidence.
