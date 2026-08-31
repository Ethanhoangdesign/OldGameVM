# HANDOFF — 13/08/2026

## 934x480 Android — Bottom-panel button recess alignment

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 |
| Scope | Strategic map bottom-panel buttons |
| Status | **Filter grid patched; native build passed; Android visual verification pending** |

---

## Completed

The supplied Wildfire reference confirms the six map-filter buttons use a **3-column × 2-row** layout, not one horizontal row.

Patched in `src/game/Strategic/Map_Screen_Interface_Border.cc`:

```text
row 1: Town, Mine, Teams    x=10, 69, 128    y=369
row 2: Militia, Air, Items  x=10, 69, 128    y=413
```

Coordinates are panel-relative. At 934x480:

```text
isWidescreenLayout() == true
isWidePanel()       == true
get_MAP_BOTTOM_BASE_X() == 0
get_MAP_BOTTOM_BASE_Y() == 0
panel origin            == (0, 359)
```

Implementation:

- `BTN_PITCH_X = 59`
- Button frame size: `50x44`
- First row: panel-relative `y=10`
- Second row: panel-relative `y=54`
- Wide-panel branch only
- Vanilla branches unchanged

Also patched `src/game/Strategic/Map_Screen_Interface_Bottom.cc`:

- Disabled time-control masks now use `g_ui.isWidePanel()`.
- Wide pause mask now matches the active pause region: `66x13`.

---

## Current coordinates

### Filter buttons

File: `src/game/Strategic/Map_Screen_Interface_Border.cc`

```text
Town:     x=10,  y=369
Mine:     x=69,  y=369
Teams:    x=128, y=369
Militia:  x=10,  y=413
Air:      x=69,  y=413
Items:    x=128, y=413
```

### Exit / time controls

File: `src/game/Strategic/Map_Screen_Interface_Bottom.cc`

```text
options:       x=554, y=372
laptop:        x=554, y=410
tactical:      x=607, y=410
time less:     x=558, y=456
time more:     x=639, y=456
pause region:  x=573, y=456, width=66, height=13
```

### Scroll controls

```text
scroll x       = 355
scroll up y    = MapScreenLogTop() + 16
scroll down y  = MapScreenLogTop() + 87
```

These remain unchanged. Move only if a 934x480 screenshot proves the circled controls are message-scroll arrows.

---

## Verification

Native build completed successfully:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
```

Result: no error/warning output.

`git diff --check` was not clean globally because of pre-existing unrelated trailing whitespace in:

```text
src/game/Tactical/Interface_Items.cc:1976
```

Targeted strategic files have no whitespace errors.

---

## Remaining work

1. Build/install Android debug APK.
2. Run 934x480 emulator preset.
3. Enter Strategic Map.
4. Verify filter buttons sit inside the Wildfire recesses.
5. Verify every moved button's click region and disabled state.
6. Capture a 934x480 screenshot.
7. Adjust only wide-panel offsets if visual evidence shows a constant offset.
8. Regression-check:
   - 1024x768 / 1280x720+ Wildfire full-size layout
   - 800x600
   - 640x480 vanilla layout

Recommended Android build command:

```bash
cd android
./gradlew assembleDebug
```

---

## Do not do

- Do not alter vanilla coordinate branches.
- Do not change map geometry.
- Do not move scroll controls without screenshot evidence.
- Do not guess 934x480 pixel offsets from the 1024x768 reference.
- Do not modify unrelated Android controller/settings files.
