# HANDOFF — 08/08/2026

**Map help 2× + IMP Wildfire code + checkbox align**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | **Android** (`#ifdef __ANDROID__`) trừ IMP codes (mọi platform) |
| Emulator | `Pronunciation_API_35` |
| Package | `io.github.ja2stracciatella` |
| Launcher | `io.github.ja2stracciatella/.LauncherActivity` |
| Prior | `docs/HANDOFF-08-08-2026-gio-crash-help-text.md` |

**Git:** commit + push end of this handoff.

---

## 0. TRẠNG THÁI

| Việc | Status | Note |
|---|---|---|
| GIO YES SEGV | DONE (prior) | User smoke OK |
| Laptop help body text | DONE (prior) | Force dirty under fit-scale |
| Laptop help checkbox size | DONE (prior) | Skip 2× when laptop scale |
| Map help popup quá nhỏ | **DONE** | 2× compose→stretch (MessageBox pattern) |
| Map help SEGV (`Blt8` OOB) | **DONE** | Compose size = STI; load STI before buttons |
| Map help checkbox lệch dưới chrome | **DONE** | Footer Y from `natH`, no pad |
| IMP Wildfire `CIA003` reject | **DONE** | Add to `imp.json` activation_codes |
| Desktop GIO checkbox restore | OPEN | prior backlog |
| Font laptop nét fractional | PARTIAL | prior backlog |

---

## 1. IMP ACTIVATION CODE (Wildfire)

### Symptom
Email Psych Pro Inc shows **SECRET Activation Code: `CIA003`**.  
IMP homepage rejects `CIA003`, accepts vanilla **`XEP624`**.

### Root cause
- Email text = game data (Wildfire BinaryData / string tables).
- Engine accept list = `assets/externalized/imp.json` only (`DefaultIMPPolicy::isCodeAccepted`).
- Vanilla list: `XEP624` / `xep624` only.

### Fix — `assets/externalized/imp.json`

```json
"activation_codes": ["XEP624", "xep624", "CIA003", "cia003"],
```

Validate path: `IMP_HomePage` → `GCM->getIMPPolicy()->isCodeAccepted(str)`.

→ skipped: per-edition `imp.json` override, add when multi-edition data packs ship.

Smoke: type `CIA003` on IMP homepage → enter profile flow.

---

## 2. MAP / TACTICAL HELP 2× (Android)

### Symptom
Map help (“You’re not in Arulco…”, “no team members hired…”) ~1× natural — too small on phone vs MessageBox 2× / laptop fit-scale.

### Approach
Same pattern as MessageBox:

1. Render STI + title/body/footer into **natural compose** surface (`gHelpNaturalW×H`).
2. `BltStretchVideoSurface` compose → FB at display rect (`× HELP_UI_SCALE`, clamp screen).
3. Buttons/checkbox/scroll **hit areas** at display coords (`loc + offset * scale`, hit enlarged).
4. Art for enlarged hits: existing `Button_System` Android stretch when `W/H > pic`.

### Active when
`HelpUiScaleActive()`:

- **Yes:** mapscreen / tactical / no-one-hired / not-in-Arulco / sector inventory  
- **No:** laptop (fit-scale present), options, load game, none  

Laptop path unchanged (prior force-dirty under `AndroidLaptopScaleActive`).

### Key APIs in `HelpScreen.cc`
| Helper | Role |
|---|---|
| `HelpUiScaleActive` | map/tactical only |
| `HelpScale` | 2 or 1 |
| `HelpDrawOriginX/Y` | 0,0 on compose when scaled |
| `HelpDrawDest` | compose vs `FRAME_BUFFER` |
| `HelpEnlargeButtonHit` | ×scale hit box |

### Draw path changes
- `DrawHelpScreenBackGround` → dest compose when scaled  
- `DisplayCurrentScreenTitleAndFooter` → `SetFontDestBuffer(HelpDrawDest())`  
- `RenderTextBufferToScreen` → blit text buffer to compose  
- Scroll chrome → compose; mouse drag maps display→natural  
- `RenderHelpScreen` end: stretch compose→FB, invalidate display rect  
- Force `REFRESH_ALL` each frame when scale active (map redraw wipe)

---

## 3. HELP SEGV (SIGSEGV in Blt8)

### Symptom
User reported Shut Down crash; log showed:

```
Fatal signal 11 (SIGSEGV) ... SDLThread
#00 Blt8BPPDataTo16BPPBufferTransparent
#01 BltVideoObject
#02 ...
#04 HelpScreenHandler
#05 MapScreenHandle
```

**Not** laptop Shut Down path. Map help open / redraw.

Noise: `libbluetooth_jni` abort on emulator — ignore.

### Root cause
Compose surface sized from `#define HELP_SCREEN_*_HEIGHT` **292**, STI art often **~300**.  
`BltVideoObject` STI → compose OOB → SEGV.

Also: buttons created **before** STI-sized rect recompute → wrong layout (secondary).

### Fix
1. Load `helpscreen.sti` **first**  
2. `gHelpNaturalW/H` from `SubregionProperties(HLP_SCRN_DEFAULT_TYPE)` (+ border if sidebar)  
3. `min` with define sizes; **no pad** (pad broke footer align — §4)  
4. Create compose, recompute display rect, **then** buttons / checkbox / scroll  
5. Null-guard `HelpDrawDest` / `guiHelpScreenBackGround` in `DrawHelpScreenBackGround`

---

## 4. CHECKBOX / FOOTER LỆCH DƯỚI CHROME

### Symptom
After 2× worked: checkbox + “Don't show me this type of help anymore” floated **below** gold footer bar (on map).

### Root cause
1. Temporary compose pad (`natH + 16`)  
2. Checkbox Y used `usScreenLocY + usScreenHeight - OFFSET*sc` — display height after clamp/pad ≠ STI bottom  

Footer text drawn natural: `drawY + panelH - OFFSET_Y` on compose → stretch OK.  
Checkbox used padded display height → mismatch.

### Fix
- Drop pad; natural = STI only  
- Checkbox:

```c
usPosY = usScreenLocY + (natH - HELP_SCREEN_SHOW_HELP_AGAIN_REGION_OFFSET_Y) * sc;
```

with `natH = gHelpNaturalH` when scaled. Aligns with footer text after stretch.

User smoke: **“ok ngon rồi”**.

---

## 5. BUILD / INSTALL

Native C++ change **must** rebuild NDK.

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
./tools/ogvm-emu-install.sh
# or:
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Log:

```bash
export ANDROID_HOME=$HOME/Library/Android/sdk
export PATH=$ANDROID_HOME/platform-tools:$PATH
adb logcat -c
# repro
adb logcat -d | grep -iE 'Fatal signal|HelpScreen|Blt8|libja2|RefreshScreen' | tail -40
```

`imp.json` ships in APK assets — reinstall enough (no pure-data hot reload assumed).

---

## 6. SMOKE CHECKLIST

1. [ ] Full native rebuild + install  
2. [ ] Map: auto help “no team hired” / “not in Arulco” — **~2×**, readable, **no crash**  
3. [ ] Checkbox + footer text **inside** bottom chrome (not below panel)  
4. [ ] X close + checkbox hit OK  
5. [ ] Laptop help still OK (fit-scale; body text; checkbox not giant)  
6. [ ] IMP: email `CIA003` accepted; vanilla `XEP624` still OK  
7. [ ] Laptop Shut Down (confirm no SEGV — prior log was help, not shutdown)  
8. [ ] GIO YES → fade → intro still OK (prior)

---

## 7. FILES

| File | Change |
|---|---|
| `src/game/HelpScreen.cc` | Android map/tactical 2× compose+stretch; STI-sized surface; footer/checkbox align; force dirty |
| `assets/externalized/imp.json` | `CIA003` / `cia003` activation codes |
| `docs/HANDOFF-08-08-2026-map-help-scale-imp.md` | This file |

---

## 8. NEXT SESSION

1. Optional: desktop GIO checkbox restore only non-Android if ghosting (prior OPEN).  
2. Optional: laptop font integer scale when room.  
3. Optional: per-edition `imp.json` when multi-edition data packs exist.  
4. If Shut Down still crashes after help fix: fresh logcat focused on laptop exit path (not HelpScreen stack).

---

## 9. COMMIT MESSAGE

```
OGVM-ANDROID: map help 2x + IMP Wildfire CIA003

Map/tactical help: natural compose + 2x stretch (MessageBox pattern);
size compose from STI to avoid Blt8 OOB SEGV; checkbox/footer from
natural height so tick sits on chrome. imp.json: accept CIA003
(Wildfire email) alongside XEP624. Handoff 08-08 map-help-scale-imp.
```
