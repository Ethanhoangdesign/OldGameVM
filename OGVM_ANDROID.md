# OGVM — Handoff: Android build / install / emulator (06/08/2026)

Repo: `Ethanhoangdesign/OldGameVM` → `origin`  
Local: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`  
Branch: `feature/multi-edition-detector`  
Version file: `0.23.0`  
May build: macOS arm64  
Phone: Samsung **SM_A175F** (A16/A17 family) arm64-v8a  
Emulator: AVD `Pronunciation_API_35` (API 35, arm64, Android 15)  
Upstream stracciatella: `upstream` remote  

Desktop handoff (controller / WF layout): `OGVM_HANDOFF.md`, `docs/HANDOFF-27-07-2026-toi.md`.

### Session 2–3 (06/08) — da ship trong branch

| Feature | Ghi chu |
|---|---|
| Res presets | 1024, 1360, 1366, **1664 mobile-only**, 640 + Custom |
| Default res | 1024×768 neu thieu `ja2.json` |
| Controller | Enable + L/R stick → `files/.ja2/controller.ini` (engine native) |
| Launcher rotate | `fullUser` + recreate; game van `sensorLandscape` |
| 1664×768 | Chi Android spinner; UILayout expand 2 ben, **khong** doi C++ / desktop RESLIST |

---

## 0. TRANG THAI

| Viec | Trang thai |
|---|---|
| Android app tree `android/` | Co san upstream (Kotlin + CMake native) |
| rust-std `aarch64-linux-android` | OK (cai tay — rustup cham) |
| NDK `25.0.8775105` | OK |
| Platform **base** android-33 (`IsBaseSdk=true`) | OK (can cho aapt2 AGP 7.4) |
| Ninja nested CMake | OK (`PATH` + `CMAKE_MAKE_PROGRAM`) |
| Debug APK OGVM arm64 | **DONE** ~36MB |
| Official APK 0.22.1 | **DONE** 42MB (fallback) |
| `adb install` phone SM_A175F | **DONE** Success |
| Emulator Mac AVD + install | **DONE** `emulator-5554` |
| Vanilla smoke (640×480) map screen | **DONE** |
| Wildfire data push + 1366×768 | **DONE** |
| Landscape lock khi choi game | **DONE** (manifest + override SDL) |
| Wide menu STI trong APK assets | **DONE local** (file `*.sti` gitignore — copy tu `build/`) |
| Game data tren phone that | **USER** — copy folder co `Data/` |
| Res presets 1024 / 1360 / 1366 / **1664** / 640 | **DONE** Settings spinner + freeform WxH (1664 mobile-only) |
| Controller enable + stick modes Android | **DONE** ghi `files/.ja2/controller.ini` (engine native) |
| Controller full remap UI | **Khong** (desktop FLTK only) |
| Launcher xoay doc/ngang theo may | **DONE** `fullUser` + recreate (game van landscape) |

### APK paths

| APK | Path | Size |
|---|---|---|
| OGVM debug arm64 | `android/app/build/outputs/apk/debug/app-debug.apk` | ~36MB |
| Copy de chia se | `~/Downloads/ja2-android/ja2-ogvm-debug-arm64.apk` | same |
| Official 0.22.1 | `~/Downloads/ja2-android/ja2-stracciatella_0.22.1-git+11e9430_android.apk` | 42MB |

Native trong OGVM APK:
- `lib/arm64-v8a/libja2.so`
- `lib/arm64-v8a/libSDL2.so`

---

## 1. BOI CANH SESSION (06/08)

1. Build/install Android tu branch OGVM (multi-edition + desktop WF layout).
2. Chay **emulator Mac** (khong chi phone USB).
3. Smoke vanilla → map screen OK.
4. Chuyen **Wildfire GOG 6.08** + res **1366×768** (giong desktop `~/.ja2/ja2.json`).
5. **Ep landscape** khi vao game (phone/emu mac dinh doc → UI letterbox sai).

Upstream Android: Kotlin launcher + SDL + `libja2.so`. Khong viet app moi.

---

## 2. DIFF LOCAL (Android)

### 2.1 `android/app/build.gradle`

- `compileSdkVersion` **33** (AGP 7.4.2 aapt2 **khong** load `android-35`).
- `buildToolsVersion "31.0.0"`.
- `ndkVersion "25.0.8775105"`.
- `abiFilters` → **chi** `'arm64-v8a'` (A16 + emu arm64 Mac; build nhanh). Full CI: `armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`.
- CMake args them:
  ```
  -DCMAKE_MAKE_PROGRAM=${ANDROID_HOME}/cmake/3.22.1/bin/ninja
  ```

### 2.2 `android/gradle.properties`

```
android.suppressUnsupportedCompileSdk=35
```

### 2.3 Landscape lock GAME (bat buoc cho 1366×768)

**Van de:** SDL `setOrientationBis` khi `resizable=true` + hint rong → `SCREEN_ORIENTATION_FULL_SENSOR` → man hinh doc, game crash/layout vo.

**Fix game (`StracciatellaActivity` only):**

1. Manifest: `android:screenOrientation="sensorLandscape"` + `configChanges=orientation|screenSize|keyboardHidden`
2. `StracciatellaActivity.kt`:
   - `onCreate`: `requestedOrientation = SENSOR_LANDSCAPE`
   - **override `setOrientationBis`** → luon `SENSOR_LANDSCAPE` (chan SDL flip)

**Launcher xoay doc/ngang (session 2):**

1. Manifest `LauncherActivity`: `screenOrientation="fullUser"`, **khong** nuot `orientation` trong `configChanges` (de recreate + reflow layout).
2. `LauncherActivity`: `requestedOrientation = FULL_USER` o `onCreate` + `onResume` (sau khi thoat game landscape).

Emu: bat **Auto-rotate** (quick settings). Nut rotate emulator doi orientation device — app theo.

### 2.4 Wide main-menu STI (local, **gitignore `*.sti`**)

`MainMenuScreen.cc`: `SCREEN_WIDTH >= 1366` → `loadscreens/mainmenubackground_wide.sti`.  
Thieu file → crash:

```
SGPFile::openInVfs: Vfs_open "loadscreens/mainmenubackground_wide.sti": entity not found
```

Desktop co file o `build/externalized/LOADSCREENS/` (khong ship git — `*.sti` gitignore).

**Moi may build APK wide res:**

```bash
mkdir -p assets/externalized/loadscreens
cp -f build/externalized/LOADSCREENS/mainmenubackground_wide.sti \
      assets/externalized/loadscreens/
cp -f build/externalized/LOADSCREENS/mainmenubackground_1024.sti \
      assets/externalized/loadscreens/
# can desktop build it nhat 1 lan de co file trong build/
```

Android `sourceSets` pack `../../assets` → APK `assets/externalized/loadscreens/…`.  
VFS Android: layer `externalized` tu APK AssetManager.

### 2.5 `tools/build-android-debug.sh`

Export SDK/NDK/PATH, check rust target, `gradlew assembleDebug`.

### 2.6 `android/local.properties` (local only, gitignore)

```
sdk.dir=/Users/ethan/Library/Android/sdk
```

### 2.7 Khong commit

- `local.properties`, `*.apk`, keystore, `android/app/build/`, `.cxx/`
- `*.sti` (copyright / binary policy)
- `ogvm-smoke*.png` screenshot local
- `layout controller/` untracked (desktop)

---

## 3. BLOCKERS + FIX

| # | Loi | Fix |
|---|---|---|
| 1 | rustup target android treo | Curl rust-std aarch64-linux-android → install.sh |
| 2 | Nested CMake no Ninja | PATH cmake/3.22.1 + CMAKE_MAKE_PROGRAM |
| 3 | aapt2 android-35 RES_TABLE | compileSdk **33** base platform |
| 4 | platform-33-ext IsBaseSdk=false | Dung base `platform-33_r02` → `platforms/android-33` |
| 5 | 1366 crash missing wide STI | Copy STI vao `assets/externalized/loadscreens/` truoc assemble |
| 6 | SDL flip portrait | Override `setOrientationBis` + manifest landscape |
| 7 | StracciatellaActivity not exported | Launch qua Launcher FAB (khong `am start` truc tiep game) |

---

## 4. BUILD (Mac)

```bash
export ANDROID_HOME="$HOME/Library/Android/sdk"
export ANDROID_SDK_ROOT="$ANDROID_HOME"
export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/25.0.8775105"
export PATH="$ANDROID_HOME/cmake/3.22.1/bin:$ANDROID_HOME/platform-tools:$PATH"

cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"

# 1 lan: wide menu assets (neu can res >= 1024)
mkdir -p assets/externalized/loadscreens
cp -f build/externalized/LOADSCREENS/mainmenubackground_wide.sti assets/externalized/loadscreens/ 2>/dev/null || true
cp -f build/externalized/LOADSCREENS/mainmenubackground_1024.sti assets/externalized/loadscreens/ 2>/dev/null || true

./tools/build-android-debug.sh
# APK: android/app/build/outputs/apk/debug/app-debug.apk
```

Lan dau native: SDL ExternalProject + cargo — vai phut.

---

## 5. EMULATOR (Mac)

```bash
export PATH="$ANDROID_HOME/emulator:$ANDROID_HOME/platform-tools:$PATH"
emulator -list-avds
# Pronunciation_API_35

emulator -avd Pronunciation_API_35 -netdelay none -netspeed full &
adb wait-for-device
# wait boot
adb shell getprop sys.boot_completed   # = 1

adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

Log emulator: `/tmp/ogvm-emulator.log` (neu chay nohup).

Tat: `adb -s emulator-5554 emu kill`

---

## 6. GAME DATA

App **khong** kem data goc.

### Wildfire (khuyen nghi OGVM layout 1366)

Source da dung session nay:

```
/Users/ethan/Documents/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)
# hoac
/Users/ethan/Documents/kimi/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)
```

Push emulator:

```bash
adb shell rm -rf /sdcard/JA2
adb shell mkdir -p /sdcard/JA2
adb push "/Users/ethan/Documents/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)/Data" /sdcard/JA2/Data
# ~875MB
```

### Config `filesDir/.ja2/ja2.json` (giong desktop)

```json
{
  "game_dir": "/storage/emulated/0/JA2",
  "resversion": "ENGLISH",
  "res": "1366x768",
  "scaling": "NEAR_PERFECT",
  "debug": true
}
```

Ghi bang adb:

```bash
cat > /tmp/ja2.json <<'EOF'
{
  "game_dir": "/storage/emulated/0/JA2",
  "resversion": "ENGLISH",
  "res": "1366x768",
  "scaling": "NEAR_PERFECT",
  "debug": true
}
EOF
adb push /tmp/ja2.json /data/local/tmp/ja2.json
adb shell "run-as io.github.ja2stracciatella mkdir -p files/.ja2"
adb shell "cat /data/local/tmp/ja2.json | run-as io.github.ja2stracciatella sh -c 'cat > files/.ja2/ja2.json'"
```

**Luu y:** `game_dir` = folder **co** `Data/` ben trong (khong tro vao `Data/` truc tiep).  
Edition WF detect qua art (`interface/b_map.sti` 16-bit, v.v.) — `resversion` van `ENGLISH` (ngon ngu text), giong desktop.

### Vanilla (smoke cu)

GOG: `.../setup_jagged_alliance_2_26614298_gog_v4_(80537)/Data`  
`res`: `640x480` du cho smoke; map screen da verify.

### Quyen storage

```bash
adb shell pm grant io.github.ja2stracciatella android.permission.READ_EXTERNAL_STORAGE
adb shell pm grant io.github.ja2stracciatella android.permission.WRITE_EXTERNAL_STORAGE
```

App `targetSdk 26` — path `/sdcard/JA2` doc duoc voi quyen tren.

---

## 7. SMOKE PLAY

```bash
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity \
  -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
# Tap FAB (Play) — toa do phu thuoc orientation launcher
# Portrait 1080x2400: ~[965, 2222]
# Landscape: dump uiautomator bounds id/fab

adb logcat -s ja2 SDL SDL/APP StracciatellaActivity LauncherActivity
```

**OK khi:**
- Log: `setOrientationBis locked landscape w=1366 h=768`
- Log: `Running main function SDL_main` + **khong** `Finished main` som + **khong** `entity not found`
- `topResumedActivity` = `StracciatellaActivity`
- Screencap **w > h** (landscape)

Screenshot session: `~/Downloads/ogvm-wf-landscape.png`

---

## 8. KIEN TRUC (tom tat)

```
android/app/   Kotlin: LauncherActivity | Data/Settings/Logs | StracciatellaActivity:SDLActivity
     │
     ▼ externalNativeBuild CMake → ../../CMakeLists.txt
libja2.so + SDL2
  BUILD_LAUNCHER=OFF  WITH_UNITTESTS=OFF  WITH_RUST_BINARIES=OFF
assets/  ← sourceSets = ../../assets  (externalized JSON + sti loadscreens neu co)
```

Config: app private `filesDir/.ja2/ja2.json`.  
Game data: external storage path user chon / adb push.

---

## 9. FILE LIEN QUAN

```
android/app/build.gradle                 compileSdk 33, abi arm64, CMAKE_MAKE_PROGRAM
android/gradle.properties                suppressUnsupportedCompileSdk
android/app/src/main/AndroidManifest.xml sensorLandscape game activity
android/app/src/main/java/.../StracciatellaActivity.kt  landscape lock override
android/app/src/main/java/.../LauncherActivity.kt       ja2.json load/save + FAB
android/app/src/main/java/.../Ja2Json.kt                res / resversion / game_dir
android/app/src/main/java/.../ControllerIni.kt          controller.ini r/w
android/app/src/main/java/.../ConfigurationModel.kt     PRESETS + controller
android/app/src/main/java/.../ui/main/SettingsFragment.kt  presets + pad UI
tools/build-android-debug.sh
assets/externalized/loadscreens/*.sti    LOCAL ONLY (gitignore) — copy tu build/
build/externalized/LOADSCREENS/*.sti     desktop build output (source copy)
OGVM_ANDROID.md                          handoff nay
src/game/MainMenuScreen.cc               wide/1024/vanilla bg selection
src/sgp/GameController.cc                engine pad (Android + desktop)
```

---

## 10. SETTINGS MOBILE = DESKTOP (06/08 session 2)

### Resolution presets (Settings tab)

Spinner **OGVM presets**:
- `1024x768` — WF native / desktop default
- `1360x768` — user request (gan 1366)
- `1366x768` — desktop tuned layout
- `1664x768` — **mobile-only** ultra-wide: +298px vs 1366 (~+149 moi ben). Engine `UILayout` center 640-UI (`STD_SCREEN_X=512`), map wood grow 2 ben, bottom pin-right. **Khong** them desktop RESLIST-LOCK.
- `640x480` — smoke / compare original
- `Custom…` — freeform WxH edit fields (Detect van dung)

Default khi **chua** co `ja2.json`: `1024x768` (khong auto-detect nua).

Math 1664 (UILayout, khong hardcode C++):
| | |
|---|---|
| STD_SCREEN_X/Y | 512 / 144 |
| Team panel X (12 slot WF) | 237 |
| Map view start X | ~605 (1px floor OK) |
| Map bottom base X | 901 (= W−763) |
| Menu STI | path wide (`>=1366`) |

### Controller (Settings tab)

| UI | Ghi |
|---|---|
| Enable gamepad | `enabled=0\|1` |
| Left stick | `none\|cursor\|wasd\|arrow` |
| Right stick | same |

File: `filesDir/.ja2/controller.ini` — **cung format** desktop engine `GameController.cc`.  
Default binds khi bat: A=LMB, B=RMB, LB/RB=wheel, D-pad=arrows, Start=Enter, Back=Esc, L stick=cursor.

**Khong** port FLTK remap bang (pad center / kind-value). Desktop only.

Code:
```
android/.../ControllerIni.kt          read/write ini
android/.../ConfigurationModel.kt     PRESETS + controller LiveData
android/.../ui/main/SettingsFragment.kt
android/.../LauncherActivity.kt       load/save controller + default res
android/.../res/layout/fragment_launcher_settings.xml
android/.../res/values/strings.xml
AndroidManifest.xml                   uses-feature gamepad optional
```

---

## 11. VIEC CON LAI

1. Phone that: push WF `Data/` + config 1366 + smoke (giong emu).
2. Optional: multi-ABI / release keystore.
3. Optional: SAF / `MANAGE_EXTERNAL_STORAGE` Android 11+ (chua).
4. Optional: ship wide menu bang asset **khong** `*.sti` gitignore (doi policy / generate) — hien copy tay tu desktop build.
5. Controller full remap UI: desktop only (mobile = enable + sticks).
6. Smoke multi-edition detector tren Android neu can.
7. Emulator GL spam `emuglGLESv2_enc GL error 0x501` — thuong khong fatal; bo qua neu game chay.
8. Smoke controller: bat Enable → Play → BT/USB pad (emu: virtual gamepad).

---

## 12. LENH NHANH (build + emulator)

```bash
export ANDROID_HOME="$HOME/Library/Android/sdk"
export ANDROID_SDK_ROOT="$ANDROID_HOME"
export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/25.0.8775105"
export PATH="$ANDROID_HOME/cmake/3.22.1/bin:$ANDROID_HOME/emulator:$ANDROID_HOME/platform-tools:$PATH"

cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"

# wide STI (can neu res >= 1024)
mkdir -p assets/externalized/loadscreens
cp -f build/externalized/LOADSCREENS/mainmenubackground_wide.sti assets/externalized/loadscreens/ 2>/dev/null || true
cp -f build/externalized/LOADSCREENS/mainmenubackground_1024.sti assets/externalized/loadscreens/ 2>/dev/null || true

# build APK
./tools/build-android-debug.sh

# start emulator (background)
emulator -avd Pronunciation_API_35 -netdelay none -netspeed full &
adb wait-for-device
# cho boot xong
until [ "$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; do sleep 2; done

# install
adb install -r android/app/build/outputs/apk/debug/app-debug.apk

# quyen storage
adb shell pm grant io.github.ja2stracciatella android.permission.READ_EXTERNAL_STORAGE
adb shell pm grant io.github.ja2stracciatella android.permission.WRITE_EXTERNAL_STORAGE

# config mau 1366 + NEAR_PERFECT (game_dir = folder CO Data/)
cat > /tmp/ja2.json <<'EOF'
{
  "game_dir": "/storage/emulated/0/JA2",
  "resversion": "ENGLISH",
  "res": "1366x768",
  "scaling": "NEAR_PERFECT",
  "debug": true
}
EOF
adb push /tmp/ja2.json /data/local/tmp/ja2.json
adb shell "run-as io.github.ja2stracciatella mkdir -p files/.ja2"
adb shell "cat /data/local/tmp/ja2.json | run-as io.github.ja2stracciatella sh -c 'cat > files/.ja2/ja2.json'"

# bat controller defaults (optional)
cat > /tmp/controller.ini <<'EOF'
enabled=1
layout=xbox
left_stick=cursor
right_stick=none
touchpad=cursor
touchpad_sens=1100
touchpad_out=none
a=mouse:left
b=mouse:right
x=none
y=none
leftshoulder=wheel:up
rightshoulder=wheel:down
lefttrigger=none
righttrigger=none
dpup=key:up
dpdown=key:down
dpleft=key:left
dpright=key:right
start=key:return
back=key:escape
EOF
adb push /tmp/controller.ini /data/local/tmp/controller.ini
adb shell "cat /data/local/tmp/controller.ini | run-as io.github.ja2stracciatella sh -c 'cat > files/.ja2/controller.ini'"

# launch launcher (tap FAB Play)
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity \
  -a android.intent.action.MAIN -c android.intent.category.LAUNCHER

# log
adb logcat -s ja2 SDL SDL/APP StracciatellaActivity LauncherActivity ControllerIni

# kill emu
adb -s emulator-5554 emu kill

# uninstall
adb uninstall io.github.ja2stracciatella
```

---

## 13. SESSION TOM TAT

### 06/08 session 1 (build/emu/landscape)
1. Emulator `Pronunciation_API_35` boot OK; install OGVM APK.
2. Push vanilla Data → config 640 → Play → map screen smoke OK (user screenshot).
3. Push Wildfire Data; config `1366x768` + `NEAR_PERFECT`.
4. Crash thieu `mainmenubackground_wide.sti` → copy vao assets, rebuild APK.
5. SDL flip portrait → lock landscape (manifest + `setOrientationBis` override).
6. Smoke: landscape 2400×1080, engine 1366×768, process song, screenshot OK.

### 06/08 session 2 (settings = desktop)
1. Res presets: 1024×768, 1360×768, 1366×768, 640×480 + Custom.
2. Default res 1024×768 khi thieu ja2.json.
3. Controller enable + L/R stick → `controller.ini` (engine native).
4. Manifest `uses-feature` gamepad optional.
5. Handoff + lenh emu cap nhat.
6. Launcher `fullUser` rotate: bo `configChanges=orientation` (recreate layout), `requestedOrientation=FULL_USER` onCreate/onResume. Game `sensorLandscape` giu.
7. Preset **1664×768** mobile-only (UILayout expand sides; khong doi desktop RESLIST-LOCK / UILayout C++).

---

## 14. LUU Y MOI TRUONG

- Duong dan co space → quote.
- Khong commit `local.properties`, APK, keystore, `*.sti`, game `Data/`.
- AGP 7.4: chi base platform ≤33.
- `abiFilters` arm64-only: du cho A16 + Apple Silicon emu; CI full ABI neu release store.
