# Handoff — Remove Android RMB overlay button

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Platform:** Android
**Status:** fixed, user verified

## Request

Remove the temporary `RMB` overlay button. It is no longer needed.

## Change

### `android/app/src/main/java/org/libsdl/app/SDLActivity.java`

Removed the `RMB` button created in `onCreate()`:

- Button construction and `RMB` label.
- Top-right overlay layout parameters.
- Click listener toggling `mSecondaryMouseModifierDown`.
- Addition of the button to `mLayout`.

## Scope

Only the visible overlay button was removed. Existing right-click input paths remain unchanged, including keyboard modifier and mouse handling that use `mSecondaryMouseModifierDown`.

No game UI, map-level selector, touch-region, or native C++ logic changed.

## Verification

Completed:

```bash
git diff --check
```

Runtime:

- User confirmed the Android result: `ok đực ròi`.

A local Android build was not run.

## Related handoff

- `docs/HANDOFF-15-08-2026-android-map-level-selector.md`
