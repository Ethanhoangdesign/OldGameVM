# HANDOFF — 07/08/2026, Laptop scale + GIO YES exit

Branch: `feature/multi-edition-detector`  
Repo local: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`  
Scope: **Android only** (`#ifdef __ANDROID__`) trừ khi ghi rõ.  
Simulator: `Pronunciation_API_35`  
Package: `io.github.ja2stracciatella`  
Launcher: `io.github.ja2stracciatella/.LauncherActivity`

---

## 0. TRẠNG THÁI

| Việc | Trạng thái |
|---|---|
| Laptop fit-scale (aspect, letterbox đen OK) | DONE — FB 1× + SDL nearest present |
| Touch map laptop scaled | DONE — `Input.cc` → `AndroidLaptopMapScreenToLogical` |
| GIO 2× + MessageBox 2× + dim | DONE — handoff `docs/HANDOFF-07-08-2026-mobile-gio-msgbox.md` |
| GIO YES (Novice confirm) exit path | DONE — see `docs/HANDOFF-08-08-2026-gio-crash-help-text.md` |
| Laptop help body text + checkbox size | DONE — same handoff 08/08 |
| Font laptop nét mọi tỷ lệ | PARTIAL — nearest giúp; bitmap stretch vẫn soft nếu fractional |
| Desktop/Mac GIO restore checkbox | THAY ĐỔI — `RestoreGIOButtonBackGrounds` empty **mọi platform** (xem §3) |

**Superseded 08/08:** full write-up + help fixes → `docs/HANDOFF-08-08-2026-gio-crash-help-text.md`.

---

## 1. BỐI CẢNH SESSION

1. User muốn **toàn laptop UI** to trên Android, giữ aspect, letterbox OK.  
2. Reject whole-game SDL scale; reject laptop post-process stretch vỡ dirty-rect.  
3. Scale 1664×678: integer scale = 1× → đổi **fractional aspect-fit**.  
4. Font mờ: chuyển present sang **SDL nearest** (không soft software stretch + LINEAR).  
5. YES trên MessageBox xác nhận độ khó Novice → app quit / bad exit.

---

## 2. LAPTOP SCALE (đã có trong code)

### Files
- `src/game/Laptop/Laptop.h` — API Android present/touch
- `src/game/Laptop/Laptop.cc` — `AndroidLaptopComputeScale`, enter/exit, skip dirty save/restore khi scale
- `src/sgp/Video.cc` — `AndroidLaptopGetPresentRects` → `SDL_SetTextureScaleMode(Nearest)` + `SDL_RenderCopy` src/dst
- `src/sgp/Input.cc` — map touch/mouse → logical khi laptop scale active

### Cơ chế
- FB laptop vẽ **natural 640×480 @ STD_SCREEN**
- Mỗi frame full redraw (không dirty-rect) khi trong laptop scaled
- Present: rect natural → dst fit màn hình, keep aspect, letterbox
- Touch: screen → logical trong rect laptop

### Giới hạn
- Font bitmap + scale fractional → vẫn soft hơn integer scale / font native 2× (GIO/MsgBox)

---

## 3. GIO YES EXIT — PATCH

File: `src/game/GameInitOptionsScreen.cc`

### 3.1 Nguyên nhân nghi
1. **`RestoreExternBackgroundRect(..., 68, 58)`** mỗi frame trong `RestoreGIOButtonBackGrounds`  
   - Android GIO 2×: gap 70, hit 68×58  
   - Màn ngắn (vd height 678) → rect dưới/cạnh ra ngoài SCREEN → hard fail trong dirty restore  
2. **`DoneFadeOutForExitGameInitOptionScreen`** đọc live button sau fade; button có thể đã teardown / state lạ

### 3.2 Fix đã apply
```c
// Snapshot trước fade
static BOOLEAN gfGIOExitOptionsSaved;
static GAME_OPTIONS gGIOExitOptions;
static void SaveGIOExitOptions(void); // đọc GetCurrent*ButtonSetting() → gGIOExitOptions

// HandleGIOScreen GIO_EXIT:
DisableButton(guiGIODoneButton);
SaveGIOExitOptions();                 // NEW
gFadeOutDoneCallback = DoneFadeOut...
FadeOutNextFrame();

// DoneFadeOut:
if (!gfGIOExitOptionsSaved) SaveGIOExitOptions();
gGameOptions = gGIOExitOptions;
// ... INTRO_SCREEN / SAVE_LOAD ...
ExitGIOScreen();

// RestoreGIOButtonBackGrounds: NO-OP
// GIO mỗi re-render đã full bg stretch; restore past screen edge = bad exit on Android scale
```

### 3.3 Side effect desktop
`RestoreGIOButtonBackGrounds` empty **không bọc `#ifdef __ANDROID__`**.  
Desktop: checkbox dirty có thể để vệt nếu không full re-render. GIO vẫn `RenderGIOScreen` + full bg khi `gfReRenderGIOScreen`.  
**Next (nếu desktop regress):** restore chỉ desktop, hoặc clamp Android như bản restore-clamp trước khi no-op.

### 3.4 Confirm YES path (sau patch)
```
MessageBox YES
→ ConfirmGioDifSettingMessageBoxCallBack → GIO_EXIT
→ HandleGIOScreen: SaveGIOExitOptions + fade
→ DoneFadeOut: gGameOptions = snapshot → ExitGIOScreen → INTRO_SCREEN
```

**Chưa snapshot ngay trong callback YES** — snapshot ở đầu fade (button còn sống). An toàn hơn đọc sau fade.

---

## 4. BUILD / EMULATOR (đúng package)

### Sai lệnh (session này)
```bash
# SAI — package không tồn tại
adb shell monkey -p org.ja2.stracciatella ...
```

### Đúng
```bash
PKG=io.github.ja2stracciatella
ACT=$PKG/.LauncherActivity
```

### One-shot (repo đã có)
```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
./tools/ogvm-emu-install.sh
# hoặc:
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

### Lưu ý quan trọng
- `./gradlew installDebug` **up-to-date** có thể **không rebuild native** C++.  
  Fix trong `.cc` → phải **clean + `build-android-debug.sh`** (hoặc force NDK rebuild) rồi install lại.
- Desktop `cmake --build build` **không** đưa code vào APK Android.

### Log repro
```bash
export ANDROID_HOME=$HOME/Library/Android/sdk
export PATH=$ANDROID_HOME/platform-tools:$PATH
adb devices   # phải thấy device, không kẹt "waiting for device"
adb logcat -c
# reproduce YES exit
adb logcat -d | grep -iE 'ja2|straccia|FATAL|AndroidRuntime' | tail -80
```

Nếu emulator die (`Netsim Wifi ... CANCELLED`) → restart AVD rồi `adb wait-for-device`.

---

## 5. SMOKE CHECKLIST

1. Full native rebuild + install (không chỉ gradle up-to-date).  
2. Main menu full width.  
3. Start New Game → GIO mở, không quit.  
4. Novice → MessageBox 2× + dim → **YES** → fade → intro/video, **không văng**.  
5. Laptop in-game: scale fit, touch hit đúng.  
6. (Optional) Desktop GIO checkbox không ghost.

---

## 6. NEXT SESSION

1. **Rebuild native + smoke YES** (`./tools/ogvm-emu-install.sh`).  
2. Nếu còn SEGV: logcat offset mới trong `RefreshScreen`; check intro/Smk.  
3. Snapshot YES đã có trong `ConfirmGioDifSettingMessageBoxCallBack`.  
4. Desktop: restore GIO checkbox chỉ non-Android nếu regress.  
5. Font laptop nét hơn: integer scale khi đủ chỗ (scope riêng).

---

## 7. FILE ĐỤNG SESSION

| File | Thay đổi |
|---|---|
| `src/game/GameInitOptionsScreen.cc` | snapshot exit options; restore no-op; (sạch rác tail) |
| `src/game/Laptop/Laptop.{h,cc}` | fit-scale + present/touch API (session trước cùng thread) |
| `src/sgp/Video.cc` | SDL nearest laptop present |
| `src/sgp/Input.cc` | map touch logical |
| `docs/HANDOFF-07-08-2026-gio-yes-crash-laptop.md` | handoff này |

Prior: `docs/HANDOFF-07-08-2026-mobile-gio-msgbox.md`

---

## 8. COMMIT GỢI Ý (chưa commit)

```
OGVM-ANDROID: GIO YES exit — snapshot options + skip off-screen restore

RestoreGIOButtonBackGrounds no-op (2× hit rect past edge on short screens).
SaveGIOExitOptions before fade; DoneFadeOut applies snapshot only.
```
