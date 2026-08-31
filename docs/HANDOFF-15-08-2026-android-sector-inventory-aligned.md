# HANDOFF — Android Sector Inventory Finalized

## Context

- Repo: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`
- Branch: `feature/multi-edition-detector`
- Date: 2026-08-15
- Target device layout: Android `934x480`
- Main file: `src/game/Strategic/Map_Screen_Interface_Map_Inventory.cc`

## Final user-visible result

Sector Inventory renders centered over the strategic map on the Android short layout.

- Wildfire art: `763x647`
- Android fitted panel: approximately `423x359`
- Panel remains intentionally over the map
- Item names render in the original white color
- `Location` and `Total Items` remain visible, rendered once after the scaled panel
- Page/count/sector text remains visible

## Fixes

1. Temporary compose surface uses at least the screen and native art dimensions.
2. Compose surface keeps black inventory pixels opaque.
3. Complete compose clipping rectangle is restored after rendering.
4. Shared `SectorInvTransform` handles panel, items, labels, text, hitboxes, and controls.
5. Scaled controls use hotspots plus manually rendered button art.
6. Scaled text is rendered directly to `guiSAVEBUFFER` after the panel stretch.
7. Scaled path skips the old pre-scale page/count/sector/label text, preventing double-rendered overlap.
8. Item names use white text, matching the previous appearance.
9. `Location` and `Total Items` labels use direct right-edge placement instead of wrapped rendering, preventing clipping/overlap in narrow boxes.
10. Vanilla and full-size paths retain their existing rendering behavior.

## Verification

- `git diff --check`: pass.
- User verified the final Android screenshot as acceptable.
- Android APK build/install still needs to be rerun after the final text-rendering change.

## Recommended device checks

- Tap every item slot.
- Tap previous/next.
- Tap Done.
- Open/close repeatedly; check stale text/art.
- Test empty and multi-item sectors.
- Test underground inventory.
- Regression: vanilla `640x480`.
- Regression: full-size Wildfire layouts.

## Build command

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:assembleDebug --no-daemon --console=plain
```

## Git state

Commit and push the final inventory implementation plus handoff documents only after reviewing the staged diff. No further visual tuning requested.
