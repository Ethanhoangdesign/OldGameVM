# Handoff — Android 934x480 strategic-map sublevel scaling

**Date:** 2026-08-14
**Branch:** `feature/multi-edition-detector`
**Target preset:** `934x480`
**Status:** **Fixed; runtime confirmed by user**

## 1. Symptom

At `934x480`, the surface strategic map used the expected half-size grid, but underground levels 1–3 appeared approximately 2x larger and were clipped at the right and bottom edges. Cave sectors consequently looked displaced relative to the shared map grid and cursor.

The `Sublevel: N` label also remained anchored to `STD_SCREEN_X/Y`, so it did not follow the special `934x480` map origin.

## 2. Root cause

Wildfire supplies these underground-map assets at full size:

```text
mine_1.sti: 676x580
mine_2.sti: 676x580
mine_3.sti: 676x580
```

The non-full-size branch of `HandleLowerLevelMapBlit()` blitted them directly. The surface map already used half-size rendering, producing mismatched scales.

Vanilla assets are already `338x290`; scaling every asset unconditionally would shrink vanilla maps a second time.

Map origin, grid, cursor, clipping, hit regions, and sector conversion were already shared and correct. No layout-wide offset fix was needed.

## 3. Fix

Changed:

- `src/game/Strategic/Map_Screen_Interface_Map.cc`

### Underground art

The non-full-size branch now loads subimage `0` through `CreateVideoSurfaceFromObjectFile()` and checks its real dimensions:

- `676x580`: render with `BltVideoSurfaceHalf()`.
- Any other size, including vanilla `338x290`: preserve the existing `BltVideoObject()` path.

Both paths retain the existing anchor:

```cpp
MAP_VIEW_START_X + 21
MAP_VIEW_START_Y + 17
```

The full-size branch remains unchanged.

### Sublevel label

The non-full-size label changed from screen-relative coordinates:

```cpp
STD_SCREEN_X + 432
STD_SCREEN_Y + 305
```

to map-relative coordinates:

```cpp
MAP_VIEW_START_X + 162
MAP_VIEW_START_Y + 295
```

Vanilla placement remains numerically identical because its map origin is `(270,10)`. The `934x480` layout now follows its map origin `(500,31)` automatically.

## 4. Deliberately unchanged

No changes to:

- `UILayout::get_MAP_VIEW_START_X/Y()`
- `DrawMap()` surface-map rendering
- `GetScreenXYFromMapXY()`
- map clipping rectangles
- grid or cursor positioning
- map mouse/hit regions
- per-level offsets
- full-size Wildfire rendering
- `BltVideoSurfaceHalf()` implementation

## 5. Verification

Completed:

- `git diff --check -- src/game/Strategic/Map_Screen_Interface_Map.cc` passed.
- Runtime result at `934x480` confirmed working by user.
- Surface and underground maps now use matching geometry.
- Sublevel art no longer renders at 2x size or clips at the right/bottom.

Not completed in this session:

- Native `unittest` build; command was blocked twice by the environment permission classifier.
- Explicit regression run with vanilla `640x480`/`800x600` assets.
- Explicit full-size Wildfire regression run at `1024x768`/`1280x720`.

## 6. Regression checklist

If revisiting:

1. Test sublevels 1–3 at `934x480`; verify grid, cursor, and cave sectors share coordinates.
2. Test vanilla `338x290` art; verify it remains `338x290`, not quarter-sized.
3. Test full-size Wildfire; verify original size and anchor remain unchanged.
4. Touch the same sector before and after changing levels; verify selection remains aligned.

## 7. Session delta

```diff
- Non-full sublevels always use BltVideoObject().
+ 676x580 sublevels use BltVideoSurfaceHalf().
+ Vanilla-sized sublevels retain BltVideoObject().

- Non-full Sublevel label uses STD_SCREEN_X/Y.
+ Non-full Sublevel label uses MAP_VIEW_START_X/Y.
```

The working tree contains many unrelated changes. Stage selectively:

```sh
git diff -- src/game/Strategic/Map_Screen_Interface_Map.cc
git add -p src/game/Strategic/Map_Screen_Interface_Map.cc
git add docs/HANDOFF-14-08-2026-android-934x480-sublevel-map-scale.md
git diff --cached
```

Suggested commit subject:

```text
OGVM-ANDROID: scale Wildfire sublevel maps at 934x480
```
