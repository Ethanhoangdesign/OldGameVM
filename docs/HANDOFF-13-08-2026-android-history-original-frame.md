# HANDOFF — 13/08/2026

## Android 934x480 — Original history-log frame

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 |
| Scope | Strategic-map history log and bottom panel |
| Status | **Completed — Android build fixed and confirmed passing** |

---

## User request

The history log was moved upward to remove the empty space. The synthetic hand-drawn frame looked wrong and overlapped surrounding UI. Use the original JA2/Wildfire frame instead.

---

## Implemented

### Layout

At 934x480:

```text
screen width: 934px
Wildfire bottom panel: 763px
panel x: 934 - 763 = 171
history column: x=0..170
```

`UILayout::get_MAP_BOTTOM_BASE_X()` returns `m_screenWidth - 763` for widescreen layouts.

`MapScreenLogTop()` returns `18` for widescreen layouts. The history text starts near `y=33`.

### Original frame

Added asset:

```text
assets/externalized/sti/interface/map_screen_log.sti
```

Copied from the Wildfire build asset:

```text
build/externalized/interface/map_screen_log_fix.sti
```

Decoded native dimensions:

```text
261x409
```

Widescreen rendering currently attempts to stretch the original asset across the left column and full screen height:

```cpp
RenderMapScreenLogFrame(0, 0,
    (INT32)g_ui.get_MAP_BOTTOM_BASE_X(), SCREEN_HEIGHT);
```

The synthetic bevel remains as an asset-missing fallback.

### Relevant files

- `src/game/Strategic/Map_Screen_Interface_Bottom.cc`
  - History frame rendering.
  - Widescreen restore region.
  - History box and scrollbar coordinates.
- `src/game/Utils/Message.cc`
  - History text clipping and wrapping.
- `src/game/UILayout.cc`
  - Widescreen detection.
  - Right-anchored bottom panel.
- `assets/externalized/sti/interface/map_screen_log.sti`
  - Original frame asset.

---

## Android build fix

Command:

```bash
./tools/build-android-debug.sh
```

Initial failure:

```text
src/game/Strategic/Map_Screen_Interface_Bottom.cc:306:9:
error: use of undeclared identifier 'GCM'
```

First attempted include:

```cpp
#include "GameRes.h"
```

`GameRes.h` does not declare `GCM`. `GameInstance.h` declares the global pointer, but only forward-declares `ContentManager`, causing the next error:

```text
error: member access into incomplete type 'ContentManager'
```

Final fix in `src/game/Strategic/Map_Screen_Interface_Bottom.cc`:

```cpp
#include "ContentManager.h"
#include "GameInstance.h"
```

`ContentManager.h` provides the complete class definition; `GameInstance.h` provides `extern ContentManager *GCM`.

Android build subsequently passed. APK generation is unblocked.

---

## APK status

Android build now passes after including the complete `ContentManager` definition and the `GCM` declaration. APK installation/runtime verification remains a separate step:

```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Runtime screenshot status: not recorded in this handoff.

---

## Non-blocking warnings

```text
src/game/Tactical/Interface.cc:1783
warning: variable 'fTileBar' set but not used
```

Also reported previously:

- Lua deprecated string-plus-int warning.
- Rust unused variables.
- Kotlin deprecated `requestPermissions(...)` warning.

These do not block the native build.

---

## Required next steps

1. Install the successfully built APK:

   ```bash
   adb install -r android/app/build/outputs/apk/debug/app-debug.apk
   adb shell am force-stop io.github.ja2stracciatella
   adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
   ```

2. Verify at 934x480:
   - Original Wildfire frame visible.
   - Frame stays inside the left history column.
   - No overlap with map or roster.
   - History text remains inside the frame.
   - Scrollbar arrows and slider remain usable.

5. If full stretching distorts the artwork, replace it with a tiled or 9-slice rendering strategy using the original `261x409` asset.

6. Regression-check:
   - 1024x768 / 1280x720+: full-size layout.
   - 800x600 / 640x480: centered vanilla layout.
   - 1024x600: right-anchored 763px panel.

---

## Verification before handoff

Desktop build previously passed with no diagnostics:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
```

Android native build is unblocked and reported passing after the include fix. Remaining verification: install APK and inspect the 934x480 runtime layout.
