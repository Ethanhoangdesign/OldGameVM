# Handoff — Android 934x480 Initial Game Settings layout

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Target preset:** `934x480`
**Status:** Runtime screenshot verified by user; ready to commit and push

## Problem

The Android Initial Game Settings screen reused the enlarged layout intended for taller resolutions at `934x480`.

Results:

- huge labels overlapped neighboring rows and controls;
- checkbox hit areas remained 2x while the checkbox renderer used 1x at this preset;
- the save-mode descriptions collided with checkboxes;
- the two option columns were too close;
- the dark backing panel did not contain the widened controls.

## Changes

### `src/game/GameInitOptionsScreen.cc`

Added an exact-resolution compact layout for Android `934x480`:

- title uses `FONT16ARIAL`;
- option labels and save-mode descriptions use `FONT12ARIAL`;
- vertical gaps and text offsets use the original 1x geometry;
- checkbox offset expands to give labels more horizontal room;
- checkbox hit areas remain natural size, matching `DrawCheckBoxButton()`;
- left and right option columns move apart, producing 40 px more separation than the first compact pass;
- OK/Cancel remain centered with the full group;
- the dark backing panel expands to the full 640 px logical width and contains both columns;
- all responsive changes are restricted to exactly `934x480`.

## Behavior preserved

- Desktop layout unchanged.
- Other Android resolutions retain huge-font, enlarged-spacing, and 2x checkbox behavior.
- Background stretching, callbacks, option state, confirmation dialogs, and game start flow unchanged.

## Verification

Passed:

```sh
git diff --check
cmake --build build --target ja2
./tools/build-android-debug.sh
```

Runtime screenshots at `934x480` confirmed:

- all text is readable without overlap;
- save-mode descriptions fit beside their checkboxes;
- option columns have sufficient separation;
- checkboxes and hit areas align;
- dark backing panel contains the complete group;
- title and OK/Cancel remain visually centered.

## Modified files

- `src/game/GameInitOptionsScreen.cc`
- `docs/HANDOFF-15-08-2026-android-934x480-gio-layout.md`

## Build output

- `android/app/build/outputs/apk/debug/app-debug.apk`
