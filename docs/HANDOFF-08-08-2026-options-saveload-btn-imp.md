# HANDOFF — 08/08/2026

**Options + SaveLoad fit-scale, popup button chrome, IMP YES hire fix**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | **Android** (`#ifdef __ANDROID__`) trừ IMP hire logic (mọi platform) |
| Emulator | `Pronunciation_API_35` |
| Package | `io.github.ja2stracciatella` |
| Launcher | `io.github.ja2stracciatella/.LauncherActivity` |
| Prior | `docs/HANDOFF-08-08-2026-map-help-scale-imp.md` |
| Supersedes notes | `docs/HANDOFF-08-08-2026-options-scale.md` (draft, replace with this) |

**Git:** commit + push end of this handoff.

---

## 0. TRẠNG THÁI

| Việc | Status | Note |
|---|---|---|
| Map help 2× + IMP CIA003 | DONE (prior) | |
| Options fit-scale + black under | **DONE** | user smoke OK |
| Options labels wrap HugeFont | **DONE** | labels `FONT14ARIAL` |
| Save/Load fit-scale + dim ~80% | **DONE** | ShadowRect ×2 underlay |
| Popup bottom buttons bold + white | **DONE** | humanist + `FONT_MCOLOR_WHITE` |
| Help side tabs bold white (not yellow) | **DONE** | `FONT10ARIALBOLD` |
| IMP YES, I DO. silent fail | **DONE** | HireMerc INT8≠BOOLEAN + popups |
| Desktop GIO checkbox restore | OPEN | backlog |
| Slider thumb art stretch | OPEN | track scaled, thumb 1× |
| Laptop font integer scale | PARTIAL | backlog |
| Per-edition `imp.json` | OPEN | backlog |

---

## 1. OPTIONS SCREEN (Android fit-scale)

### Symptom
Map → Options panel ~1× natural, too small on phone. Need ~frame fill + black under.

### Approach (`Options_Screen.cc`)
Same family as **GIO** (coord scale from screen center), not MessageBox compose-stretch:

1. `OptUiScale()` = `min(2, SCREEN_W/640, SCREEN_H/480)`.
2. `OptSX/Y(x)` = center + `(x - STD_SCREEN center) * scale`.
3. Layout macros (buttons, toggles, sliders, text) via `OptSX/Y` + scaled gaps.
4. Render: **black full FB** → stretch STI base/header (8→16 temp → `BltStretchVideoSurface`).
5. Buttons: hit enlarge `pic * scale`; `Button_System` Android stretches art when hit > pic.
6. Checkboxes: hit 2× (matches global `DrawCheckBoxButton` 2× stretch).
7. After panel paint: FB→`guiSAVEBUFFER` so slider restore has black+panel underlay.

### Fonts
| Role | Android | Desktop |
|---|---|---|
| Checkbox / slider labels | `FONT14ARIAL` (`OPT_MAIN_FONT`) | `FONT12ARIAL` |
| Bottom Save/Load/Quit/Done | `FONT14HUMANIST` (`OPT_BTN_FONT`) | `OPT_BUTTON_FONT` |
| Label color | `73` gray (`OPT_MAIN_COLOR` fixed, **not** button color) | same |
| Button color | via `OPT_BUTTON_*` white | `73` |

### Build fail fixed earlier
`SaveLoadScreen.cc`: missing `PIXEL_DEPTH` → `#include "Local.h"`.

---

## 2. SAVE / LOAD SCREEN (Android)

### Symptom
Save Game dialog small over Options; need large + black under ~alpha 0.8.

### Approach (`SaveLoadScreen.cc`)
- `SlgUiScale` / `SlgSX` / `SlgSY` / `SlgS` (signed: skull Y can be negative).
- Capture full-frame underlay on entry → restore + `ShadowRect` × `SLG_DIM_PASSES` (2) ≈ 80% dim.
- Stretch loadscreen / header / slots / scroll chrome.
- Text input size scaled; buttons hit enlarge; font `FONT14ARIAL` body, `FONT14HUMANIST` btn.

---

## 3. POPUP BUTTON CHROME (bold + white)

### Request
Bottom Options row (and all popup buttons) **bold + yellow** → user then: **white**, keep bold. Help tabs: white + bold, smaller (~10).

### Shared macros — `Options_Screen.h`
```c
#ifdef __ANDROID__
#define OPT_BUTTON_FONT		FONT14HUMANIST   // heavier than Arial (no 16pt bold STI)
#define OPT_BUTTON_ON_COLOR	FONT_MCOLOR_WHITE
#define OPT_BUTTON_OFF_COLOR	FONT_MCOLOR_WHITE
#else
#define OPT_BUTTON_FONT		FONT14ARIAL
#define OPT_BUTTON_ON_COLOR	73
#define OPT_BUTTON_OFF_COLOR	73
#endif
```

### Call sites
| Screen | Change |
|---|---|
| Options bottom | `OPT_BTN_FONT` + `OPT_BUTTON_*` |
| SaveLoad Save/Cancel | `SLG_BTN_FONT` + `OPT_BUTTON_*` |
| MessageBox YES/NO/OK… | Android: `FONT14HUMANIST` + white (ignore style colour) |
| GIO OK/Cancel | `OPT_BUTTON_*` only (drop HugeFont Android special) |
| Help side tabs | Android: `FONT10ARIALBOLD` + white (narrow chrome; 14 wraps) |

**Note:** only true bold STI is `FONT10ARIALBOLD`. Larger “bold look” = `FONT14HUMANIST`.

---

## 4. IMP CONFIRM YES (hire silent fail)

### Symptom
Laptop IMP payment: **YES, I DO.** no reaction (NO works). Balance $45k, team has mercs.

### Root cause (`IMP_Confirm.cc`)
1. `HireMerc` returns **`INT8`**: `MERC_HIRE_OK=1`, `FAILED=0`, `OVER_20=-1`.
2. Code did `BOOLEAN f = HireMerc(...)` then `if (!f)`.  
   **`-1` is truthy as BOOLEAN** → team-full path treated as success, set `fIMPCompletedFlag`, then `FindSoldier` null → silent return **after** sticky complete.
3. Money / already-complete / hire fail paths returned **without popup** (MainPage already has popups for money/team).

### Fix
- `AddCharacterToPlayersTeam` → return `INT8`; compare `== MERC_HIRE_OK`.
- Set `fIMPCompletedFlag = TRUE` only after hire OK **and** soldier found.
- Popups: money `[3]`, team full `[5]`, already done `[6]`, other fail `[4]`.
- Clamp `iPortraitNumber` for face table; reset weird `bMercStatus` before hire.
- `#include "Overhead.h"` for `NumberOfMercsOnPlayerTeam`.

### Smoke
1. Finish IMP profile → Confirm → **YES, I DO.**  
2. Success: −$3000, back IMP home, merc in transit / team.  
3. Fail cases: popup, no charge, can retry.

---

## 5. FILES THIS SESSION

| File | Change |
|---|---|
| `src/game/Options_Screen.cc` | Fit-scale, black under, fonts, hits, SAVEBUFFER |
| `src/game/Options_Screen.h` | Android `OPT_BUTTON_*` humanist+white |
| `src/game/SaveLoadScreen.cc` | Fit-scale, dim underlay, stretch, fonts |
| `src/game/MessageBoxScreen.cc` | Android btn humanist+white |
| `src/game/GameInitOptionsScreen.cc` | GIO uses `OPT_BUTTON_*` only |
| `src/game/HelpScreen.cc` | Android help tabs 10 bold white |
| `src/game/Laptop/IMP_Confirm.cc` | Hire INT8 check + popups + face clamp |
| `docs/HANDOFF-08-08-2026-options-saveload-btn-imp.md` | This file |

---

## 6. BUILD / INSTALL

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
./tools/ogvm-emu-install.sh
# or:
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

---

## 7. SMOKE CHECKLIST

1. [x] Options map: large panel + black under  
2. [x] Options labels ~14, no wrap on circled text  
3. [x] Save/Load large + dim ~80%  
4. [x] Popup buttons white + heavier font  
5. [x] Help tabs white bold 10  
6. [x] IMP YES hire works / fail shows popup  
7. [ ] Desktop options still 1× / gray buttons  
8. [ ] MessageBox YES/NO still 2× dim  

---

## 8. COMMIT MESSAGE

```
OGVM-ANDROID: options/saveload scale + popup btn chrome + IMP YES hire

Options/SaveLoad: fit-scale from screen center (cap 2x), black/dim
underlay, stretch chrome, enlarge hits. Labels FONT14; buttons
FONT14HUMANIST white on Android (desktop gray Arial). MessageBox/GIO
share OPT_BUTTON_*; Help tabs FONT10ARIALBOLD white.

IMP Confirm YES: HireMerc is INT8 not BOOLEAN (-1 team-full was
sticky-true); only set fIMPCompletedFlag after hire OK + soldier
found; surface money/team/fail via existing pImpPopUpStrings.

Handoff 08-08 options-saveload-btn-imp.
```

---

## 9. NEXT (optional backlog)

- Desktop GIO checkbox restore (prior).  
- Slider thumb stretch in `Slider.cc`.  
- Laptop font integer scale.  
- Per-edition `imp.json` activation codes.  
- Delete draft `docs/HANDOFF-08-08-2026-options-scale.md` if still around (superseded).
