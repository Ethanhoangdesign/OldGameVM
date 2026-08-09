# HANDOFF — 08/08/2026

**Android Settings full controller mapping + hotplug detect**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | Android launcher Settings |
| Emulator | `Pronunciation_API_35` |
| Package | `io.github.ja2stracciatella` |
| Launcher | `io.github.ja2stracciatella/.LauncherActivity` |

---

## 0. STATUS

| Work | Status | Note |
|---|---|---|
| Full Android controller mapping UI | DONE | Dropdown mapping, no file editing needed |
| Complete `controller.ini` persistence | DONE | Schema matches native `GameController.cc` |
| Xbox / PS5 layout selector | DONE | PS5 touchpad controls conditional |
| Left/right stick modes | DONE | None, Cursor, WASD, Arrows |
| Hotplug controller detection | DONE | Polls while Settings view active |
| SDL Android motion source matching | DONE | Accepts joystick-class source bitmask, not exact source value |
| Native controller diagnostics | DONE | Logs SDL joystick enumeration, mapping status, and open failures |
| Android unit tests | DONE | `ControllerIniTest` |
| Android debug build | DONE | APK generated |
| Live button-listening remap | OPEN | Not included; dropdown mapping only |
| Native multi-controller support | OPEN | Engine still opens first controller |

---

## 1. ANDROID SETTINGS

`SettingsFragment.kt` now exposes:

- Enable gamepad.
- Xbox / PS5 layout.
- Left stick: None, Cursor, WASD, Arrows.
- Right stick: None, Cursor, WASD, Arrows.
- PS5 touchpad: Cursor, Button, Disabled.
- PS5 touchpad sensitivity: 200–4000.
- Touchpad output mapping.
- 14 physical button rows:
  - A, B, X, Y.
  - Left/right shoulder.
  - Left/right trigger.
  - D-pad up/down/left/right.
  - Start, Back.
- Output types:
  - None.
  - Mouse left/right/middle.
  - Wheel up/down.
  - Cursor nudge up/down/left/right.
  - Keyboard keys, arrows, Enter, Escape, Space, Tab, Backspace, Delete, F1–F12, A–Z, 0–9.

Descriptions shortened to 1–2 lines. Resolution preset menu now appears before manual resolution fields and Detect. Debug Mode moved to bottom of Settings.

---

## 2. CONTROLLER HOTPLUG DETECT

`SettingsFragment.kt` polls Android `InputDevice` every 500 ms while view exists.

Detection matches SDL source logic:

- `SOURCE_CLASS_JOYSTICK`.
- `SOURCE_DPAD`.
- `SOURCE_GAMEPAD`.

UI states:

- No device: `No controller detected`.
- Insert: `Controller detected: <name>` plus one Toast.
- Removal: `Controller disconnected`.

Polling stops in `onDestroyView`. No native SDL state changed. Game still initializes controller through existing native path.

Reference implementation: `android/app/src/main/java/org/libsdl/app/SDLControllerManager.java`, `isDeviceSDLJoystick()`.

---

## 3. CONFIG FORMAT

`ControllerIni.kt` reads/writes:

```ini
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
```

Path Android:

```text
<filesDir>/.ja2/controller.ini
```

Invalid output specs become `none`. Missing bindings use native defaults. Sensitivity clamps to `200..4000`.

---

## 4. FILES CHANGED

- `android/app/src/main/java/io/github/ja2stracciatella/ConfigurationModel.kt`
- `android/app/src/main/java/io/github/ja2stracciatella/ControllerIni.kt`
- `android/app/src/main/java/io/github/ja2stracciatella/LauncherActivity.kt`
- `android/app/src/main/java/io/github/ja2stracciatella/ui/main/SettingsFragment.kt`
- `android/app/src/main/java/org/libsdl/app/SDLControllerManager.java`
- `src/sgp/GameController.cc`
- `android/app/src/main/res/layout/fragment_launcher_settings.xml`
- `android/app/src/main/res/values/strings.xml`
- `android/app/src/test/java/io/github/ja2stracciatella/ControllerIniTest.kt`
- `docs/HANDOFF-08-08-2026-android-settings-controller.md`

---

## 5. USB / EMULATOR VERIFICATION

### Simulator result

`Pronunciation_API_35` booted and APK installed successfully. Host macOS listed the DualSense, but its state was `Not Connected`. The AVD exposed no matching Android `InputDevice`:

```text
adb shell dumpsys input | grep -i -E 'dual|sense|sony|gamepad|joystick'
# no output
```

Therefore ordinary Android AVD did not receive the Mac-attached PS5 controller. `CONTROLLER nativeSetupJNI()` in logcat confirms SDL JNI setup only; it does not prove controller enumeration.

### Samsung USB test

Samsung USB/OTG test remains pending. Android may expose the controller under `Wireless Controller`, `Sony Interactive Entertainment`, `USB Gamepad`, or another HID name. Check Android visibility first:

```bash
adb devices
adb shell dumpsys input | grep -i -E -A 20 -B 5 'dual|sense|sony|wireless|gamepad|joystick|054c|0ce6|usb|hid'
adb shell getevent -lp
```

Expected HID capabilities include `ABS_X`, `ABS_Y`, `ABS_RX`, `ABS_RY`, or `BTN_GAMEPAD`.

Install and launch APK:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Open Settings. Expected status: `Controller detected: <name>`. Enable gamepad, start game, then capture native logs with:

```bash
adb logcat -c
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
sleep 3
adb logcat -d -v time | grep -i -E 'OGVM-CONTROLLER|SDL|controller|joystick|gamepad|InputDevice'
```

Expected native path:

```text
OGVM-CONTROLLER: enumerating 1 joystick(s)
OGVM-CONTROLLER: joystick 0 name='...' game_controller=yes
OGVM-CONTROLLER: opened '...' touchpads=...
```

Interpretation:

- No device in `dumpsys input`: USB/OTG/Android visibility issue.
- Android device visible but Settings says none: Android source detection issue.
- Settings detects device but native enumerates `0 joystick(s)`: SDL Android registration issue.
- Native opens controller but input fails: mapping/event routing issue.

Do not mark simulator or Samsung game-input smoke pass without Android device visibility, native open log, and button/stick evidence.

---

## 6. BUILD / INSTALL

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:testDebugUnitTest app:assembleDebug --no-daemon
```

Last verified:

```text
BUILD SUCCESSFUL in 7s
42 actionable tasks: 16 executed, 26 up-to-date
```

Install and launch:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

APK:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

SDK XML/package warnings remain environmental; build passed.

---

## 7. SMOKE CHECKLIST

1. [x] Android unit tests pass.
2. [x] Android debug APK builds.
3. [x] Resolution preset appears before manual fields + Detect.
4. [x] Resolution and Scaling descriptions shortened.
5. [x] Debug Mode appears at Settings bottom.
6. [ ] Open Settings without controller → `No controller detected`.
7. [ ] Connect controller → detected name + one Toast.
8. [ ] Disconnect controller → `Controller disconnected`.
9. [ ] Reconnect controller → detected again.
10. [ ] Start game and verify existing SDL mappings.
11. [ ] Samsung USB/OTG: `dumpsys input` exposes PS5 HID device.
12. [ ] Samsung Settings: `Controller detected: <name>`.
13. [ ] Samsung native log: SDL enumerates and opens controller.
14. [ ] Samsung game: buttons, d-pad, triggers, sticks, and touchpad verified.
15. [x] Simulator PS5 input: not received; AVD exposed no controller device.

---

## 8. COMMIT

```text
OGVM-ANDROID: full controller settings + hotplug detect
```

---

## 9. NEXT BACKLOG

- Live button-listening remap mode.
- Native multi-controller support.
- Reset-to-default button in Android UI.
- Resolve local Android SDK package path warnings.

---

## 10. CURRENT SESSION HANDOFF — 09/08/2026

### User-reported Samsung A16 result

- PS5 DualSense pairs and controls Android launcher/game.
- Android exposes real device, not virtual:
  - Vendor `0x054c`.
  - Product `0x0ce6`.
  - Name `DualSense Wireless Controller`.
  - Sources include `GAMEPAD`, `JOYSTICK`, `MOUSE`, and `TOUCHPAD`.
- Cursor image became visible after prior rendering changes.
- Latest user-reported defects: cursor trails, button mapping selections not visibly applying, mapping rows too cramped.

### Changes made this session

#### Android cursor compositing

`src/sgp/Video.cc` Android `RefreshScreen()` now:

1. Keeps `FrameBuffer` as clean game image.
2. Copies `FrameBuffer` into `ScreenBuffer` every frame.
3. Draws software cursor onto `ScreenBuffer`.
4. Uploads `ScreenBuffer` using full-frame `SDL_UpdateTexture`.

This prevents old cursor pixels from remaining in `FrameBuffer` and creating trails. Keep full-frame texture upload; prior Android GLES partial-update path caused crashes.

#### Controller pointer state

`src/sgp/GameController.cc` analog stick cursor movement now calls `SetUsingTouch(false)` before updating mouse position. Touchpad and nudge paths already clear touch state. This lets controller cursor render after touchscreen input and prevents cursor definitions hidden during touch mode.

#### Controller tab and mapping UI

- Controller/Gamepad remains separate tab immediately after Settings.
- Dynamic mapping rows now initialize value spinner with `None`.
- Kind selection rebuilds value options and selects first valid output.
- Value selection updates `ControllerIni.Config` and persists through existing save path.
- UI synchronization uses non-animated `setSelection(..., false)` while callbacks are suppressed, preventing observer feedback from overwriting user selection.
- Mapping rows use 6dp vertical padding and 6dp bottom margin, approximately 50% more vertical spacing.
- Android mapping now mirrors desktop two-column semantics: kind labels match desktop and value dropdown includes desktop keyboard keys and mouse/wheel/motion labels.
- Removed unused `IsSoftwareCursorVisible()` helper/declaration.

### Verification state

- `git diff --check`: passed.
- Android build: not verified in this session. Bash execution was blocked repeatedly by temporary model safety-classifier availability:

```text
claude-cx/gpt-5.6-luna-review is temporarily unavailable, so auto mode cannot determine the safety of Bash right now.
```

- Samsung APK smoke test: pending after successful build.
- Native build: pending.

### Files touched this session

- `android/app/src/main/java/io/github/ja2stracciatella/ui/main/ControllerFragment.kt`
- `src/sgp/Video.cc`
- `src/sgp/GameController.cc`
- `src/sgp/Cursor_Control.cc`
- `src/sgp/Cursor_Control.h`

Earlier controller-tab files remain changed as listed above.

### Manual build

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:testDebugUnitTest app:assembleDebug --no-daemon
```

Expected APK:

```text
/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android/app/build/outputs/apk/debug/app-debug.apk
```

### Install and launch

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

### Next action

1. Run `app:testDebugUnitTest app:assembleDebug`.
2. Fix compile/test failures.
3. Install APK on Samsung A16.
4. Verify cursor does not leave trails after repeated stick/touchpad movement.
5. Verify mapping kind/value visibly update and survive leaving/reopening Controller tab.
6. Verify mapping rows have usable spacing.
7. Capture `dumpsys input`, `OGVM-CONTROLLER` enumeration/open logs, and button/stick evidence before marking Samsung verification complete.
8. Do not commit until build and smoke checks pass.

---

## 11. CURRENT SESSION HANDOFF — 09/08/2026 MAPPING FIX

### User-reported defect

Samsung A16 showed only A / Cross retaining selected mapping kind. Other rows returned to `<empty>` after choosing a kind. Desktop uses two controls per row: kind on left, value on right.

### Root cause

Android `Spinner` fires `onItemSelected` while value adapter is being replaced. Value callback read `kind.selectedItemPosition` during that transient update; Android reported position `0`, so `withBinding()` wrote `none`. `LiveData` observer then synchronized row back to `<empty>`.

### Fix

`ControllerFragment.kt` now:

- Stores selected kind index in `kind.tag`.
- Value callback reads stored kind, not transient Spinner position.
- Suppresses callbacks while replacing value adapter and setting selection.
- Suppresses nested UI synchronization during user callback.
- Keeps desktop two-column flow: select `Kind`, then select `Value`.
- Adds `Button / Kind / Value` headers.
- Uses desktop-compatible labels and output list.
- Adds larger dropdown rows through `launcher_spinner_dropdown_item.xml`.

`ControllerIni.kt` output list now matches desktop choices: mouse buttons, wheel, motion, arrows, common keyboard keys, modifiers, `F1`–`F12`, `A`–`Z`, and `0`–`9`.

### Verification

- `git diff --check`: passed.
- Android unit test/build: run after this handoff update; record result here.
- Samsung A16 mapping persistence: user reports fixed in latest APK; repeat after build if APK install available.

### Files changed in mapping fix

- `android/app/src/main/java/io/github/ja2stracciatella/ui/main/ControllerFragment.kt`
- `android/app/src/main/java/io/github/ja2stracciatella/ControllerIni.kt`
- `android/app/src/main/res/layout/fragment_launcher_controller.xml`
- `android/app/src/main/res/layout/launcher_spinner_dropdown_item.xml`
- `docs/HANDOFF-08-08-2026-android-settings-controller.md`

### Next action

1. Run Android tests and debug APK build.
2. Install APK on Samsung A16.
3. Verify B/Circle, X/Square, shoulder, d-pad, Start, and Back mappings persist after tab switch and app restart.
4. Capture native controller logs and button/stick evidence.
5. Keep live button-listening remap and native multi-controller support in backlog.
6. Keep commit/push status in Git history after build result.
