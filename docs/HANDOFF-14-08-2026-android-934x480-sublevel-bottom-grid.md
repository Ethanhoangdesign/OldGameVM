# Handoff — Android 934x480 sublevel bottom grid

**Date:** 2026-08-14
**Branch:** `feature/multi-edition-detector`
**Target:** Wildfire, Android `934x480`
**Status:** **Fixed; runtime confirmed by user**

## Symptom

On underground strategic-map levels, approximately rows N–P were replaced by a solid black band. The `SUBLEVEL: 1/2/3` label remained visible over the band.

Surface-map rendering was unaffected.

## Root cause

Wildfire's `mine_1.sti`, `mine_2.sti`, and `mine_3.sti` report a real subimage size of `676x580`. The half-size `934x480` path first converts the STI video object into a temporary `SGPVSurface`, then halves that surface to `338x290`.

`CreateVideoSurfaceFromObjectFile()` created the complete `676x580` destination surface, but `BltVideoObject()` still used the global clipping rectangle. At `934x480`, that rectangle ended at screen Y `480`, so the last `100px` of the temporary `580px`-high surface were never copied and remained black.

The later half-size blit converted those missing `100px` into a `50px` black band:

```text
source subimage height     = 580px
active global clip height  = 480px
missing source height      = 100px
half-size missing height   = 50px
sector row height          = 18px
covered rows               = about 2.8 rows (N–P)
```

The level label survived because `DisplayLevelString()` draws it separately after the underground artwork.

This was not:

- a bottom-panel overlap;
- missing grid artwork in `mine_N.sti`;
- a map-origin error;
- a restore-width regression;
- underground-sector hiding logic.

## Fix

Changed:

- `src/game/Tactical/Interface_Panels.cc`

`CreateVideoSurfaceFromObjectFile()` now temporarily clips to the temporary surface itself:

```cpp
SGPRect clip;
clip.set(0, 0, r.usWidth, r.usHeight);
SGPRect const oldClip = SetClippingRect(clip);
BltVideoObject(sf.get(), vo.get(), usRegionIndex, 0, 0);
SetClippingRect(oldClip);
```

This allows off-screen-sized assets to be copied completely while preserving the caller's previous global clip.

No map-specific offset or special case was added.

## Verification

Completed:

- Inspected Wildfire `Data/Interface.slf`: `mine_2.sti` subimage 0 is `676x580`.
- Confirmed the asset contains opaque artwork through source row `579`.
- `git diff --check -- src/game/Tactical/Interface_Panels.cc` passed.
- Native `ja2` target built successfully; filtered output contained no warnings or errors.
- User confirmed Android `934x480` runtime result is fixed.

Recommended regression checks:

1. Open sublevels 1–3 at `934x480`; verify rows A–P remain visible.
2. Switch repeatedly between surface and underground levels.
3. Check full-size Wildfire layouts remain unchanged.
4. Exercise other callers of `CreateVideoSurfaceFromObjectFile()` such as stretched tactical panels and strategic UI assets.

## Related handoffs

- `docs/HANDOFF-14-08-2026-android-934x480-map-right-edge-brightness.md`
- `docs/HANDOFF-14-08-2026-android-934x480-sublevel-map-scale.md`
- `docs/HANDOFF-14-08-2026-android-map-red-frame-centering.md`
