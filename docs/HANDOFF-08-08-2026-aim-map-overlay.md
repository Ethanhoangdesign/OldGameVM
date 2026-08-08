# HANDOFF — 08/08/2026

**AIM video-conference guard + map new-mail icon anchor**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | AIM laptop guard + Android/full-size mapscreen overlay |
| Emulator | `Pronunciation_API_35` |
| Package | `io.github.ja2stracciatella` |
| Launcher | `io.github.ja2stracciatella/.LauncherActivity` |
| Prior | `docs/HANDOFF-08-08-2026-options-saveload-btn-imp.md` |

---

## 0. STATUS

| Item | Status | Note |
|---|---|---|
| AIM selection-light null crash | **DONE** | Guard missing video-conference button refs |
| Mapscreen blinking new-mail icon drift | **DONE** | Anchor icon to Laptop button |
| Normal map layout | **DONE** | Existing icon coordinates preserved |
| Full-size map layout | **DONE** | Icon follows Laptop button, no finance-box overlap |
| User visual smoke | **DONE** | User confirmed map overlay is correct |
| Native rebuild after latest two-file patch | Not recorded here | Rebuild NDK before device verification |

---

## 1. AIM VIDEO-CONFERENCE GUARD

### Symptom

AIM video-conference rendering could call `DisplaySelectLights()` while contract-length or equipment button references were not initialized. Dereferencing a null `GUIButtonRef` could crash during redraw or mode transitions.

### Fix — `src/game/Laptop/AIMMembers.cc`

`DisplaySelectLights()` now returns when any required button is missing:

- `giContractLengthButton[0..2]`
- `giBuyEquipmentButton[0..1]`

Guard runs before `DrawButtonSelection()` dereferences button positions or click state. No behavior changes when all buttons exist.

---

## 2. MAPSCREEN NEW-MAIL ICON

### Symptom

Blinking new-mail icon on mapscreen sat over the finance box in full-size layout. Icon used fixed coordinates based on normal bottom-panel layout, while full-size layout moves Laptop button right.

### Root cause

`CheckForAndRenderNewMailOverlay()` used hardcoded offsets:

- Normal layout icon: approximately `+464, +417`
- Full-size Laptop button: approximately `+554`

The overlay ignored `g_ui.isMapFullSize()` and did not share Laptop button position.

### Fix — `src/game/Strategic/MapScreen.cc`

Compute one Laptop anchor:

```c++
INT32 const lapX = (g_ui.isMapFullSize() ? 554 : 456) + g_ui.get_MAP_BOTTOM_BASE_X();
INT32 const lapY = 410 + g_ui.get_MAP_BOTTOM_BASE_Y();
```

Use anchor for all overlay paths:

- blinking icon, normal state
- pressed icon state
- disabled hatch rectangle
- invalidation regions

Normal layout remains byte-equivalent in visual position (`lapX + 8`, `lapY + 7` = old `+464`, `+417`). Full-size layout now places icon on Laptop button instead of finance area.

---

## 3. VALIDATION

### Static checks

```bash
git diff --check
```

Expected: no output.

Native C++ changes require Android rebuild:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
./tools/ogvm-emu-install.sh
```

Or:

```bash
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

### Smoke checklist

1. [ ] Open AIM video conference through hire flow; no selection-light crash during mode changes.
2. [x] Normal mapscreen new-mail icon stays on Laptop button.
3. [x] Full-size mapscreen new-mail icon stays on Laptop button.
4. [x] Icon no longer overlaps finance box.
5. [ ] Verify disabled Laptop hatch and pressed icon state after rebuild.
6. [ ] Confirm no fresh fatal signal in logcat.

Log filter:

```bash
adb logcat -d | grep -iE 'Fatal signal|SIGSEGV|AIMMembers|MapScreen|libja2'
```

---

## 4. FILES

| File | Change |
|---|---|
| `src/game/Laptop/AIMMembers.cc` | Null guard for selection-light button refs |
| `src/game/Strategic/MapScreen.cc` | Anchor blinking mail icon to Laptop button |
| `docs/HANDOFF-08-08-2026-aim-map-overlay.md` | This handoff |

---

## 5. NEXT SESSION

1. Rebuild/install Android APK.
2. Verify AIM hire/video-conference transition and selection lights.
3. Verify map new-mail icon in normal and full-size layouts.
4. If AIM still crashes, capture stack with `AIMMembers.cc` in frame.
5. Continue backlog from prior handoffs: desktop GIO checkbox restore, slider thumb stretch, laptop integer font scale, per-edition `imp.json`.

---

## 6. COMMIT MESSAGE

```text
OGVM-ANDROID: guard AIM selection and anchor map mail icon

Guard AIM selection-light rendering until all video-conference buttons exist.
Anchor mapscreen blinking new-mail icon to Laptop button so full-size layout
stays clear of finance box while normal layout keeps existing coordinates.

Handoff 08-08 AIM map overlay.
```
