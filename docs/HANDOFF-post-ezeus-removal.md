# HANDOFF: Post eZeus Removal — Multi-Edition Detector Baseline

**Date:** 2026-08-19  
**Branch:** `feature/multi-edition-detector`  
**Commit:** `90abc4d3a` — "OGVM-ANDROID: improve controller input and tactical performance"

---

## Summary

Đã revert commit eZeus (`15a95842e`) về trạng thái baseline trước khi tích hợp eZeus. Branch giờ ở commit `90abc4d3a` — bản ổn định nhất trước khi thêm eZeus scaffolding.

---

## What Was Removed

Commit `15a95842e` ("OGVM-ANDROID: multi-edition detector + eZeus support scaffolding") introduced:
- **Android:** `EZeusActivity.kt`, `EZeusStatusFragment.kt`, `GameId.kt` multi-edition wiring, launcher dropdown
- **CMake:** `cmake/ezeus.cmake` (137 lines)
- **C++:** `DefaultContentManager.cc`, `ItemModel.cc/h`, `Interface_Items.cc` multi-edition support
- **Docs:** 8 handoff files về eZeus startup/instrumentation/commercial data

Tất cả đã được xoá qua `git reset --hard 90abc4d3a`.

---

## Current State (Commit 90abc4d3a)

### Android Improvements (this commit)
- Controller input improvements
- Tactical performance optimizations
- Map overlay alignment fix (from `304d5e5de`)
- Insurance popup dismissal fix (from `434b03cba`)
- Tactical turn speed control (from `afccda077`)

### C++ Changes (this commit + parents)
- `src/game/Tactical/Interface.cc` — controller/input
- `src/game/Tactical/LOS.cc` — line of sight optimizations
- `src/game/TacticalAI/AIMain.cc`, `DecideAction.cc`, `FindLocations.cc` — AI performance
- `src/sgp/GameController.cc` — game controller fixes

### Untracked Directories (not in git)
```
android/GOG/           # GOG commercial data (ignored)
android/Zeus + Poseidon/  # Zeus+Poseidon commercial data (ignored)
android/ezeus/         # eZeus runtime (ignored)
```
> These are in `.gitignore` — intentional, không commit.

---

## Next Steps

### Option A: Re-implement eZeus Cleanly
1. Tạo branch mới: `git checkout -b feature/ezeus-clean`
2. Chỉ add `GameId.kt` multi-edition detector (core logic)
3. eZeus runtime load sau khi detector xác định edition
4. Không thêm fragmentation vào launcher UI trước khi core stable

### Option B: Continue Android Polish
- Build current baseline: `cd android && ./gradlew installDebug`
- Test controller/input/tactical performance
- Fix remaining UI/layout issues

### Option C: Sync with Upstream
- `git fetch upstream && git rebase upstream/master`
- Merge upstream fixes before next feature

---

## Build Commands

```bash
# Android debug build + install
cd android && ./gradlew installDebug

# Clean build
cd android && ./gradlew clean installDebug

# CMake desktop (nếu cần)
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build -j$(nproc)
```

---

## Related Branches
- `backup-ezeus-before-reset` — chưa tạo (timeout), có thể tạo từ `15a95842e` nếu cần rollback
- `origin/feature/multi-edition-detector` — remote có commit eZeus, local đã revert

---

## Notes
- Không có thay đổi uncommitted trừ 3 dirs commercial data (ignored)
- Baseline này stable cho Android controller/tactical
- eZeus integration nên tách riêng, không gộp vào multi-edition detector