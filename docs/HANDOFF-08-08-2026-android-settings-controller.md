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
- `android/app/src/main/res/layout/fragment_launcher_settings.xml`
- `android/app/src/main/res/values/strings.xml`
- `android/app/src/test/java/io/github/ja2stracciatella/ControllerIniTest.kt`
- `docs/HANDOFF-08-08-2026-android-settings-controller.md`

---

## 5. BUILD / INSTALL

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

## 6. SMOKE CHECKLIST

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

---

## 7. COMMIT

```text
OGVM-ANDROID: full controller settings + hotplug detect
```

---

## 8. NEXT BACKLOG

- Live button-listening remap mode.
- Native multi-controller support.
- Reset-to-default button in Android UI.
- Resolve local Android SDK package path warnings.
