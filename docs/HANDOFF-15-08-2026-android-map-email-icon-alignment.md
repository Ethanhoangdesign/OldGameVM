# Handoff — Android map email icon alignment

**Date:** 2026-08-15
**Branch:** `feature/multi-edition-detector`
**Device/layout:** Android, 934×480
**Status:** fixed, user verified

## Problem

On the strategic map, the blinking new-email icon appeared left of the laptop button instead of on top of it.

## Root cause

The laptop button position in `CreateButtonsForMapScreenInterfaceBottom()` uses `g_ui.isWidePanel()`:

- wide panel: `MAP_BOTTOM_BASE_X + 554`
- standard panel: `MAP_BOTTOM_BASE_X + 456`

`CheckForAndRenderNewMailOverlay()` still selected those coordinates with `g_ui.isMapFullSize()`.

At 934×480, the layout is a wide panel but not full-size. The laptop moved to the wide-panel position while the email overlay remained at the standard position.

## Fix

### `src/game/Strategic/MapScreen.cc`

Changed the email overlay anchor condition:

```cpp
INT32 const lapX = (g_ui.isWidePanel() ? 554 : 456) + g_ui.get_MAP_BOTTOM_BASE_X();
```

This matches the laptop button condition and keeps the email icon attached to the button in both layouts.

## Scope

One-line coordinate-condition fix. No asset, input, scaling, or laptop-screen behavior changed.

## Verification

Completed:

```bash
git diff --check
```

Runtime:

- User tested the fix on the affected Android layout.
- User confirmed: `ok được ròi ấy`.

Android build was not run in this session because the tool permission classifier blocked the build command.

## Pending

The source change and this handoff are uncommitted.
