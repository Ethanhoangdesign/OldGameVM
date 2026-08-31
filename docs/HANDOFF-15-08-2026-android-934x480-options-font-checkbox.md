# Handoff — Android 934x480 options font and checkbox

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Target preset:** `934x480`
**Status:** Runtime screenshot verified by user; committed and pushed

## Problem

The Options screen used Android's `FONT14ARIAL` and 2x checkbox rendering at `934x480`.

Results:

- labels wrapped or overflowed their fixed columns;
- 2x checkboxes were too large for the 12pt labels and overlapped nearby text;
- the issue had to be fixed only at exactly `934x480`; all other resolutions had to remain unchanged.

## Changes

### `src/game/Options_Screen.cc`

`OPT_MAIN_FONT` now selects `FONT12ARIAL` only at exactly `934x480`:

```cpp
#define OPT_MAIN_FONT ((SCREEN_WIDTH == 934 && SCREEN_HEIGHT == 480) ? FONT12ARIAL : FONT14ARIAL)
```

This applies to option labels and slider headings. Bottom button text remains `FONT14HUMANIST`, per user request.

The checkbox mouse region now matches the visual scale:

- `934x480`: natural 1x checkbox size;
- every other Android resolution: existing 2x size.

### `src/sgp/Button_System.cc`

`DrawCheckBoxButton()` now renders Android checkboxes at:

- 1x when `SCREEN_WIDTH == 934 && SCREEN_HEIGHT == 480`;
- existing 2x at other Android resolutions.

The draw scale and mouse-region scale are therefore consistent.

## Behavior preserved

- Desktop font and checkbox behavior unchanged.
- Non-`934x480` Android font remains `FONT14ARIAL`.
- Non-`934x480` Android checkbox rendering remains 2x.
- Bottom Options buttons retain their existing font.
- Options geometry, art stretching, row spacing, callbacks, and settings behavior unchanged.

## Verification

Passed:

```sh
git diff --check
```

Runtime screenshots at `934x480` confirmed:

- `FONT12ARIAL` fits the label columns;
- checkbox size matches the smaller font;
- checkbox/text overlap is removed.

Native build was not completed in this session because the environment blocked the build command. Runtime confirmation came from the rebuilt app screenshot supplied by the user.

## Modified files

- `src/game/Options_Screen.cc`
- `src/sgp/Button_System.cc`

## Remaining work

1. Optionally run the normal Android build.
2. Spot-check one non-target Android resolution to confirm its 14pt/2x appearance remains unchanged.
