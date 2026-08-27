# HANDOFF — Load Game Opens at Newest Save

## Problem

The save/load screen stores its scroll position in the static `gCurrentScrollTop`. Reopening **Load Game** reused the previous position, so the newest saves could be hidden above the visible list.

## Change

- `src/game/SaveLoadScreen.cc`
  - `EnterSaveLoadScreen()` now resets `gCurrentScrollTop` to `0` when `gfSaveGame` is false.
  - Load entries remain sorted by modification time descending through `compareSaveGames()`.
  - Save mode keeps its existing scroll position behavior.
  - Quick Load behavior is unchanged.

## Result

Opening **Load Game** always shows the newest save at the top. No manual scroll-up is required.

## Verification

- `git diff --check`: passed.
- Expected Android build/install:

```bash
cd android
./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

- Manual test: scroll down in Load Game, exit, reopen; scrollbar returns to the top and newest save is visible.
- Manual test: Save Game still opens with its existing behavior.
