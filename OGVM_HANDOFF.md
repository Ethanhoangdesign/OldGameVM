# OGVM — Handoff: Wildfire 7.62mm WP Magazine Artwork (02/09/2026)

Repo: `Ethanhoangdesign/OldGameVM`
Branch: `feature/multi-edition-detector`
Local: `<repo-root>`

## Status — DONE

Wildfire 7.62mm WP magazine IDs `96–99` now use dedicated artwork, separate from 5.56mm IDs `92–95`:

- Big/detail artwork: `assets/externalized/sti/interface/inventory/custom-762wp-big.sti` (`38×45`).
- Small inventory artwork: `assets/externalized/sti/interface/inventory/custom-762wp-small.sti` (`34×23`).
- Both assets converted from the user-provided reference PNG.
- Small artwork keeps the curved magazine plus the bullet at right, scaled to the inventory footprint.
- Metadata unchanged: `AMMO762W`, capacities `30/75`, AP/HP types.
- 5.56mm IDs `92–95` remain on `bigitems/p1item29.sti` / `bigitems/p1item30.sti`.

## Files changed

- `OGVM_HANDOFF.md`
- `src/externalized/DefaultContentManager.cc`
- `src/externalized/DefaultContentManager_unittests.cc`
- `assets/externalized/sti/interface/inventory/custom-762wp-small.sti`
- `assets/externalized/sti/interface/inventory/custom-762wp-big.sti`

## Verification

- STI metadata validated: indexed 8-bit, 256-color palette, ETRLE, one subimage.
- Small frame: `34×23`; big frame: `38×45`.
- `git diff --check`: passed.
- Android build: `BUILD SUCCESSFUL`.
- APK install: `Success`.
- Launch: `io.github.ja2stracciatella/.LauncherActivity`.
- Device: connected USB Android device.
- User visually confirmed the big/detail artwork; small inventory artwork was then reduced to fit.

## Build/install

```bash
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

## Remaining scope

- No release APK generated.
- No commit existed before this handoff update; commit/push follows this handoff.
- No gameplay, calibre, capacity, or reload logic changed.

---

# OGVM — Handoff: Wildfire Autoresolve Victory Cleanup (30/08/2026)

Repo: `Ethanhoangdesign/OldGameVM`
Branch: `feature/multi-edition-detector`
Local: `<repo-root>`

## 0. TRANG THAI — DONE

Wildfire autoresolve victory no longer leaves a stale Strategic Map battle warning or Pre-Battle Interface:

- `RemoveAutoResolveInterface()` reapplies player ownership after enemy cleanup.
- Clears `gfBlitBattleSectorLocator`.
- Clears `SF_PLAYER_KNOWS_ENEMIES_ARE_HERE` for the resolved sector.
- Refreshes map and bottom-panel state.
- Resets encounter globals after creature-specific cleanup.
- Uses `NumHostilesInSector()` for excess-enemy cleanup.
- Defeat, retreat, surrender, creature-loss paths unchanged.

Previous Wildfire 5.56 magazine fix remains complete:

- IDs `92–93`: `bigitems/p1item29.sti`
- IDs `94–95`: `bigitems/p1item30.sti`
- Small inventory rendering: scaled, transparent, no oversized shadow.
- Item description/Bobby Ray: big artwork unchanged.

### Files changed in latest fix

- `src/game/Strategic/Auto_Resolve.cc`
- `OGVM_HANDOFF.md`

### Technical notes

Victory previously called `SetThisSectorAsPlayerControlled()` before autoresolve removed strategic enemies. Its hostile-count guard rejected the update. Cleanup then skipped `EliminateAllEnemies()` when enemy count had already reached zero, leaving the battle locator and awareness flag active. Final teardown now handles both cases.

### Validation

```bash
./tools/build-android-debug.sh
adb devices
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
git diff --check
```

Build/install/start completed successfully on the connected USB device. Manual target: autoresolve victory → `DONE` → Strategic Map; confirm no red warning/locator, no stale `AUTO RESOLVE`/`GO TO SECTOR` panel, time controls available. No strategic autoresolve unit-test target exists.

## USB ANDROID RULE — ALWAYS APPLY

When an Android phone is connected by USB and appears under `adb devices`, automatically build, install, and launch Android before visual verification. Do not wait for a separate request.

```bash
adb devices
./tools/build-android-debug.sh
APK="android/app/build/outputs/apk/debug/app-debug.apk"
DEVICE="$(adb devices | awk 'NR>1 && $2=="device" {print $1; exit}')"
adb -s "$DEVICE" install -r "$APK"
adb -s "$DEVICE" shell am force-stop io.github.ja2stracciatella
adb -s "$DEVICE" shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Use the physical USB device when available; start an emulator only when no USB device is connected.

---

# OGVM — Handoff: Wildfire 5.56 Magazine Inventory (29/08/2026)

Repo: `Ethanhoangdesign/OldGameVM`
Branch: `feature/multi-edition-detector`
Local: `<repo-root>`

## 0. TRANG THAI — DONE

Wildfire IDs `92–95` now use the exact curved magazine artwork:

- `92–93`: `bigitems/p1item29.sti`
- `94–95`: `bigitems/p1item30.sti`
- Inventory slots: scaled to small-slot bounds, transparent background, no oversized shadow.
- Item description/Bobby Ray: big artwork unchanged.
- Other editions/items: existing rendering unchanged.

### Files changed

- `src/externalized/DefaultContentManager.cc`
- `src/externalized/DefaultContentManager_unittests.cc`
- `src/game/Tactical/Interface_Items.cc`
- `src/sgp/VSurface.cc`
- `src/game/Tactical/Interface.cc` — existing enemy-turn button visibility change retained.

### Technical notes

`INVRenderItem()` detects only item IDs `92–95`, renders the source VObject into a transparent 16-bit temporary surface, then scales it through `BltStretchVideoSurface()`. `BltStretchVideoSurface()` now reads SDL color-key state with `SDL_HasColorKey()` / `SDL_GetColorKey()`; black transparent pixels are skipped. The affected items bypass the normal big-art shadow.

### Validation

```bash
./android/gradlew -p android assembleDebug
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
git diff --check
```

Build/install/start completed successfully. Visual check target: Wildfire Sector Inventory, IDs `92–95`; confirm no black square, correct small curved magazine, no large shadow, big item-description art intact.

## USB ANDROID RULE — ALWAYS APPLY

When an Android phone is connected by USB and appears under `adb devices`, automatically build, install, and launch Android before visual verification. Do not wait for a separate request.

```bash
adb devices
./tools/build-android-debug.sh
APK="android/app/build/outputs/apk/debug/app-debug.apk"
DEVICE="$(adb devices | awk 'NR>1 && $2=="device" {print $1; exit}')"
adb -s "$DEVICE" install -r "$APK"
adb -s "$DEVICE" shell am force-stop io.github.ja2stracciatella
adb -s "$DEVICE" shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Use the physical USB device when available; start an emulator only when no USB device is connected.

---

# OGVM — Handoff: Controller / Gamepad (05/08/2026, session 3)

Repo: `Ethanhoangdesign/OldGameVM`
Branch: `feature/multi-edition-detector`
Local: `<repo-root>`
Build: `cmake --build build -j8 --target ja2-launcher ja2` → `./build/ja2-launcher` / `./build/ja2`  
May: macOS arm64. Dich: Mac + Windows + Linux.

---

## 0. TRANG THAI

| Viec | Trang thai |
|---|---|
| Sector inventory (patch 3+5) | DONE, da commit (`66d711c56`) |
| Gamepad native engine | **DONE** — pad→key/mouse/wheel/nudge + stick modes |
| Tab Controller layout x360ce-ish | **DONE** — pad giua, kind/value 2 cot quanh |
| PNG Xbox/PS5 | **DONE** — `assets/externalized/controller_{ps5,xbox}.png` |
| PS5 touchpad → chuot (trackpad) | **DONE** — relative swipe + `touchpad_sens` |
| PS5 touchpad mode Cursor\|Button | **DONE** — dropdown + slider / kind+value |
| Quit game crash launcher | **DONE** — `logsDisplay` null buffer fix |
| Test tay cam that (Mac) | **PARTIAL** — user xac nhan touchpad OK; can smoke full bind |
| Windows MSVC | **PARTIAL** — phong ngua `std::abs`; chua log crash that |

---

## 1. BOI CANH (session 3)

User muon UI giong x360ce: **pad to giua**, control quanh.  
PS5 co touchpad → di chuot kieu Mac trackpad + chinh nhay; hoac map click touchpad → 1 nut bat ky.

Khong port x360ce (C#/WinForms). Chi layout FLTK 1.3 + SDL2.

---

## 2. UI LAUNCHER (session 3)

### 2.1 Cua so / layout

- Window: **780×580** (`size_range` 780×580).
- Tab Controller: pad `ControllerView` giua `(250,110,280,210)`.
- Bo `Fl_Scroll` bang trai; 14 bind = 2 cot co dinh quanh pad.
  - Trai: LT, LB, Back, D-Up, D-Left, D-Right, D-Down  
  - Phai: RT, RB, Start, Y, X, B, A  
- Top bar: Enable | Xbox/PS5 | L stick | R stick | status.
- Help 1 dong day.

### 2.2 Hang Touchpad (PS5 only — an khi Xbox)

```
[Touchpad] [Cursor|Button]  [slider sens]     ← mode Cursor
[Touchpad] [Cursor|Button]  [kind] [value]    ← mode Button
```

| Mode | UI | Hanh dong |
|---|---|---|
| Cursor | `Fl_Value_Slider` 200–4000 step 50 default 1100 | Vuot touchpad = chuot relative |
| Button | kind + value (giong hang pad) | Click touchpad = `touchpad_out` |

Widgets: `touchpadSensLabel`, `touchpadModeChoice`, `touchpadSensSlider`, `touchpadKindChoice`, `touchpadValueChoice`.  
`refreshTouchpadRow()` show/hide theo `layoutIndex` + `touchpadMode`.

### 2.3 Crash fix quit-to-launcher

Stack: `maintainSubProcessState` → `updateLogs()` → `logsDisplay->buffer()` **NULL** → SIGSEGV.  
Fix: `show()` gan `logsDisplay->buffer(&logsBuffer)`; `updateLogs()` null-safe + ghi qua `logsBuffer`.

---

## 3. ENGINE

### 3.1 Stick / pad bind (giu nhu session 2)

Pad token → output:
```
mouse:left|right|middle
wheel:up|down
nudge:up|down|left|right
key:<name>
none
```
Stick: `left_stick` / `right_stick` = `none|cursor|wasd|arrow`. Chi 1 stick `cursor`.

### 3.2 PS5 touchpad (moi)

SDL 2.0.14+:
- `SDL_GameControllerGetNumTouchpads` / `GetTouchpadFinger`
- Events: `SDL_CONTROLLERTOUCHPADDOWN|MOTION|UP`
- Click nut: `SDL_CONTROLLER_BUTTON_TOUCHPAD` token `"touchpad"`

| `touchpad=` | Hanh vi |
|---|---|
| `cursor` (default) | Relative mouse; `g_tpSensPx` (full swipe ≈ N logic px) |
| `button` | Tat swipe; bind click qua `touchpad_out` → `BindPadToken("touchpad", …)` |
| `off` / `none` / `0` | Tat touchpad |

`touchpad_sens=` clamp 200..4000, default 1100.  
Poll path `UpdateTouchpadCursor()` + event path (an toan neu host bo event).

### 3.3 `~/.ja2/controller.ini` (format hien tai)

```ini
enabled=1
layout=ps5
left_stick=cursor
right_stick=none
touchpad=cursor
touchpad_sens=1100
touchpad_out=none
a=mouse:left
b=mouse:right
leftshoulder=wheel:up
rightshoulder=wheel:down
dpup=key:up
dpdown=key:down
dpleft=key:left
dpright=key:right
start=key:return
back=key:escape
# … con lai pad tokens
```

Button mode vi du:
```ini
touchpad=button
touchpad_out=mouse:left
```

---

## 4. FILE LIEN QUAN

```
src/sgp/GameController.cc / .h     pad bind + stick + touchpad cursor/button
src/sgp/Input.cc / .h              PadInject*
src/sgp/SGP.cc                     init + events + Update
src/sgp/CMakeLists.txt             GameController.cc
src/launcher/ControllerView.h      PNG + sketch (header-only)
src/launcher/Launcher.cc / .h      remap UI, ini, logs buffer fix, touchpad row
src/launcher/StracciatellaLauncher.cc / .h   layout 780 + widgets
assets/externalized/controller_ps5.png
assets/externalized/controller_xbox.png
assets/externalized/gamecontrollerdb.txt
OGVM_HANDOFF.md
CONTROLLER_REMAP_SPEC.md           checklist cu (format da doi)
```

---

## 5. VIEC CON LAI

1. Smoke test full: enable → bind vai nut → Play → Quit → launcher con song + Logs tab.
2. PS5: Cursor vuot + doi sens; Button click map key/mouse.
3. Xbox: hang Touchpad an; pad bind binh thuong.
4. Hotplug rut/cam pad.
5. Windows MSVC: build + paste log neu crash.
6. Optional polish: line noi nut→label (x360ce), click-to-listen (option B), multi-pad.

---

## 6. LUU Y MOI TRUONG

- Duong dan co space/`()` → boc nhay kep.
- Truoc push: `git diff --cached --name-only | grep -E '^(build|build-win)/'` rong.
- FLUID khong co — sua `.cc`/`.h` tay; `.fl` bo qua.
- Chay tu `build/`. PNG: rebuild **ja2** (POST_BUILD copy) hoac  
  `cp assets/externalized/controller_*.png build/externalized/`
- Shell classifier co the chan `cmake --build` trong agent — user build local.

---

## 7. SO LIEU NHANH

| Hang so | Gia tri |
|---|---|
| AXIS_DEADZONE | 8000 |
| MAX_SPEED_PX | 600 |
| TRIGGER_THRESH | 12000 |
| STICK_KEY_THRESH | 16000 |
| NUDGE_PX | 40 |
| TP_SENS_DEFAULT | 1100 |
| TP_SENS range UI | 200–4000 step 50 |
| Pad bind rows | 14 |
| Window | 780×580 |
| Home ini | `~/.ja2/controller.ini` |

---

## 8. SESSION 3 DIFF TOM TAT

1. Layout x360ce-ish: pad center, 2 cot kind/value, cua so 780.
2. PS5 touchpad relative mouse + `touchpad_sens` slider.
3. Touchpad mode dropdown Cursor | Button (+ kind/value bind).
4. Launcher crash khi Quit game: attach `logsBuffer`.
