# HANDOFF — 02/09/2026

**Strategic map: explored sectors show enemy count**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `Ethanhoangdesign/OldGameVM` |
| Local | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Android package | `io.github.ja2stracciatella` |
| Device | `R5GL31H83QX` |

---

## 0. STATUS

| Item | Status |
|---|---|
| Bright/explored surface sector enemy display | **DONE** |
| Exact red enemy-icon rendering restored | **DONE** |
| Desktop build | **DONE** |
| Android debug build | **DONE** |
| Android install | **DONE** |
| Android launcher start | **DONE** |
| In-game visual check on a specific save | Pending |

---

## 1. USER REQUIREMENT

When a sector has been explored and is bright on the strategic map, display the full current enemy count. Do not keep displaying a red `?`.

Expected behavior:

- Explored/bright surface sector with enemies: one red enemy icon per current enemy.
- Unexplored sector: no enemy marker.
- Genuine uncertainty: red `?`.
- Zero enemies: no marker.
- Underground and existing stationary-garrison rules: unchanged.

Brightness uses `SF_ALREADY_VISITED`; it is strategic-map exploration state, not tactical FOV.

---

## 2. CODE CHANGE

File: `src/game/Strategic/Map_Screen_Interface_Map.cc`

### Exact-count renderer

`ShowEnemiesInSector()` again draws the requested number of red enemy icons:

```cpp
while (n_enemies-- != 0)
{
    DrawMapBoxIcon(guiCHARICONS, SMALL_RED_BOX, sMap, icon_pos++);
}
```

### Explored-sector knowledge

`WhatPlayerKnowsAboutEnemiesInSector()` now returns `KNOWS_HOW_MANY` for `SF_ALREADY_VISITED`:

```cpp
if (GetSectorFlagStatus(sSector, SF_ALREADY_VISITED))
{
    // Explored (bright) sectors show the current enemy count on the map.
    return KNOWS_HOW_MANY;
}
```

Enemy count source remains `NumEnemiesInSector()`. No count calculation, map shading, save format, API, asset, or dependency changes.

The shared knowledge result also makes visited-sector town/mine information exact, matching the strategic map.

---

## 3. VALIDATION

### Static

```bash
git diff --check
```

Result: passed.

### Android

```bash
./tools/build-android-debug.sh
```

Result: `BUILD SUCCESSFUL in 6s`.

```bash
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
```

Result: `Success`.

```bash
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Result: launcher activity started and resumed.

Filtered logcat check found no `FATAL EXCEPTION`, `AndroidRuntime`, `SIGSEGV`, `SIGABRT`, or ANR.

Specific-save visual verification was not performed. Next session should load a save with an explored sector containing enemies and confirm icon count visually.

---

## 4. FILES

| File | Change |
|---|---|
| `src/game/Strategic/Map_Screen_Interface_Map.cc` | Restore exact enemy icons; explored sectors return `KNOWS_HOW_MANY` |
| `docs/HANDOFF-02-09-2026-strategic-map-enemy-count.md` | This handoff |

---

## 5. NEXT SESSION

1. Load a valid save on Android.
2. Open the strategic map.
3. Confirm a bright sector with enemies shows the full red count, not `?`.
4. Confirm unexplored sectors remain blank.
5. Confirm genuine uncertainty still shows red `?`.

---

## 6. COMMIT MESSAGE

```text
OGVM-ANDROID: show enemy count in explored map sectors

Treat already-visited strategic-map sectors as exact enemy-count knowledge
and restore the red icon loop for each current enemy.

Handoff 02-09 strategic map enemy count.
```
