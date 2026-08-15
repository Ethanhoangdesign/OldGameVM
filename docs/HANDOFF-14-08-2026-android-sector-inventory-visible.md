# HANDOFF — Android Sector Inventory Visible

## Context

- Repo: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`
- Branch: `feature/multi-edition-detector`
- Date: 2026-08-14
- Target device layout: Android `934x480`
- Modified file: `src/game/Strategic/Map_Screen_Interface_Map_Inventory.cc`
- No commit/push requested.

## Initial symptom

On a real Android device, opening Sector Inventory showed no inventory panel.

The scaled Wildfire art is `763x647`. The first implementation created a temporary compose surface sized only to the logical screen (`934x480`). `BltStretchVideoSurface()` rejected the source rectangle because the source height (`647`) exceeded the surface height (`480`), so the stretch returned without drawing.

## Root cause

`BlitInventoryPoolGraphic()` used a temporary 16-bit compose surface sized to `SCREEN_WIDTH x SCREEN_HEIGHT`.

For the Wildfire scaled path:

- native source: `763x647`
- display destination: approximately `423x359`
- temporary source surface: `934x480` — too short
- `BltStretchVideoSurface()` source bounds check rejected `763x647`
- result: inventory invisible, no explicit runtime error

## Fix applied

In `Map_Screen_Interface_Map_Inventory.cc`:

1. Temporary compose surface now uses at least both screen and native art dimensions:

   - width: `max(SCREEN_WIDTH, native_w)`
   - height: `max(SCREEN_HEIGHT, native_h)`
   - 16-bit surface

2. Removed `SetTransparency(0)` from the compose surface.

   Black is valid inventory artwork/background. Treating black as transparent could erase valid pixels during the stretch.

3. Added temporary clipping rectangle covering the complete compose surface.

4. Restored the previous global clipping rectangle after native inventory rendering.

5. Existing shared transform remains responsible for:

   - inventory background
   - item art
   - status bars
   - labels/text
   - page/count/sector text
   - mask
   - slot hitboxes
   - previous/next/done hitboxes

6. Scaled controls still use hotspot regions plus manually rendered button art. Vanilla and full-size Wildfire paths remain unchanged.

## Verification

- Android C++ build initially failed because `old_clip` was scoped inside the scaled-render block but restored outside it.
- Fixed by declaring `old_clip` in the surrounding function scope.
- `git diff --check`: pass.
- User rebuilt and installed the Android APK.
- User confirmed Sector Inventory is now visible on the real device.

## Commands

Android build:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android"
./gradlew app:assembleDebug --no-daemon --console=plain
```

Build, install, launch on the first ready physical device:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android" && \
adb start-server >/dev/null; \
printf 'Đang chờ device thật...\n'; \
while :; do \
  SERIAL=$(adb devices | awk '$1 !~ /^emulator-/ && $2=="device" {print $1; exit}'); \
  if [ -n "$SERIAL" ] && [ "$(adb -s "$SERIAL" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; then break; fi; \
  sleep 2; \
done; \
printf 'Device sẵn sàng: %s\n' "$SERIAL"; \
./gradlew app:assembleDebug --no-daemon --console=plain && \
adb -s "$SERIAL" install -r app/build/outputs/apk/debug/app-debug.apk && \
adb -s "$SERIAL" shell am force-stop io.github.ja2stracciatella && \
adb -s "$SERIAL" shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

## Device test status

Confirmed:

- Sector Inventory opens.
- Inventory panel renders visibly on real Android device.

Still recommended:

- Tap every item slot.
- Tap previous/next.
- Change page.
- Tap Done.
- Close with right mouse button/mask input if available.
- Open and close repeatedly; verify no stale art.
- Test no-item and multi-item sectors.
- Test underground inventory.
- Regression check vanilla `640x480`.
- Regression check full-size Wildfire `1024x720`, `1024x768`, `1280x720`.

## Remaining risks

- Global framebuffer/font/clipping swaps are manually restored, not RAII guarded.
- Temporary compose surface is allocated on each scaled redraw.
- Geometry regression test is not yet added.
- No commit created.

## Next action

Run the remaining touch/close/regression checks. If all pass, review diff. Commit only when explicitly requested.
