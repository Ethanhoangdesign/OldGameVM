# HANDOFF — 13/08/2026

## 934x480 Android — Bottom panel anchored right

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 |
| Scope | Strategic-map bottom panel, history log |
| Status | **Visual verification passed** |

---

## Problem

At 934x480, the Wildfire `map_screen_bottom.sti` panel was anchored at `x=0`.
Its leftmost button recesses covered the history-log area. The screenshot showed
history text over the panel art.

## Fix

The 763px Wildfire bottom panel now anchors against the right screen edge:

```text
934px screen width - 763px panel width = x=171
```

This preserves the left strip for history:

```text
history log: x=8 .. 163
panel art:   x=171 .. 934
```

History text wrapping, mouse region, scroll area, and arrows now derive from
the reserved width instead of the former fixed 335px width.

## Files changed

- `src/game/UILayout.cc`
  - `get_MAP_BOTTOM_BASE_X()` returns `m_screenWidth - 763` for widescreen,
    non-full-size layouts.
- `src/game/Strategic/Map_Screen_Interface_Bottom.cc`
  - History box width ends before the right-anchored panel.
  - Scroll bar and arrows move to the history box's right edge.
- `src/game/Utils/Message.cc`
  - Message line wrapping and display clip region use the reserved history width.

## Screenshot verification

The supplied 934x480 screenshot confirms:

- History remains on the left, unobstructed.
- Bottom panel begins to its right.
- Finance, radar, clock, pause, map controls, and sector display remain inside
  their Wildfire panel recesses.
- No panel art overlaps the history text.

## Native verification

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
```

Result: no error/warning output.

## Regression targets

Before commit, confirm:

- 1024x768 / 1280x720+: full-size map layout unchanged.
- 800x600 / 640x480: vanilla centered layout unchanged.
- 1024x600: history strip remains clear; 763px panel remains right-anchored.
