# HANDOFF — 07/08/2026, Mobile GIO scale + MessageBox 2× + dim overlay

Branch: `feature/multi-edition-detector`  
Repo: `Ethanhoangdesign/OldGameVM`  
Simulator: **Pronunciation_API_35**  
Scope: **Android only** (`#ifdef __ANDROID__`). Desktop UI unchanged.

## 1. Goal

- Enlarge **Initial Game Settings (GIO)** for touch readability.
- Enlarge **MessageBox** (e.g. Novice confirm) **2×**.
- Dark **modal overlay** behind MessageBox (~90%+ dim).
- Align YES/NO button **art + label** after hit-area scale.
- Fix crash on Start New Game when scale pushed buttons past screen height.

## 2. Code status (DONE)

### `src/game/GameInitOptionsScreen.cc`
- Android layout scale from screen center:
  - `GIO_SCALE_X/Y` = **2×** (was 3.1/2.8 → OK/Cancel Y overflow on 768p → native crash).
  - Cast to `INT16` so float scale does not fail `-Werror`.
- Larger gaps / toggle offsets for touch.
- Title / toggle text: `gpHugeFont` with **fallback** to `FONT16ARIAL` if null.
- OK Y clamp: keep button fully on-screen (`SCREEN_HEIGHT - 48`).
- GIO bg: 8bpp STI → temp 16bpp surface → `BltStretchVideoSurface` full frame.
- Full-screen `guiSAVEBUFFER` capture (from prior handoff).

### `src/game/Utils/Font_Control.cc`
- Always try load `hugefont.sti` on Android.
- `try/catch` → `gpHugeFont = nullptr` if missing (no abort).

### `src/game/MessageBoxScreen.cc`
- `MSGBOX_UI_SCALE 2` on Android.
- Content built 1× via `PrepareMercPopupBox`; **display** size/position use 2×.
- Full-screen save buffer; each frame: restore → `ShadowRect` ×5 (~97% dark) → stretch box 2× → `InvalidateRegion(0,0,SCREEN_WIDTH,SCREEN_HEIGHT)` (partial dirty dropped dim).
- YES/NO hit areas 2×; button font `FONT16ARIAL`.
- Exit restore uses full-screen buffer on Android.

### `src/sgp/Button_System.cc`
- Checkbox draw: stretch 2× via temp 16bpp + `BltStretchVideoSurface` (only if dest rect in bounds).
- Quick buttons: if hit `W/H` larger than art (msgbox YES/NO), **stretch art to hit box** so label centers on graphic.
- Casts for `UINT16` / `UseImage` (clang `-Wc++11-narrowing`).

### `src/sgp/VSurface.h`
- Free declaration of `BltStretchVideoSurface` (friend alone not enough for call sites).

### `src/game/MainMenuScreen.cc`
- Main menu bg stretch full screen (same 16bpp temp pattern).

### `tools/ogvm-emu-install.sh`
- One-shot: start AVD if needed → clean → build → install → launch.

## 3. Smoke (manual)

1. Emulator `Pronunciation_API_35` online.
2. Main menu fills width (no black letterbox sides).
3. **Start New Game** → GIO opens (no crash); text/checkboxes larger.
4. Pick **Novice** → MessageBox ~2×, background dark, YES/NO large and aligned.
5. Desktop/mac build: GIO + MessageBox still original 1×.

## 4. One-liner build / install

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella" && export ANDROID_HOME="$HOME/Library/Android/sdk" && export PATH="$ANDROID_HOME/platform-tools:$ANDROID_HOME/emulator:$PATH" && (adb devices 2>/dev/null | awk 'NR>1 && $2=="device"{found=1} END{exit !found}' || (nohup emulator -avd Pronunciation_API_35 -netdelay none -netspeed full -no-snapshot-save >/tmp/ogvm-emulator.log 2>&1 & adb wait-for-device && until [ "$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; do sleep 2; done)) && ./android/gradlew -p android clean && ./tools/build-android-debug.sh && adb install -r "android/app/build/outputs/apk/debug/app-debug.apk" && adb shell am force-stop io.github.ja2stracciatella && adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Or: `./tools/ogvm-emu-install.sh`

## 5. Known limits / next

- MessageBox body text still drawn at 1× then stretched (pixelated but readable). True large font inside `MercTextBox` is a follow-up.
- GIO scale fixed at 2× for 768p safety; taller phones could use dynamic scale later.
- Checkbox stretch is global for `BUTTON_CHECKBOX` on Android (fine for GIO; watch other screens).
- Prior related docs: `docs/HANDOFF-06-08-2026-main-menu-gio-savebuf.md`, `docs/HANDOFF-06-08-2026-gio-bg-stretch.md`.
