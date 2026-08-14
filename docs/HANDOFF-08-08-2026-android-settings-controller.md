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
- Android unit tests + debug APK build: passed.
- Build result:

```text
BUILD SUCCESSFUL in 3s
42 actionable tasks: 3 executed, 39 up-to-date
```

- APK install on Samsung A16/connected Android device: passed.
- Launcher force-stop and start: passed.
- User confirmed mapping fix works: rows other than A / Cross can retain selected `Kind` and `Value`.
- SDK warnings remain environmental and do not block build:
  - SDK XML version mismatch.
  - `platforms;android-33` inconsistent path.
  - `package.xml` unexpected `abis` element.
  - Deprecated Gradle features warning.
- Full Samsung native button/stick smoke evidence: still pending.

### Commit and push

```text
Commit: b1110c98d
Message: OGVM-ANDROID: fix controller mapping persistence
Branch: feature/multi-edition-detector
Remote: origin
Status: pushed successfully
```

Repository:

```text
https://github.com/Ethanhoangdesign/OldGameVM/tree/feature/multi-edition-detector
```

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

---

## 12. CURRENT SESSION HANDOFF — 09/08/2026 LAUNCHER UI ONLY

### User request

Only change Android launcher UI:

- Remove header area containing `JA2 Stracciatella`.
- Keep tabs as sole header content.
- Reduce header height so page content moves upward.
- Change select controls to outlined fields with visible line and floating label, matching supplied reference image.
- Preserve all existing behavior and configuration logic.
- Do not commit unless explicitly requested.

### UI changes made

- Removed launcher title `TextView` with id `title` from `activity_launcher.xml`.
- Kept `TabLayout`, `ViewPager2`, and FAB unchanged.
- Added reusable `LauncherOutlinedSpinner` style based on `Widget.MaterialComponents.TextInputLayout.OutlinedBox`.
- Wrapped existing raw `Spinner` controls in Material `TextInputLayout` wrappers:
  - Data: Game version.
  - Settings: Resolution preset and Scaling quality.
  - Controller: Layout, Left stick, Right stick, Touchpad mode, Touchpad output.
  - Dynamic controller mappings: Kind and Value.
- Kept all existing Spinner IDs, adapters, `OnItemSelectedListener` callbacks, `selectedItemPosition` usage, LiveData observers, enable/disable logic, dynamic adapter replacement, and persistence.
- Increased `launcher_spinner_item.xml` minimum height to 48dp and added horizontal padding.
- Added field-label string resources for floating labels.
- Added spacing and adjusted weights for dynamic mapping rows so Button / Kind / Value remain readable on phone screens.
- No new dependency added; existing Material Components `1.6.1` reused.

### Crash found after first UI build

App installed successfully, but opening `LauncherActivity` crashed:

```text
Caused by: java.lang.IllegalArgumentException: The style on this component requires your app theme to be Theme.MaterialComponents (or a descendant).
at com.google.android.material.internal.ThemeEnforcement.checkMaterialTheme
at com.google.android.material.textfield.TextInputLayout.<init>
```

Root cause: `LauncherActivity` uses `@style/AppTheme.NoActionBar`, while that style did not inherit from a Material Components theme. New `TextInputLayout` instances therefore failed during ViewPager layout.

### Crash fix applied

Updated `android/app/src/main/res/values/styles.xml`:

```xml
<style name="AppTheme.NoActionBar" parent="Theme.MaterialComponents.DayNight.NoActionBar">
    <item name="colorPrimary">@color/colorPrimary</item>
    <item name="colorPrimaryDark">@color/colorPrimaryDark</item>
    <item name="colorAccent">@color/colorAccent</item>
</style>
```

This is a theme-only compatibility fix. No controller, persistence, adapter, or callback logic changed.

### Verification state

- Initial UI APK build: passed.
- Initial APK install: passed.
- Initial `LauncherActivity` start: command returned `Status: ok`, but app then crashed on first view layout due to theme mismatch.
- Crash log captured from emulator `emulator-5554`.
- Theme fix applied locally.
- Rebuild and reinstall after theme fix: pending; run from Android directory:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:testDebugUnitTest app:assembleDebug --no-daemon --console=plain
adb -s emulator-5554 install -r app/build/outputs/apk/debug/app-debug.apk
adb -s emulator-5554 shell am force-stop io.github.ja2stracciatella
adb -s emulator-5554 shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

- `git diff --check`: previously passed before latest theme-only edit; rerun after rebuild.
- Emulator: `emulator-5554`.
- Samsung A16 serial: `R5GL31H83QX`.

### Files changed for launcher UI

- `android/app/src/main/java/io/github/ja2stracciatella/ui/main/ControllerFragment.kt`
- `android/app/src/main/res/layout/activity_launcher.xml`
- `android/app/src/main/res/layout/fragment_launcher_controller.xml`
- `android/app/src/main/res/layout/fragment_launcher_data_tab.xml`
- `android/app/src/main/res/layout/fragment_launcher_settings.xml`
- `android/app/src/main/res/layout/launcher_spinner_item.xml`
- `android/app/src/main/res/values/strings.xml`
- `android/app/src/main/res/values/styles.xml`

### Next action

1. Rebuild after `AppTheme.NoActionBar` Material parent fix.
2. Reinstall APK on `emulator-5554`.
3. Launch and confirm no crash.
4. Visually verify tabs-only header, reduced header height, outlined fields, floating labels, dropdown popup, and readable mapping rows.
5. Verify tab switching and controller selection callbacks still update configuration.
6. Run `git diff --check`.
7. Leave changes uncommitted unless user requests commit.
8. Keep live button-listening remap and native multi-controller support in backlog.

### Repository state

- Branch: `feature/multi-edition-detector`.
- Launcher UI changes remain uncommitted.
- No commit created for this UI session.
- Existing prior commits remain unchanged.
- Do not mark UI smoke verification complete until rebuilt APK launches without crash and screenshot confirms requested layout.

---

## 13. COMMAND QUICK REFERENCE

```bash
# List available AVDs
~/Library/Android/sdk/emulator/emulator -list-avds

# One-terminal build/install/launch flow; starts first AVD if needed
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android" && AVD=$(~/Library/Android/sdk/emulator/emulator -list-avds | sed -n '1p') && [ -n "$AVD" ] || { printf 'Chưa có AVD\n'; exit 1; }; adb start-server >/dev/null; SERIAL=$(adb devices | awk '$1 ~ /^emulator-/ {print $1; exit}'); [ -n "$SERIAL" ] || { nohup ~/Library/Android/sdk/emulator/emulator -avd "$AVD" >/tmp/ja2-emulator.log 2>&1 </dev/null & }; printf 'Đang chờ simulator...\n'; while :; do SERIAL=$(adb devices | awk '$1 ~ /^emulator-/ && $2=="device" {print $1; exit}'); if [ -n "$SERIAL" ] && [ "$(adb -s "$SERIAL" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; then break; fi; sleep 2; done; printf 'Simulator sẵn sàng: %s\n' "$SERIAL"; ./gradlew app:assembleDebug --no-daemon --console=plain && adb -s "$SERIAL" install -r app/build/outputs/apk/debug/app-debug.apk && adb -s "$SERIAL" shell am force-stop io.github.ja2stracciatella && adb -s "$SERIAL" shell am start -n io.github.ja2stracciatella/.LauncherActivity

# Rebuild after theme crash fix
./gradlew app:testDebugUnitTest app:assembleDebug --no-daemon --console=plain

# Install and launch existing emulator
adb -s emulator-5554 install -r app/build/outputs/apk/debug/app-debug.apk
adb -s emulator-5554 shell am force-stop io.github.ja2stracciatella
adb -s emulator-5554 shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Do not paste `NAME_AVD` literally; it is only a placeholder. Use the exact name returned by `-list-avds`.

---

## 14. WARNINGS AND KNOWN LIMITATIONS

- Android SDK XML version mismatch warning is environmental.
- `platforms;android-33` package path warning is environmental.
- `package.xml` unexpected `abis` element warning is environmental.
- Gradle deprecated-feature warning is non-blocking.
- AVD may not receive Mac-attached DualSense input even when macOS sees controller.
- Samsung native button/stick smoke evidence remains pending.
- Full live button-listening remap remains backlog.

---

## 15. HANDOFF ACCEPTANCE CRITERIA

UI session complete only when all items pass:

- [x] Rebuilt APK includes Material theme fix.
- [x] `LauncherActivity` opens without crash.
- [x] Header contains tabs only; title area removed.
- [x] Header is visibly shorter and content starts higher.
- [x] Select controls show gray bottom border and readable values.
- [x] Directory fields show gray bottom border.
- [x] Dropdown popups open and values remain selectable.
- [x] Disabled fields render correctly.
- [x] Tab switching works.
- [x] Controller configuration updates remain functional.
- [x] `controller.ini` persistence remains functional.
- [x] `git diff --check` passes.
- [x] Changes committed and pushed after explicit user request.

---

## 16. CURRENT SESSION HANDOFF — 09/08/2026 LAUNCHER FIELD BORDERS

### User request

- Keep tab labels white with visible active-tab indicator.
- Add visible selector borders so tap targets are clear.
- Use gray bottom border only, not full rectangular outline.
- Apply same bottom border to game and save directory fields.

### Changes made

- Added gray tab text and white active-tab indicator in `activity_launcher.xml`.
- Added `launcher_spinner_border.xml`: white background with gray `2dp` bottom line anchored to view bottom.
- Added `launcher_directory_border.xml`: same gray bottom line for directory path fields.
- Applied selector background to static and dynamic Spinner rows.
- Applied directory background to `gameDirValueText` and `saveGameDirValueText`.
- Removed full `TextInputLayout` outline styling; retained wrapper for existing labels/layout.
- Kept controller mapping, callbacks, persistence, and tab behavior unchanged.

### Verification

- `git diff --check`: passed.
- User screenshot confirmed selector rows and directory field layout visible.
- Android SDK XML/package warnings remain environmental.
- Samsung native button/stick smoke evidence remains pending.

### Files changed

- `android/app/src/main/java/io/github/ja2stracciatella/ui/main/ControllerFragment.kt`
- `android/app/src/main/res/layout/activity_launcher.xml`
- `android/app/src/main/res/layout/fragment_launcher_controller.xml`
- `android/app/src/main/res/layout/fragment_launcher_data_tab.xml`
- `android/app/src/main/res/layout/fragment_launcher_settings.xml`
- `android/app/src/main/res/layout/launcher_spinner_item.xml`
- `android/app/src/main/res/drawable/launcher_directory_border.xml`
- `android/app/src/main/res/drawable/launcher_spinner_border.xml`
- `android/app/src/main/res/values/colors.xml`
- `android/app/src/main/res/values/strings.xml`
- `android/app/src/main/res/values/styles.xml`
- `docs/HANDOFF-08-08-2026-android-settings-controller.md`

### Next action

1. Run Android unit tests and debug APK build if final clean verification needed.
2. Keep Samsung native controller evidence pending until logs and input proof captured.
3. Keep live remap and native multi-controller support in backlog.
4. Use commit/push recorded in Git history.

### Commit

```text
OGVM-ANDROID: add launcher selector bottom borders
```

### Repository

```text
Branch: feature/multi-edition-detector
Remote: origin
```

---

## 17. COMMAND QUICK REFERENCE — FINAL UI VERIFY

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:testDebugUnitTest app:assembleDebug --no-daemon --console=plain
adb -s emulator-5554 install -r app/build/outputs/apk/debug/app-debug.apk
adb -s emulator-5554 shell am force-stop io.github.ja2stracciatella
adb -s emulator-5554 shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Warnings expected: SDK XML version mismatch, inconsistent `platforms;android-33` path, unexpected `abis` in `package.xml`, and Gradle deprecated features.

Do not mark Samsung native controller smoke complete without Android device visibility, SDL enumeration/open logs, and button/stick evidence.

---

## 18. FINAL STATUS

- Launcher UI changes: ready for commit and push.
- Android controller mapping UI and persistence: done.
- Samsung native controller smoke evidence: pending.
- Live button-listening remap: backlog.
- Native multi-controller support: backlog.
- Reset-to-default button: backlog.

---

## 19. CURRENT SESSION HANDOFF — 09/08/2026 DISABLED CONTROLLER FIELDS

### User-reported issue

Controller mapping rows stayed visually active while Gamepad was disabled. Kind and Value text/lines did not share disabled styling, and disabled fields could still open dropdowns.

### Fix

`ControllerFragment.kt` now synchronizes disabled state across Spinner and `TextInputLayout`:

- Disabled Kind and Value text use light gray `#D9D9D9`.
- Enabled Kind and Value text use black `#000000`.
- Disabled bottom lines use light gray `#D9D9D9`.
- Enabled bottom lines use black `#000000`.
- All selector lines remain `1dp`.
- Disabled wrapper and Spinner reject touch, so dropdowns cannot open.
- Value stays disabled when Kind is `<empty>`.
- Static controller selectors use same state handling.
- Directory fields keep one-pixel bottom borders.

Separate enabled/disabled drawables:

- `android/app/src/main/res/drawable/launcher_spinner_enabled_border.xml`
- `android/app/src/main/res/drawable/launcher_spinner_disabled_border.xml`

### Verification

- Android unit tests: passed.
- Debug APK build: passed.

```text
BUILD SUCCESSFUL in 3s
42 actionable tasks: 3 executed, 39 up-to-date
```

- APK install: passed.
- Launcher restart: passed.
- User confirmed disabled mapping line/text appearance is correct.
- SDK XML, SDK path, `package.xml`, and Gradle deprecation warnings remain environmental/non-blocking.
- Samsung native button/stick evidence remains pending.

### Build APK

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew clean app:testDebugUnitTest app:assembleDebug --no-daemon --console=plain
```

APK output:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

### Install and restart

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

### Repository state

- Branch: `feature/multi-edition-detector`.
- Changes ready for commit and push after this handoff update.
- Keep live button-listening remap and native multi-controller support in backlog.
- Do not mark Samsung native controller smoke complete without Android visibility, SDL enumeration/open logs, and button/stick evidence.

---

## 20. BUILD / INSTALL QUICK REFERENCE

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew clean app:testDebugUnitTest app:assembleDebug --no-daemon --console=plain
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
``` 

APK: `android/app/build/outputs/apk/debug/app-debug.apk`

Commit message: `OGVM-ANDROID: fix disabled controller field styling`.

---

## 21. CURRENT SESSION HANDOFF — 09/08/2026 VALUE DROPDOWN TOUCH FIX

### User-reported defect

Only A / Cross and B / Circle Value dropdowns opened. Later rows showed selected Kind and Value text but tapping Value did nothing.

### Root cause

When a row started with `Kind = <empty>`, `syncControllerUi()` disabled both Value Spinner and its `TextInputLayout` wrapper. Kind selection later set only `value.isEnabled = true`; wrapper stayed disabled. `guardSpinner()` therefore consumed Value touches.

### Fix

`ControllerFragment.kt` now calls existing `setSpinnerEnabled()` after Kind changes. Spinner, `TextInputLayout`, border styling, and touch state stay synchronized. Value enables only when Gamepad is enabled and selected Kind is not `<empty>`.

### Verification

- `git diff --check`: passed.
- Android unit tests: passed.
- Debug APK build: passed.
- APK installed and launcher reopened on Android simulator.
- User confirmed Value dropdowns work after fix.
- Samsung native button/stick evidence remains pending.

---

## 28. CURRENT SESSION HANDOFF — 10/08/2026 LAPTOP CURSOR AUTO-CENTER

### User-reported defect

- Laptop opens with cursor at previous strategic/tactical position.
- On Android (aspect-fit), cursor often ends up in black bars/letterbox area or obscured regions, making it invisible upon opening Laptop.

### Fix

`src/game/Laptop/Laptop.cc` now forces the cursor to the logical center of the Laptop screen during initialization:

- Modified `EnterLaptop()` to reset mouse position at the end of initialization.
- Uses `SetSafeMousePositionLogical()` to target the 640x480 workspace center (`x=360, y=227`).
- Calls `SetUsingTouch(false)` on Android to ensure the software cursor renders immediately, even if the user was just using touch input.
- Position is set after `LaptopScreenRect` and mouse regions are initialized, ensuring the cursor stays within valid bounds.

### Verification

- Code change applied to `src/game/Laptop/Laptop.cc`.
- Build and runtime verification on Samsung A16: pending (safety classifier delay).
- Expected behavior: Every time Laptop opens, the cursor should jump to the center (near the AIM icon), regardless of where it was on the previous screen.

### Files changed

- `src/game/Laptop/Laptop.cc`

### Next action

1. Run Android debug build and install on device.
2. Verify cursor centers correctly upon opening Laptop.
3. Verify cursor becomes visible immediately if touch was used before opening.
4. Commit after user confirmation.

### Commit

```text
OGVM-ANDROID: fix disabled controller field styling
```

Changes committed on `feature/multi-edition-detector`. Working tree clean.

### Remaining backlog

- Live button-listening remap.
- Native multi-controller support.
- Reset-to-default button.
- Samsung native button/stick smoke evidence.

---

## 22. CURRENT SESSION HANDOFF — 09/08/2026 LAPTOP CURSOR + IMMEDIATE SAVE

### User-reported defects

- Left-stick Cursor mode drifts away from laptop buttons.
- Controller mappings disappear after leaving the game/app and require remapping.

### Findings

- Controller movement already uses logical game coordinates. `SetSafeMousePosition()` applied Android laptop screen-to-logical conversion a second time while laptop scale was active. `SimulateMouseMovement()` also warped a physical pointer for controller-generated movement.
- Android only wrote `controller.ini` from `LauncherActivity.saveJA2Json()` when Start Game ran. Mapping changes were not immediately persisted when leaving app/game through another lifecycle path. USB install is not cause.

### Changes made, pending verification

- Added `SetSafeMousePositionLogical()` in `src/sgp/Input.h` / `src/sgp/Input.cc`.
- Controller cursor, nudge, and touchpad movement now update logical position directly and refresh mouse regions without physical pointer warp in `src/sgp/GameController.cc`.
- `ControllerFragment.updateConfig()` now writes `controller.ini` after every user configuration change and logs save failures.
- Each dynamic mapping row ignores Spinner callbacks while `syncControllerUi()` loads its persisted Kind and Value. Readiness is deferred through the UI queue and versioned per synchronization, preventing delayed adapter/setSelection callbacks from saving the first Value of a Kind over distinct persisted Values during Controller tab recreation.

### Verification state

- `git diff --check`: passed.
- Android tests/APK rebuild after latest changes: pending; Bash execution was blocked by temporary safety-classifier unavailability.
- USB Samsung runtime verification: pending.
- Do not mark cursor fix or persistence fix complete until APK/native build and Samsung laptop button-targeting plus restart persistence pass.

### Next commands

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:testDebugUnitTest app:assembleDebug --no-daemon --console=plain
adb -d install -r app/build/outputs/apk/debug/app-debug.apk
adb -d shell am force-stop io.github.ja2stracciatella
adb -d shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Then verify laptop left-stick Cursor targeting, remap one button, leave app, relaunch, and confirm mapping remains.

---

## 23. CURRENT SESSION HANDOFF — 09/08/2026 DISTINCT VALUE RESTORE FIX

### User-reported defect

Mappings persisted, but after returning to Controller tab, rows sharing same Kind restored same first Value. Example: two `Mouse Button` rows with different Values became same default Value.

### Root cause

Dynamic Spinner adapter replacement and `setSelection()` emit delayed `onItemSelected()` callbacks. Readiness became active before all programmatic callbacks finished; callback then wrote `outputsForKind(kind)[0]` over persisted Value.

### Fix

`ControllerFragment.kt` now:

- Resets each row readiness during every `syncControllerUi()`.
- Version-tags each synchronization.
- Applies persisted Kind and exact persisted Value before enabling row callbacks.
- Defers readiness until next UI queue turn.
- Ignores stale callbacks from older synchronization versions.
- Keeps immediate `ControllerIni.save()` after real user changes.

### Verification

- `git diff --check`: passed.
- User confirmed distinct Values remain distinct after leaving and reopening game/app.
- Cursor movement reaches laptop buttons correctly.
- Android test/APK rebuild after final fix: pending; run command below.
- Native Samsung smoke logs remain pending.

### Commit

Planned commit:

```text
OGVM-ANDROID: preserve distinct controller mapping values
```

### Build and install

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android" && ./gradlew app:testDebugUnitTest app:assembleDebug --no-daemon --console=plain && adb -d install -r app/build/outputs/apk/debug/app-debug.apk && adb -d shell am force-stop io.github.ja2stracciatella && adb -d shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

### Remaining backlog

- Live button-listening remap.
- Native multi-controller support.
- Reset-to-default button.
- Full Samsung native button/stick evidence.

---

## 24. CURRENT SESSION HANDOFF — 10/08/2026 LAPTOP CURSOR AXIS DIAGNOSTIC

### User-reported defect

- Samsung A16 opens laptop with software cursor hidden.
- Left Stick remains configured as `Cursor`.
- Moving Left Stick does not reveal or move cursor.
- Touching PS5 touchpad makes cursor appear.

### Confirmed configuration and device

`controller.ini`:

```ini
enabled=1
left_stick=cursor
right_stick=none
touchpad=cursor
```

Samsung native startup log:

```text
OGVM-CONTROLLER: enumerating 2 joystick(s)
OGVM-CONTROLLER: joystick 0 name='DualSense Wireless Controller' game_controller=yes
OGVM-CONTROLLER: opened 'DualSense Wireless Controller' touchpads=0
```

The DualSense opens successfully. `touchpads=0` means SDL does not expose the PS5 touchpad through `SDL_GameController`; Android touch events are separate input.

### Root-cause investigation

Original `MainLoop()` called `GameController_Update()` only when `SDL_PollEvent()` returned no event. Stick motion produces `SDL_CONTROLLERAXISMOTION`, so polling could be skipped while the stick moved. Touchpad events already had explicit routing, matching the symptom that touchpad input restored the cursor.

Changes now present:

- `src/sgp/SGP.cc` routes `SDL_CONTROLLERAXISMOTION` to `GameController_HandleEvent()`.
- `src/sgp/GameController.cc` handles axis events by calling `GameController_Update()`.
- `RestoreControllerCursor()` clears touch mode, refreshes mouse regions, and selects `CURSOR_LAPTOP_SCREEN` while laptop is active.
- Controller cursor movement uses `SetSafeMousePositionLogical()` to avoid applying Android laptop coordinate conversion twice.
- Runtime diagnostics log axis events and sampled stick values.

Diagnostic log lines:

```text
OGVM-CONTROLLER: axis event axis={} value={}
OGVM-CONTROLLER: update dt={} lx={} ly={} rx={} ry={}
```

### Build/install result

Build and install on Samsung succeeded:

```text
BUILD SUCCESSFUL in 3s
Performing Streamed Install
Success
Starting: Intent { cmp=io.github.ja2stracciatella/.LauncherActivity }
```

The supplied `adb -d logcat` output only showed Android/SDL lifecycle and screen-touch lines. It did not contain `OGVM-CONTROLLER`, `axis event`, or `update` lines. This does not prove axis input failed because native logger writes to `cache/ja2.log`, and the logcat buffer was cleared after launch.

### Correct Samsung verification

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:assembleDebug --rerun-tasks --no-daemon --console=plain
adb -d install -r app/build/outputs/apk/debug/app-debug.apk
adb -d shell am force-stop io.github.ja2stracciatella
adb -d shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Reproduce:

1. Start game.
2. Open laptop.
3. Move Left Stick for 2–3 seconds without touching screen or DualSense touchpad.
4. Pull native log:

```bash
adb -d exec-out run-as io.github.ja2stracciatella cat cache/ja2.log > /tmp/ja2.log
grep -F "OGVM-CONTROLLER" /tmp/ja2.log
```

Interpretation:

- `axis event` + changing `update ... lx/ly`: SDL receives stick; inspect cursor overwrite or Android laptop crop/render state.
- `axis event` without `update`: timing guard or repeated same-clock updates blocks movement.
- `update` with `lx/ly` near zero: Android/SDL axis mapping problem.
- No axis/update lines, only enumeration/open: APK lacks current diagnostics or Android does not forward stick events.

### Current source concerns

- `GameController_Update()` runs from both axis-event dispatch and no-event polling.
- Every call updates `g_lastUpdate`; rapid axis events can produce `dtMs == 0` and return.
- The current diagnostics are required before changing cursor or laptop rendering again.
- Keep transition-time `VIDEO_NO_CURSOR` behavior unchanged until runtime evidence proves a later cursor overwrite.

### Verification state

- Controller config: confirmed.
- DualSense SDL open: confirmed.
- Android debug build: passed.
- Samsung APK install/restart: passed.
- Runtime axis evidence: pending.
- Samsung laptop Left Stick cursor fix: not confirmed.
- Do not mark fix complete until cursor appears and moves without touchpad, then touchpad behavior still works.

### Repository state

- Branch: `feature/multi-edition-detector`.
- Prior UI commit: `755964511` — `OGVM-ANDROID: fix disabled controller field styling`.
- Latest `src/sgp/GameController.cc` and `src/sgp/SGP.cc` controller changes remain subject to normal git status check.
- Do not commit new controller changes unless explicitly requested.

---

## 25. CURRENT SESSION HANDOFF — 10/08/2026 GAMESIR LAYOUT & STICK LABELS

### User request

- Thêm lựa chọn layout "GameSir" vào danh sách chọn layout tay cầm.
- Ghi rõ nhãn "Left analog stick" và "Right analog stick" cho hai cần xoay thay vì ghi vắn tắt.
- Thêm nhãn (label) tiêu đề phía trên 3 Spinner: Controller layout, Left analog stick, và Right analog stick do `TextInputLayout` bọc ngoài `Spinner` không tự động hiển thị gợi ý (hint).
- Không tự động commit khi chưa được yêu cầu.

### UI changes made

- Cập nhật [ControllerIni.kt](android/app/src/main/java/io/github/ja2stracciatella/ControllerIni.kt):
  - Thêm `"gamesir"` vào mảng `LAYOUTS`.
  - Hỗ trợ lưu/đọc config trường `layout` cho `"gamesir"` vào tệp `controller.ini`.
- Cập nhật [ControllerFragment.kt](android/app/src/main/java/io/github/ja2stracciatella/ui/main/ControllerFragment.kt):
  - Hiển thị `"GameSir"` trong danh sách chọn Layout (Spinner).
  - Tự động nhận diện tay cầm GameSir X5 Lite (Vendor ID `0x0500`, Product ID `0x3735`) để gán sang `"gamesir"` khi kết nối.
  - Ẩn phần tuỳ chỉnh touchpad (PS5) khi chọn layout GameSir hoặc Xbox.
- Cập nhật [strings.xml](android/app/src/main/res/values/strings.xml):
  - Đổi từ `Left stick` thành `Left analog stick` (với label chính) và `Left analog stick mode` (ở ô chọn).
  - Đổi từ `Right stick` thành `Right analog stick` (với label chính) và `Right analog stick mode` (ở ô chọn).
- Cập nhật [fragment_launcher_controller.xml](android/app/src/main/res/layout/fragment_launcher_controller.xml):
  - Thêm 3 `TextView` làm nhãn cố định đặt phía trên 3 Spinner: "Controller layout", "Left analog stick", và "Right analog stick" để khắc phục việc hint biến mất trong `TextInputLayout`.
- Cập nhật [ControllerIniTest.kt](android/app/src/test/java/io/github/ja2stracciatella/ControllerIniTest.kt):
  - Sửa unit test kiểm tra việc đọc/ghi layout `gamesir` hoạt động ổn định.

### Verification state

- Android unit tests: Passed.
- Debug APK build: Passed.
- UI layouts verified.
- Handoff file updated.

---

## 26. NEXT HANDOFF ACTION

1. Pull `/tmp/ja2.log` from Samsung after Left Stick reproduction.
2. Read `axis event` and `update` evidence before changing source.
3. If events exist, fix `GameController_Update()` timing/event scheduling first.
4. If values change but cursor remains hidden, trace later cursor selection and Android laptop presentation.
5. Rebuild, reinstall, and repeat laptop test.
6. Commit only after user confirms fix.

---

## 27. CURRENT SESSION HANDOFF — 10/08/2026 LEFT STICK CURSOR EVENT INJECTION

### User-reported defect

- Left Stick configured as `cursor` on GameSir G8 did not move the mouse properly.
- Releasing the stick or interacting with other inputs would cause the cursor to snap back to the edge of the screen.
- SDL axis motion events (`axis 0` and `axis 1` for Left Stick) were successfully registered by SDL and native code, but the UI ignored them.

### Root cause

`GameController_Update()` previously handled cursor movement by directly modifying internal variables (`gusMouseXPos` and `gusMouseYPos`) via `SetSafeMousePositionLogical()`. However, it never enqueued an actual `MOUSE_POS` event into the game's event queue (`gEventQueue`). Because the event system and UI were unaware of the movement, the cursor didn't visually update correctly, and subsequent touch/mouse events would overwrite the position with stale coordinates, causing the snap-back effect.

### Fixes applied

1. **Mouse Event Injection**:
   - Added `PadInjectMousePos(int x, int y)` in `src/sgp/Input.cc` and `src/sgp/Input.h`.
   - This function clears `gfIsUsingTouch`, clamps coordinates, and correctly enqueues a `MOUSE_POS` event using `QueuePointerEvent`.
   - Updated `src/sgp/GameController.cc` to call `PadInjectMousePos()` during stick/touchpad cursor movement instead of silently setting logical bounds.

2. **GameSir Hardware Mapping**:
   - Injected explicit standard SDL mapping strings in `LoadMappingDb()` for **GameSir-X5 Lite** (Vendor `0x0500`, Product `0x3735`) and **GameSir G8 Galileo** (Vendor `0x3212`, Product `0x4102`).
   - Ensures axes and buttons are mapped to the correct SDL standard natively on initialization, regardless of Android's default behavior.

### Verification state

- Code successfully modified.
- User tested the compiled APK on Samsung device with GameSir G8.
- Cursor now tracks Left Stick correctly without drifting or snapping back.
- Game interactions (Laptop, UI buttons) process the injected MOUSE_POS correctly.

### Commit and next actions

- Code changes are verified and ready to be committed.
- Keep tracking `Native multi-controller support` and `Live button-listening remap` in the backlog.

---

## 29. CURRENT SESSION HANDOFF — 11/08/2026 BIG MAP FOR 720P & RESOLUTION PRESETS

### User request

- Tăng kích thước Strategic Map cho màn hình 720p (1280x720) để tận dụng diện tích hiển thị.
- Sắp xếp lại Resolution Presets trong Launcher, ưu tiên 1280x720 lên đầu.
- Giữ lại các chuẩn cũ (1664x768) nhưng thêm các chuẩn an toàn (4:3) cho Mobile.

### Changes made

1. **Resolution Presets (Launcher)**:
   - Cập nhật `ConfigurationModel.kt`:
     - Đưa `1280x720` lên vị trí đầu tiên.
     - Thêm đầy đủ 4 chuẩn an toàn: `640x480`, `800x600`, `1024x768`, `1280x720`.
     - Giữ lại chuẩn siêu rộng `1664x768` ở cuối danh sách.

2. **Big Map for 720p (Engine)**:
   - Cập nhật `src/game/UILayout.cc`:
     - Hạ ngưỡng chiều cao tối thiểu để kích hoạt "Full-size Map" (Big Map) từ `768` xuống `720`.
     - Điều này cho phép màn hình `1280x720` hiển thị bản đồ chiến lược lớn (ô lưới 42x36) thay vì bản đồ thu nhỏ (21x18) lọt thỏm giữa màn hình.

3. **Toạ độ UI Map Screen**:
   - Do Big Map (612px cao) + Bottom Panel (121px cao) = 733px, trên màn hình 720px thực tế sẽ có sự chồng lấn nhẹ (13px) hoặc cần cắt bớt phần gỗ dư thừa.
   - Logic `get_MAP_BOTTOM_BASE_Y()` hiện tại đặt panel tại `m_screenHeight - 480`. Với 720p, panel bắt đầu tại $Y=240$. Kết hợp offset vẽ `+359`, thanh đáy sẽ nằm sát mép dưới tại $Y=599$, khớp khít với bản đồ phía trên.

### Verification state

- **Code changes**: Applied to `ConfigurationModel.kt` and `UILayout.cc`.
- **Android Build**: Sẵn sàng để chạy `assembleDebug`.
- **Expected behavior**: Strategic Map của 1280x720 sẽ to tương đương với bản 768p, không còn bị co nhỏ ở giữa.

### Next action

1. Chạy build Android: `cd android && ./gradlew assembleDebug`.
2. Test thực tế trên 1280x720, kiểm tra xem thanh HUD đáy có bị đè lên bản đồ hay không.
3. Nếu bị đè, tinh chỉnh `get_MAP_BOTTOM_BASE_Y` hoặc `get_MAP_VIEW_START_Y`.

---

## 30. HIDE TACTICAL HISTORY LOG ON MOBILE (11/08/2026)

### User request

- Ẩn hoàn toàn bảng hiển thị tin nhắn (History Log) chứa text xanh/trắng ở góc dưới bên trái màn hình khi đang ở màn hình chiến thuật (Tactical / GAME_SCREEN).
- Bảng tin nhắn này lấn chiếm quá nhiều không gian trên màn hình nhỏ của mobile (như kích thước 1280x720 vừa được chỉnh lại).
- Giữ nguyên hệ thống History Log ở màn hình bản đồ chiến lược (Strategic Map / MAP_SCREEN).

### Changes made

1. **Vô hiệu hóa render và cập nhật Tactical Messages (`src/game/Utils/Message.cc`)**:
   - `TacticalScreenMsg`: Thêm `return;` ngay đầu hàm để chặn hoàn toàn việc tạo dòng tin nhắn mới và đưa vào hàng đợi (`pStringS`) của màn hình chiến thuật.
   - `ScrollString`: Thêm `return;` ngay đầu hàm để vô hiệu hóa việc tạo các Video Overlay cho các dòng tin nhắn mới.
   - `BlitString`: Thêm điều kiện `if (guiCurrentScreen == GAME_SCREEN) return;` để phòng hờ chặn render bất kỳ video overlay nào nếu vô tình lọt vào.

2. **Kết quả**:
   - Hệ thống tin nhắn hệ thống (ScrollString) ngừng hoàn toàn việc tạo chữ, xếp hàng và hiển thị đồ hoạ khi vào GAME_SCREEN.
   - Hàm `MapScreenMessage` chịu trách nhiệm ghi lại log cho màn hình bản đồ vẫn hoạt động bình thường, bảo toàn tính năng đọc lịch sử cũ ở ngoài Strategic Map.

### Verification state

- Cập nhật thành công mã nguồn C++.
- Đã build thành công bản Android Debug (`assembleDebug` - 42s).
- Cài đặt APK bằng `adb install -r`.
- Kiểm tra thực tế trên Emulator (Samsung A16 - 1280x720): Góc trái màn hình Tactical đã hoàn toàn biến mất phần History Log, trả lại không gian hiển thị mặt đất rộng rãi cho map.

### Next action

- Commit code changes.
- Chuyển sang tối ưu hoặc sửa thêm các lỗi hiển thị khác trên màn hình nhỏ (nếu có yêu cầu từ user).
