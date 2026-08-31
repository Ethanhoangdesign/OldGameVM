# HANDOFF — 13/08/2026 (SESSION 2)

**934x480 Bottom Panel Full-Width — Vanilla Map + Extended Panel**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | 934x480 widescreen: extend bottom panel to full width (x=0 to x=934) while keeping vanilla map art |
| Context | Token quota exceeded in previous session (70M+); continuing implementation |

---

## 1. BACKGROUND

**Previous session (HANDOFF-13-08-2026-map-unified-anchor.md):**
- ✅ Implemented unified bottom panel anchor for full-size layouts (1024x768, 1280x768, 1366x768, 1664x768)
- ✅ Panel now anchors to `get_MAP_VIEW_START_X()` instead of right edge
- ✅ Left filler expands to fill gap (x=0 to map start)
- ❌ 934x480 unchanged: `isMapFullSize()` requires `height >= 720`; 934x480 only has 480px height

**User decision:** Extend 934x480 bottom panel to full width **without scaling map**.

---

## 2. CURRENT STATE

### 934x480 Layout (vanilla):
```
Map view: 336x298px at STD_SCREEN + (270, 10)
  → STD_SCREEN_X = (934 - 640) / 2 = 147
  → Map at x = 147 + 270 = 417, width 336

Bottom panel:
  → MAP_BOTTOM_BASE_X = m_stdScreenOffsetX = 147
  → Panel from x=147 to x=934 (width 787px, not full-width)
  → History log from x=155 (MESSAGE_BOX_X = 147+8)
```

### Code path:
- `isMapFullSize()` = false → uses `m_stdScreenOffsetX`
- `get_MAP_BOTTOM_BASE_X()` returns `m_stdScreenOffsetX` for vanilla
- Bottom panel art (763px wide) starts at MAP_BOTTOM_X

---

## 3. GOAL

Make 934x480 bottom panel **span full width (x=0 to x=934)** like full-size layouts, but:
- Keep map at vanilla size (336x298) — no scaling needed
- Keep map viewport position (STD_SCREEN + 270, 10) — unchanged
- Just extend bottom panel + history log + filter buttons + level selector

---

## 4. IMPLEMENTATION STRATEGY

### Option A: Add widescreen-but-not-fullsize path

Create third condition in key functions:
```cpp
bool isWidescreenLayout() const {
  return m_screenWidth > 934 && m_screenHeight < 720;
}
```

Then in getters:
```cpp
UINT16 UILayout::get_MAP_BOTTOM_BASE_X() const {
  if (isMapFullSize()) return get_MAP_VIEW_START_X();
  if (isWidescreenLayout()) return 0;  // Full-width for 934x480, 1024x600, etc.
  return m_stdScreenOffsetX;          // Centered for smaller screens
}
```

### Files to modify:

1. **src/game/UILayout.h** — add `isWidescreenLayout()` method declaration
2. **src/game/UILayout.cc** — implement logic above
3. **src/game/Strategic/Map_Screen_Interface_Bottom.cc** — left filler should work as-is (already uses `get_MAP_VIEW_START_X()`)
   - Verify line 305: `SGPBox const leftColumn = {0, lcTop, g_ui.get_MAP_VIEW_START_X(), lcH};`
   - For 934x480: `get_MAP_VIEW_START_X()` when `isMapFullSize()=false` returns `m_stdScreenOffsetX + 270 = 417`
   - So left filler spans x=0 to x=417 ✓ (correct — fills left of map)

---

## 5. VERIFICATION CHECKLIST

**On 934x480 preset (Android emulator):**
- [ ] Compile: 0 errors
- [ ] APK builds
- [ ] Map Screen shows vanilla 336x298 map at same position
- [ ] Bottom panel + history log spans full width (x=0 to x=934)
- [ ] No wood gap between left column and map
- [ ] History log text visible and full-width
- [ ] Filter buttons (6 town/mine/teams/militia/air/item) clickable and in panel
- [ ] Level selector visible
- [ ] Finance/radar/clock visible

**On other presets (regression check):**
- [ ] 1024x768 — unchanged (full-size layout)
- [ ] 1280x768 — unchanged (full-size layout)
- [ ] 800x600 — unchanged (vanilla centered)

---

## 6. EDGE CASES

**Screens smaller than 934x480 (e.g., 800x600, 640x480):**
- Keep using vanilla centered layout (`m_stdScreenOffsetX`)
- `isWidescreenLayout()` only applies to width > 934 AND height < 720

**Screen exactly 934x480:**
- `isWidescreenLayout()` = true
- Bottom panel goes from 0 to 934

---

## 7. RELATED DOCS

- [HANDOFF-13-08-2026-map-unified-anchor.md](HANDOFF-13-08-2026-map-unified-anchor.md) — previous session, full-size layout fix
- [HANDOFF-13-08-2026-android-mapscreen-layout.md](HANDOFF-13-08-2026-android-mapscreen-layout.md) — root cause analysis
- [docs/KE-HOACH-mapscreen-fullsize.md](KE-HOACH-mapscreen-fullsize.md) — full-size architecture

---

## 8. NEXT STEPS

1. **Implement** `isWidescreenLayout()` in UILayout.h/cc
2. **Update** `get_MAP_BOTTOM_BASE_X()` to use new condition
3. **Compile** and verify no errors
4. **Test** on Android emulator 934x480 preset
5. **Regression test** on 1024x768, 1280x768, 800x600
6. **Commit** when verified

---

## 9. SESSION 1 GIT DIFF SUMMARY

Already committed (previous session):
```
M src/game/UILayout.cc (line 281)
M src/game/Strategic/Map_Screen_Interface_Bottom.cc (line 305)
```

This session will add:
```
M src/game/UILayout.h (add isWidescreenLayout declaration)
M src/game/UILayout.cc (add isWidescreenLayout implementation + update get_MAP_BOTTOM_BASE_X)
```
