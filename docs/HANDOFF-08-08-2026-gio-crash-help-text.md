# HANDOFF — 08/08/2026

**GIO YES SEGV + laptop help body text + checkbox size**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | **Android** (`#ifdef __ANDROID__`) trừ khi ghi rõ |
| Emulator | `Pronunciation_API_35` |
| Package | `io.github.ja2stracciatella` |
| Launcher | `io.github.ja2stracciatella/.LauncherActivity` |
| Prior | `docs/HANDOFF-07-08-2026-gio-yes-crash-laptop.md`, `docs/HANDOFF-07-08-2026-mobile-gio-msgbox.md` |

**Git:** changes staged, **chưa commit/push** (Bash classifier blocked commit earlier). Next session: commit + `git push origin HEAD`.

---

## 0. TRẠNG THÁI

| Việc | Status | Note |
|---|---|---|
| GIO Novice YES → fade → intro | **DONE** | User smoke OK — không văng |
| Android `RefreshScreen` safe | **DONE** | FB full texture, no soft cursor |
| Laptop help body text trống | **DONE** | Force dirty mỗi frame khi scale |
| Help checkbox quá to | **DONE** | Skip 2× khi laptop scale active |
| Stretch blit / dirty restore clamp | **DONE** | SEGV/assert guard |
| MessageBox 2× size clamp | **DONE** | Short screens |
| Fade restore size clamp | **DONE** | SAVE/FB min |
| Desktop GIO checkbox restore | **OPEN** | `RestoreGIOButtonBackGrounds` no-op mọi platform |
| Font laptop nét fractional | **PARTIAL** | Nearest present; soft nếu non-integer scale |

---

## 1. GIO YES CRASH (SIGSEGV)

### Symptom
Start New Game → GIO → Novice → MessageBox YES → app crash/quit.

Logcat:
```
Fatal signal 11 (SIGSEGV) ... tid=... name=SDLThread
#00 pc ... libSDL2.so (SDL_UpdateTexture / blit)
...
RefreshScreen()+...
```
Fault address ~`0x1c` (null-ish / bad pointer).

### Root cause
Android GLES present path brittle during fade + MessageBox:

1. Partial dirty-rect `SDL_UpdateTexture` with bad rect / pitched pointer  
2. Software cursor blit when fade sets `VIDEO_NO_CURSOR` (w/h = 0) or dst OOB  
3. Stretch/restore rects past `SCREEN_*` on 2× UI + short height (678)

### Fix — `src/sgp/Video.cc`

Android **early-return** in `RefreshScreen()`:

1. Null/size guards: `ScreenTexture`, `ScreenBuffer`, `FrameBuffer`, `GameRenderer`, pixels, pitch  
2. If fade active → `gFadeFunction()`  
3. If scrolling → FB→SB → scroll → SB→FB  
4. **No software cursor** on Android  
5. Full upload:  
   `SDL_UpdateTexture(ScreenTexture, NULL, FrameBuffer->pixels, FrameBuffer->pitch)`  
6. Present:  
   - If `AndroidLaptopGetPresentRects` valid → `RenderCopy` natural→fit  
   - Else full-screen `RenderCopy` (GIO / fade / msgbox)  
7. Clear dirty counters, **return** (desktop path untouched)

Desktop path kept: clamp dirty rects + soft cursor only if w/h > 0 + full texture upload.

Helper: `ClampSDLRectToSurface`.

Compile note: dropped `struct rect : SDL_Rect` + `operator+=` (cannot assign from `const SDL_Rect`).

### Related clamps

| File | Change |
|---|---|
| `src/sgp/VSurface.cc` | `BltStretchVideoSurface`: null/size, clamp dest into surface, reject OOB src |
| `src/game/TileEngine/Render_Dirty.cc` | `RestoreExternBackgroundRect`: Assert → clamp to SCREEN |
| `src/game/MessageBoxScreen.cc` | `usDispW/H` clamp to SCREEN; signed center math |
| `src/game/Fade_Screen.cc` | Restore w/h = min(SCREEN, SAVE, FB) |
| `src/game/GameInitOptionsScreen.cc` | Snapshot options on YES before fade (prior thread) |

### Smoke
YES on Novice → fade → intro video. User: **“ok được ròi”**.

---

## 2. LAPTOP HELP — MẤT CHỮ BODY

### Symptom
Help popup: title, sidebar (Overview/Email/…), scroll arrows, footer OK.  
**Body text area empty** (black).

### Root cause
Android laptop fit-scale redraws natural FB under help every frame:

```
LaptopScreenHandle
  → (scale) RenderLapTopImage + RenderLaptop + …
  → HelpScreenHandler
  → AndroidLaptopPresentScaled / InvalidateScreen
```

Help only redraws when `ubHelpScreenDirty != NOT_DIRTY`.  
Frame 1: paint text → clear dirty.  
Frame 2+: laptop redraw wipes FB under help; text buffer never re-blitted.  
Buttons still OK (`RenderButtons` every frame).

### Fix — `src/game/HelpScreen.cc` in `HelpScreenHandler`

```c
#ifdef __ANDROID__
// Fit-scale laptop redraws FB under help every frame. Dirty-once then
// leaves chrome/buttons only — body text never re-blitted.
if (gHelpScreen.bCurrentHelpScreen == HELP_SCREEN_LAPTOP && AndroidLaptopScaleActive())
{
    gHelpScreen.ubHelpScreenDirty = HLP_SCRN_DRTY_LVL_REFRESH_ALL;
}
#endif
```

before dirty check → `RenderHelpScreen` every frame under laptop scale.

Smoke: body text visible (user screenshot OK).

---

## 3. HELP CHECKBOX QUÁ TO

### Symptom
“Don’t show this type of help anymore” checkbox = large black square vs footer text (user circled).

### Root cause
`DrawCheckBoxButton` Android path stretches checkbox STI **2×** (for Options/GIO full-screen mobile UI).  

Help checkbox lives in laptop FB → already **fit-scale present** → double scale (2× draw + present stretch).

### Fix — `src/sgp/Button_System.cc`

```c
#ifdef __ANDROID__
#include "Laptop.h"
#endif

// DrawCheckBoxButton:
if ((b->uiFlags & BUTTON_CHECKBOX) && !AndroidLaptopScaleActive())
{
    // 2× stretch into ButtonDestBuffer …
}
```

| Context | Checkbox draw |
|---|---|
| Options / GIO (no laptop scale) | still 2× |
| Laptop help (scale active) | 1× in FB → present scales once |

Smoke: rebuild native needed; size should match footer line.

---

## 4. LAPTOP FIT-SCALE (context, prior)

Still in tree (not re-derived this session):

- FB laptop natural **640×480 @ STD_SCREEN**  
- Present: aspect-fit dst, letterbox OK  
- Touch: `AndroidLaptopMapScreenToLogical`  
- API: `AndroidLaptopScaleActive`, `AndroidLaptopGetPresentRects` in `Laptop.{h,cc}`  
- Video present uses those rects on Android path  

---

## 5. BUILD / INSTALL

Native C++ change **must** rebuild NDK — gradle up-to-date may skip.

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
adb logcat -d | grep -iE 'ja2|straccia|FATAL|AndroidRuntime' | tail -80
```

---

## 6. SMOKE CHECKLIST

1. [ ] Full native rebuild + install (not gradle-only)  
2. [ ] Main menu OK  
3. [ ] GIO open, no quit  
4. [ ] Novice → MessageBox 2× + dim → **YES** → fade → intro, **no crash**  
5. [ ] Laptop open: scale fit, touch OK  
6. [ ] Help (H / auto): **body text** visible, subpages OK  
7. [ ] Help checkbox size ≈ footer text (not giant black square)  
8. [ ] Options checkboxes still 2× (not laptop)  

---

## 7. FILES

| File | Change |
|---|---|
| `src/sgp/Video.cc` | Android FB-direct present; desktop clamp; no zero cursor |
| `src/sgp/VSurface.cc` | Stretch blit safety clamp |
| `src/sgp/Button_System.cc` | Skip checkbox 2× when `AndroidLaptopScaleActive` |
| `src/game/HelpScreen.cc` | Force `REFRESH_ALL` laptop+scale |
| `src/game/MessageBoxScreen.cc` | Size/center clamp |
| `src/game/TileEngine/Render_Dirty.cc` | Restore rect clamp |
| `src/game/Fade_Screen.cc` | Restore size clamp |
| `src/game/GameInitOptionsScreen.cc` | YES options snapshot (minor) |
| `docs/HANDOFF-07-08-2026-gio-yes-crash-laptop.md` | Pointer → this handoff |
| `docs/HANDOFF-08-08-2026-gio-crash-help-text.md` | This file |

---

## 8. NEXT SESSION

1. **Commit + push** (staged already if workspace intact):

```bash
git add docs/HANDOFF-08-08-2026-gio-crash-help-text.md \
  docs/HANDOFF-07-08-2026-gio-yes-crash-laptop.md \
  src/game/Fade_Screen.cc src/game/GameInitOptionsScreen.cc \
  src/game/HelpScreen.cc src/game/MessageBoxScreen.cc \
  src/game/TileEngine/Render_Dirty.cc \
  src/sgp/Button_System.cc src/sgp/VSurface.cc src/sgp/Video.cc

git commit -m "$(cat <<'EOF'
OGVM-ANDROID: GIO YES SEGV + laptop help text/checkbox

RefreshScreen Android: present FrameBuffer full texture, no soft cursor
(dirty partial UpdateTexture + zero-size cursor SEGV on GLES fade/msgbox).
Help: force REFRESH_ALL each frame under laptop fit-scale so body text
survives full FB redraw. Checkbox 2x stretch skip when laptop scale active.
Clamp stretch blit, dirty restore, msgbox size, fade restore.
Handoff: docs/HANDOFF-08-08-2026-gio-crash-help-text.md

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"

git push -u origin HEAD
```

2. Rebuild + smoke §6 (especially checkbox size if not retested after last fix).  
3. Optional: desktop GIO checkbox restore only non-Android if ghosting.  
4. Optional: laptop font integer scale when room.

---

## 9. COMMIT MESSAGE (short)

```
OGVM-ANDROID: GIO YES SEGV + laptop help text/checkbox

RefreshScreen Android: FB full texture, no soft cursor.
Help force dirty under laptop fit-scale; skip checkbox 2x when scale.
Clamp stretch/dirty/msgbox/fade. Handoff 08-08.
```
