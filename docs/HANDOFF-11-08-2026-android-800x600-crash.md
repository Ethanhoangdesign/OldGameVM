# HANDOFF — 11/08/2026

**Android 800x600 launch crash — temp-surface clipping fix**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | Android resolution preset / native graphics blit crash |
| Package | `io.github.ja2stracciatella` |
| Launcher | `io.github.ja2stracciatella/.LauncherActivity` |
| Primary symptom | Select `800x600` in Android launcher → game immediately crashes on launch |

---

## 0. STATUS

| Work | Status | Note |
|---|---|---|
| Reproduce / analyze crash path | DONE | Crash is in native ETRLE transparent blitter after selecting `800x600` |
| Root-cause resolution-specific path | DONE | `800x600` uses larger background art then stretch-blits to screen |
| Main menu fix | DONE | `MainMenuScreen.cc` now blits temp surface with temp-sized clipping rect |
| Other background stretch paths | DONE | GIO/options/save-load protected too |
| Android button stretch temp paths | DONE | Same temp-surface blit pattern protected in `Button_System.cc` |
| Android unit test build path | DONE | `cd android && ./gradlew testDebugUnitTest` passed |
| Android debug APK build | DONE | `cd android && ./gradlew assembleDebug` passed |
| Live emulator manual verification | TODO | Install/run on simulator and select `800x600` to confirm no launch crash |

---

## 1. USER-OBSERVED BUG

Android launcher exposes these resolution presets in `ConfigurationModel.kt`:

- `1366x768`
- `1280x768`
- `1024x768`
- `800x600`
- `640x480`
- `1664x768`

Bug report:

> Chọn `800x600`, nhấn chạy game là văng app ngay.

The important detail is that `800x600` is not one of the original/native UI art dimensions. Several screens use larger STI background assets and stretch them down/up to the actual screen.

---

## 2. ROOT CAUSE

The crash is not caused by the launcher Kotlin preset itself. The preset correctly passes `800x600` into native engine options.

The crash happens in the C++ graphics path:

1. At `800x600`, main menu chooses larger background art, usually `1024x768`.
2. That STI video object is 8bpp ETRLE compressed.
3. Stretch path requires a 16bpp source surface, so code creates a temp surface matching native art size:

   ```cpp
   SGPVSurface tmp(bgProps.usWidth, bgProps.usHeight, 16);
   BltVideoObject(&tmp, vo, 0, 0, 0);
   BltStretchVideoSurface(FRAME_BUFFER, &tmp, &src, &dst);
   ```

4. `BltVideoObject()` does not clip against the destination temp surface. It clips against the global `ClippingRect`.
5. At `800x600`, global `ClippingRect` is screen-sized: `0,0,800,600`.
6. Drawing a `1024x768` object into a `1024x768` temp surface is therefore incorrectly considered right/bottom-clipped.
7. That sends execution through clipped `BltTransparent()` in `VObject_Blitters.cc`.
8. The clipped ETRLE parser has a latent bug when right/bottom clipping is active; source pointer parsing can go invalid and segfault.

Key native logic:

```cpp
ClipInfo const ci{ src, iDestX, iDestY, usRegionIndex, &ClippingRect };
if (ci.status != ClipInfo::Status::Not_Clipped)
{
    BltTransparent(ci, pBuffer, uiPitch);
}
else
{
    Blt8BPPDataTo16BPPBufferTransparent(...);
}
```

So the immediate fix is to prevent false clipping while preparing the temp surface.

---

## 3. FIX APPLIED

For each temp-surface preparation path, temporarily set global clipping to the temp surface bounds before calling `BltVideoObject(&tmp, ...)`, then restore the old clipping rect immediately afterward.

Pattern applied:

```cpp
SGPVSurface tmp(width, height, 16);
SGPRect tmpClip;
tmpClip.set(0, 0, tmp.Width(), tmp.Height());
SGPRect const oldClip = SetClippingRect(tmpClip);
BltVideoObject(&tmp, vo, sub, 0, 0);
SetClippingRect(oldClip);
```

This makes `BltVideoObject()` see the temp-surface draw as `Not_Clipped`, so it uses the safe non-clipped ETRLE blit path.

Important: this does **not** change final screen clipping or stretch behavior. It only fixes the temporary conversion surface used before `BltStretchVideoSurface()`.

---

## 4. FILES CHANGED FOR THIS FIX

### `src/game/MainMenuScreen.cc`

Added:

```cpp
#include "VObject_Blitters.h"
```

Protected main menu background temp draw. This is the primary `800x600` launch crash path.

### `src/game/GameInitOptionsScreen.cc`

Added:

```cpp
#include "VObject_Blitters.h"
```

Protected initial game options background temp draw.

### `src/game/Options_Screen.cc`

Added:

```cpp
#include "VObject_Blitters.h"
```

Protected Android `StretchVO()` helper.

### `src/game/SaveLoadScreen.cc`

Already included `VObject_Blitters.h`.

Protected `SlgStretchVO()` helper.

### `src/sgp/Button_System.cc`

Already included `VObject_Blitters.h`.

Protected two Android button-art stretch paths:

- enlarged quick button draw
- 2x checkbox button draw

These were not the reported launch crash path, but they used the same temp-surface + `BltVideoObject(&tmp, ...)` pattern and could trigger the same clipping bug under some layouts.

---

## 5. WHY NOT FIX `BltTransparent()` DIRECTLY?

The deeper latent bug is probably in the clipped ETRLE parser in `VObject_Blitters.cc`, especially around right-skip / line-skip handling.

However, the reported crash is caused by a false clipping context:

- Destination is a temp surface sized to the source art.
- There should be no clipping at all.
- The only reason clipping happens is because `BltVideoObject()` consults the global screen `ClippingRect`.

Changing temp-surface clipping is safer and narrower than rewriting the low-level ETRLE parser. The core blitter can be audited separately later.

---

## 6. VERIFICATION DONE

From repo root / Android folder:

```sh
cd android && ./gradlew testDebugUnitTest
```

Result:

```text
BUILD SUCCESSFUL
```

Then:

```sh
cd android && ./gradlew assembleDebug
```

Result:

```text
BUILD SUCCESSFUL
```

Native CMake debug build also completed during `assembleDebug`:

```text
:app:configureCMakeDebug[arm64-v8a]
:app:buildCMakeDebug[arm64-v8a]
:app:externalNativeBuildDebug
:app:assembleDebug
BUILD SUCCESSFUL
```

---

## 7. COMMAND TO RUN ON MAC SIMULATOR

Use one terminal command from repo root:

```sh
cd android && ./gradlew installDebug && adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

If already inside `android/`, use:

```sh
./gradlew installDebug && adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Check emulator connection:

```sh
adb devices
```

Expected device observed during session:

```text
emulator-5554    device
```

---

## 8. MANUAL TEST CHECKLIST

On simulator:

1. Launch app.
2. Go to Settings / resolution preset.
3. Select `800x600`.
4. Start game.
5. Expected: game reaches main menu instead of immediate crash.
6. Optional extra screens:
   - Start new game and reach initial game options.
   - Open Options screen.
   - Open Save/Load screen.
   - Confirm scaled backgrounds still render.

If crash still occurs, collect native log:

```sh
adb logcat | grep -i "fatal\|backtrace\|libja2\|BltTransparent\|SIGSEGV"
```

---

## 9. RELATED CONTEXT

Previous Android settings/controller handoff:

- `docs/HANDOFF-08-08-2026-android-settings-controller.md`

Related resolution/layout file:

- `android/app/src/main/java/io/github/ja2stracciatella/ConfigurationModel.kt`
- `src/game/UILayout.cc`

Related low-level graphics files:

- `src/sgp/VObject.cc`
- `src/sgp/VObject_Blitters.cc`
- `src/sgp/VObject_Blitters.h`

---

## 10. NEXT STEPS

Recommended next actions:

1. Run simulator command above.
2. Select `800x600` and confirm no launch crash.
3. If fixed, commit this focused graphics fix separately from unrelated controller/settings changes if possible.
4. Later, consider a deeper defensive audit of `BltTransparent()` clipped ETRLE parsing, but do not mix that with this narrow Android preset fix.
