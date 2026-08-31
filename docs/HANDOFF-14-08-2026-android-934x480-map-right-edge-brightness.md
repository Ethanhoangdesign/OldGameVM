# Handoff — Android 934x480 strategic-map right-edge brightness

**Date:** 2026-08-14
**Branch:** `feature/multi-edition-detector`
**Target preset:** `934x480`
**Status:** **Fixed; runtime confirmed by user**

## 1. Symptom

At `934x480`, the two rightmost columns of the surface strategic map appeared brighter than the rest of the map.

The terrain, grid, cursor, and underground maps were otherwise aligned correctly.

## 2. Root cause

The surface map and sector shading were rendered correctly into `guiSAVEBUFFER`.

The failure happened when `RenderMapRegionBackground()` copied that buffer to `FRAME_BUFFER`:

```cpp
RestoreExternBackgroundRect(
    MAP_LEFT_COL_X + 261,
    MAP_LEFT_COL_Y,
    MAP_BG_WIDTH,
    359);
```

`MAP_BG_WIDTH` is the original vanilla width:

```cpp
#define MAP_BG_WIDTH (640 - 261) // 379px
```

At `934x480`:

```text
restore start                         = MAP_LEFT_COL_X + 261 = 432
old restore end                       = 432 + 379 = 811
half-size Wildfire map start          = MAP_VIEW_START_X + 1 = 501
half-size Wildfire map width          = 714 / 2 = 357
half-size Wildfire map exclusive end  = 501 + 357 = 858
missing restored width                = 858 - 811 = 47px
```

Those missing `47px` cover approximately the final two sector columns. Their newly shaded pixels remained only in `guiSAVEBUFFER`; the visible `FRAME_BUFFER` retained older/brighter pixels.

This was not:

- a defect in `ShadeMapElem()`;
- incorrect `SF_ALREADY_VISITED` state;
- unwanted source-art overflow;
- a map-origin or grid error;
- an underground-map scaling regression.

## 3. Fix

Changed:

- `src/game/Strategic/MapScreen.cc`

The non-full-size restore now expands only for exact `934x480`:

```cpp
UINT16 const mapBgWidth = SCREEN_WIDTH == 934 && SCREEN_HEIGHT == 480
    ? MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X + 1 -
        (MAP_LEFT_COL_X + 261)
    : MAP_BG_WIDTH;

RestoreExternBackgroundRect(
    MAP_LEFT_COL_X + 261,
    MAP_LEFT_COL_Y,
    mapBgWidth,
    359);
```

The expression restores through the half-size map art's exclusive right edge:

```text
MAP_VIEW_START_X + MAP_VIEW_WIDTH + MAP_GRID_X + 1
= 500 + 336 + 21 + 1
= 858
```

Other layouts retain the original `MAP_BG_WIDTH` behavior.

## 4. Deliberately unchanged

No changes to:

- `DrawMap()`;
- `ShadeMapElem()`;
- `BltVideoSurfaceHalf()`;
- surface-map source clipping;
- visited-sector flags;
- `UILayout::get_MAP_VIEW_START_X/Y()`;
- map grid or cursor geometry;
- underground-map rendering;
- vanilla and full-size restore paths.

Do not clip the `714px` Wildfire source to `672px`. The additional source width contains the left rail plus the complete playable terrain; clipping it would remove sector column 16.

## 5. Verification

Completed:

- `git diff --check -- src/game/Strategic/MapScreen.cc` passed.
- Native `ja2` target built successfully.
- Filtered build output contained no warnings or errors.
- User confirmed the `934x480` Android runtime result is correct.

Recommended regression checks if revisiting:

1. Open the surface strategic map at `934x480`; verify columns 15–16 match the shading behavior of columns 1–14.
2. Trigger redraws by toggling towns, mines, teams, militia, items, and airspace.
3. Check underground levels 1–3 remain scaled and aligned.
4. Check vanilla `640x480` and `800x600` layouts.
5. Check full-size Wildfire at `1024x768` and `1280x720`.

## 6. Session delta

```diff
- Restore every non-full-size map with fixed MAP_BG_WIDTH (379px).
+ At exact 934x480, restore through the shifted map's right edge.
+ Preserve MAP_BG_WIDTH for every other non-full-size layout.
```

The working tree contains unrelated changes. Stage selectively:

```sh
git add -p src/game/Strategic/MapScreen.cc
git add docs/HANDOFF-14-08-2026-android-934x480-map-right-edge-brightness.md
git diff --cached
```

Suggested commit subject:

```text
OGVM-ANDROID: restore full strategic map at 934x480
```
