# HANDOFF — 14/08/2026

## Map level selector: hitbox alignment fix

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Scope | Strategic Map, wide-panel level selector |
| Changed file | `src/game/Strategic/Map_Screen_Interface_Border.cc` |
| Status | **Code fixed; final runtime verification pending** |

---

## 1. Symptom

The four terrain/depth rows were visible in Strategic Map, but clicking them did not change the selected map level.

Observed at the full-size/wide-panel layout shown in the 1280x720 build.

---

## 2. Click path

```text
LevelMouseRegions[4]
  → LevelMarkerBtnCallback()
  → JumpToLevel(level)
  → ChangeSelectedMapSector(...)
```

Relevant code:

- Mouse regions: `Map_Screen_Interface_Border.cc:529-545`
- Callback: `Map_Screen_Interface_Border.cc:554-565`
- State change: `Map_Screen_Interface.cc:847-871`

The callback and `JumpToLevel()` logic were intact. The failure came from mouse-region geometry.

---

## 3. Root causes

### A. Wrong vertical pitch

Wide-panel artwork uses four rows, each `23 px` high. The mouse regions initially retained vanilla's `8 px` pitch:

```cpp
#define MAP_LEVEL_MARKER_DELTA 8
#define MAP_LEVEL_MARKER_HEIGHT (g_ui.isWidePanel() ? 23 : 8)
```

This produced overlapping regions:

```text
level 0: y +  0 .. y + 23
level 1: y +  8 .. y + 31
level 2: y + 16 .. y + 39
level 3: y + 24 .. y + 47
```

Fix:

```cpp
#define MAP_LEVEL_MARKER_DELTA (g_ui.isWidePanel() ? 23 : 8)
```

Result: hitboxes and current-level marker now use the same `23 px` pitch as the wide artwork. Vanilla remains `8 px`.

### B. Wrong horizontal anchor

After fixing the pitch, runtime testing still failed. The visible selector is baked into `map_screen_bottom.sti` at panel-relative `x = 200`, but the mouse regions were created at panel-relative `x = 500`.

Old:

```cpp
g_ui.get_MAP_BOTTOM_BASE_X() + 500
```

Fixed:

```cpp
g_ui.get_MAP_BOTTOM_BASE_X() + 200
```

The old hitboxes sat `300 px` to the right of the visible terrain rows. Clicking the artwork therefore never reached `LevelMarkerBtnCallback()`.

---

## 4. Final geometry

Current definitions:

```cpp
#define ONMAP_MAP_LEVEL_MARKER_X (!g_ui.isWidePanel() ? (STD_SCREEN_X + 565) : \
    (g_ui.get_MAP_BOTTOM_BASE_X() + 200))
#define ONMAP_MAP_LEVEL_MARKER_Y (!g_ui.isWidePanel() ? (STD_SCREEN_Y + 323) : \
    (g_ui.get_MAP_BOTTOM_BASE_Y() + 369))
#define MAP_LEVEL_MARKER_DELTA   (g_ui.isWidePanel() ? 23 : 8)
#define MAP_LEVEL_MARKER_WIDTH   (g_ui.isWidePanel() ? 151 : 55)
#define MAP_LEVEL_MARKER_HEIGHT  (g_ui.isWidePanel() ? 23 : 8)
```

Wide-panel regions:

```text
x = panel base + 200
width = 151 px

level 0: y = panel base + 369, height 23
level 1: y = panel base + 392, height 23
level 2: y = panel base + 415, height 23
level 3: y = panel base + 438, height 23
```

Render marker and mouse regions now share `MAP_LEVEL_SLOT_*` geometry.

---

## 5. Verification status

Completed:

- User built and tested the first pitch-only fix.
- That build still failed, exposing the horizontal-anchor bug.
- Horizontal anchor corrected from `+500` to `+200`.
- `git diff --check -- src/game/Strategic/Map_Screen_Interface_Border.cc` passed after the final edit.

Pending:

- Rebuild after the `+200` anchor correction.
- Confirm all four visible rows respond to clicks.
- Confirm the current-level marker moves to the clicked row.
- Confirm vanilla layouts retain the original `55x8` selector behavior.

Do not mark runtime verification complete until the post-anchor build has been tested.

---

## 6. Runtime checklist

1. Rebuild the same target used for the 1280x720 test.
2. Open Strategic Map.
3. Click the center of each terrain row, top to bottom.
4. Verify the selected map depth changes for levels `0`, `1`, `2`, and `3`.
5. Verify the marker moves exactly `23 px` per row.
6. Test 640x480 or another vanilla layout; verify its selector still uses `8 px` rows.

If clicks still fail, first inspect whether a higher-priority `MOUSE_REGION` covers panel-relative rectangle `(200,369)-(351,461)`. Do not change `JumpToLevel()` without evidence; its state path is correct.

---

## 7. Session delta

Only these intended edits belong to this bug fix:

```diff
- panel-relative selector X: 500
+ panel-relative selector X: 200

- wide-panel row pitch: 8
+ wide-panel row pitch: 23
```

The working tree contains many unrelated changes. Stage this fix selectively:

```bash
git diff -- src/game/Strategic/Map_Screen_Interface_Border.cc
git add -p src/game/Strategic/Map_Screen_Interface_Border.cc
git add docs/HANDOFF-14-08-2026-map-level-selector-click.md
git diff --cached
```

Suggested commit subject after runtime verification:

```text
OGVM: align map level selector hitboxes with wide panel
```

---

*Handoff end.*
