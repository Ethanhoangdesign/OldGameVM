# Handoff — Android map level selector

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Device/layout:** Android, 934×480
**Status:** fixed, user verified

## Problem

The four map-level rows accepted taps and changed the selected underground level, but the white square selector was invisible.

## Root cause

`RenderMapBorder()` initially drew the current-level marker into `guiSAVEBUFFER`. On the 934×480 widescreen layout, `RenderMapScreenInterfaceBottom()` subsequently drew the full-width bottom panel over that location.

The final marker redraw in `RenderMapScreenInterfaceBottom()` was guarded by `g_ui.isMapFullSize()`. The affected 934×480 layout uses `g_ui.isWidePanel()` through `isWidescreenLayout()`, but is not a full-size map. Therefore the bottom panel erased the marker without redrawing it.

The input path was already correct:

1. `CreateMouseRegionsForLevelMarkers()` creates four 151×23 regions.
2. `LevelMarkerBtnCallback()` calls `JumpToLevel()`.
3. `JumpToLevel()` updates `sSelMap.z` through `ChangeSelectedMapSector()`.

## Fix

### `src/game/Strategic/Map_Screen_Interface_Bottom.cc`

Changed the final selector redraw guard:

```cpp
if (g_ui.isWidePanel())
{
	RenderMapLevelSelectorFullSize();
}
```

This redraws `greenarr.sti` after the bottom panel for both full-size maps and 934×480 widescreen panels. Vanilla layouts retain their existing render path.

## Scope

One condition changed. No mouse regions, level state, level artwork, geometry, underground-map logic, or map data changed.

## Verification

Completed:

```bash
git diff --check
```

Runtime:

- User tapped the map levels on Android 934×480.
- User confirmed the white selector is visible and follows the selected level: `ok đựoc rồi ấy`.

A local build was not run because the tool permission classifier blocked the build command.

## Related change

The same working set also contains the verified Android 934×480 new-email overlay alignment fix documented in:

- `docs/HANDOFF-15-08-2026-android-map-email-icon-alignment.md`
