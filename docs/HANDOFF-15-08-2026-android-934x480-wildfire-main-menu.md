# Handoff — Android 934x480 Wildfire main menu

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Target preset:** `934x480`
**Status:** Runtime verified by user; committed and pushed

## Problem

The JA2 Wildfire main-menu background and logo were not suitable for the `934x480` Android widescreen preset:

- the background initially appeared too large and hid content;
- proportional fitting introduced black bars at the sides;
- scaling the transparent logo through its own 16-bit temporary surface produced a large black rectangle;
- the logo was clipped or positioned at the upper-left instead of inside the screen frame;
- centering only the ETRLE dimensions ignored the image's internal offsets.

## Final behavior

At exactly `934x480`:

- `mainmenubackground_1024.sti` stretches across the full `934x480` framebuffer;
- the transparent `ja2logo.sti` is composed onto the 1024x768 background surface before that surface is stretched;
- therefore transparent logo pixels preserve the forest background instead of becoming black;
- the visible logo is horizontally centered using its ETRLE `sOffsetX` and width;
- the visible logo begins approximately `40` screen pixels below the top edge;
- the logo is rendered only once;
- menu buttons, version text, copyright text, and background selection remain unchanged;
- the cursor starts at the exact logical center of the screen whenever the main menu initializes.

Other resolutions retain the existing logo render path and position. Their cursor also starts at screen center.

## Change

### `src/game/MainMenuScreen.cc`

Inside `RenderMainMenu()`:

1. Load the selected menu background into the existing 16-bit temporary surface.
2. At exactly `934x480`, load `guiJa2LogoImage` and composite it onto that same temporary surface.
3. Calculate the logo placement from the visible ETRLE region:

```cpp
INT32 const logoX = static_cast<INT32>(bgProps.usWidth) / 2 -
    (static_cast<INT32>(logoProps.sOffsetX) + logoProps.usWidth / 2);
INT32 const logoY = static_cast<INT32>(bgProps.usHeight) * 40 / SCREEN_HEIGHT -
    logoProps.sOffsetY;
```

4. Stretch the composed background and logo to the full framebuffer.
5. Skip the original separate logo blit only at `934x480`.

The temporary-surface clipping rectangle is restored after compositing.

## Iterations rejected

- **Aspect-ratio fit:** avoided distortion but created black side bars.
- **Separate scaled logo surface:** transparent pixels became a black rectangle.
- **Raw width/height centering:** ignored ETRLE offsets; logo appeared left of center.
- **Vertical centering:** placed the logo too low and overlapped the menu.
- **60px top inset:** accepted visually, then user requested another 20px upward adjustment.

### Cursor centering

`InitMainMenu()` now sets the logical cursor position to:

```cpp
SetSafeMousePositionLogical(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
```

At `934x480`, this is `(467, 240)`: the exact center, equally spaced from both horizontal edges.

## Verification

Passed:

```sh
git diff --check
```

Runtime screenshots supplied by the user confirmed:

- background fills the widescreen frame;
- black rectangle removed;
- complete Wildfire logo visible;
- horizontal placement accepted;
- final top inset accepted at approximately `40px`.

A local native/Android build was not completed because the environment blocked the build command. Runtime confirmation came from the user's rebuilt Android app screenshots.

## Modified file

- `src/game/MainMenuScreen.cc`

## Unrelated working-tree changes

Do not overwrite or discard these existing changes:

- `src/game/Options_Screen.cc`
- `src/sgp/Button_System.cc`
- `docs/HANDOFF-15-08-2026-android-934x480-options-font-checkbox.md`

## Remaining work

1. Optionally run the normal Android build.
2. Spot-check a non-`934x480` resolution to confirm the original logo path remains unchanged.
