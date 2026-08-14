# HANDOFF — 13/08/2026

## Android 934x480 — History scrollbar buttons

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Resolution | 934x480 target |
| Scope | Strategic-map history scrollbar controls |
| Status | Code change completed; runtime verification pending |

---

## User request

The history frame is stretched vertically at widescreen resolution. Reposition the history up/down buttons into the two visible empty holes of the stretched scrollbar artwork.

Required behavior remains unchanged:

- Existing up/down callbacks.
- Single-click scrolling.
- Repeat scrolling.
- Right-click scrolling.
- Mouse-wheel scrolling.
- Slider interaction.
- Disabled/enabled states.

Vanilla and full-size geometry must remain unchanged.

---

## Implemented change

Changed [src/game/Strategic/Map_Screen_Interface_Bottom.cc](../src/game/Strategic/Map_Screen_Interface_Bottom.cc).

Widescreen-only geometry now maps the native scrollbar artwork to the stretched frame:

- Native frame size: `261x409`.
- Destination frame at 934x480: `171x480`.
- Native scrollbar x region: approximately `238..250`.
- Widescreen scrollbar x: approximately `156`.
- Upper hole native y: approximately `12`.
- Upper button destination y: approximately `14`.
- Lower hole native y: approximately `381`.
- Lower button destination y: approximately `447`.
- Scroll track: stretched from native y `27..381` to approximately screen y `31..446`.

The coordinate formulas use `SCREEN_HEIGHT` and the original `261x409` asset dimensions, so the widescreen geometry scales with the target resolution.

Only `g_ui.isWidescreenLayout()` uses the new coordinates. Existing `g_ui.isWidePanel()` and vanilla branches remain unchanged.

---

## Expected 934x480 geometry

```text
history frame:       x=0..170, y=0..479
scrollbar x:         156
upper button:        y≈14
scroll track:        y≈31..446
lower button:        y≈447
```

The buttons should visually occupy the upper and lower empty holes in the stretched scrollbar. The slider remains inside the stretched track.

---

## Verification

Passed:

```bash
git diff --check -- src/game/Strategic/Map_Screen_Interface_Bottom.cc
```

Not completed:

- Native C++ build. The filtered build command was blocked by a temporary environment classifier outage.
- Android build after this geometry change.
- APK installation and runtime screenshot.
- Touch/hitbox verification at 934x480.
- Regression checks at 800x600 and 1024x768/1280x720+.

Recommended commands:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:" | tail -30
./tools/build-android-debug.sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Runtime checks at 934x480:

- Upper button aligns with the upper hole.
- Lower button aligns with the lower hole.
- Hit rectangles match the visible buttons.
- Slider remains usable across the full stretched track.
- History scrolling, repeat, right-click, and wheel behavior remain functional.
- Disabled states remain correct.
- Surface and underground maps remain aligned.

Regression resolutions:

- 800x600: centered vanilla layout unchanged.
- 1024x600: widescreen history geometry remains correct.
- 1024x768 / 1280x720+: full-size Wildfire layout unchanged.
