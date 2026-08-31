# Handoff — Multi-Edition Branch Session State

**Ngày:** 2026-08-18  
**Nhánh:** `feature/multi-edition-detector`  
**Trạng thái:** Chưa commit/stage/push. Nhiều luồng mở song song.

## Session này làm gì

Chỉ recover context + khảo sát trạng thái. Không sửa code, không build, không chạy emulator.

- List + đọc top handoff docs.
- Snapshot git state.
- Inspect untracked data dirs (`android/GOG/`, `android/Zeus + Poseidon/`, `android/ezeus/`).

## Git state

Modified (12): `CMakeLists.txt`, `android/app/build.gradle`, `AndroidManifest.xml`, `LauncherActivity.kt`, `SectionsPagerAdapter.kt`, `activity_launcher.xml`, `strings.xml`, `dependencies/lib-sdl2/CMakeLists.txt`, `src/externalized/DefaultContentManager.cc`, `src/externalized/ItemModel.cc`, `ItemModel.h`, `src/game/Tactical/Interface_Items.cc`.

Untracked code: `EZeusActivity.kt`, `GameId.kt`, `EZeusStatusFragment.kt`, `fragment_ezeus_status.xml`, `cmake/ezeus.cmake`, `android/ezeus/` (`egamedir.cpp`, `ezeus_android_main.cpp`, `ezeus_android_smoke.cpp`).

Untracked data (KHÔNG commit — lớn, bản quyền):
- `android/GOG/` — GOG JA2 + Wildfire installers.
- `android/Zeus + Poseidon/` — `Zeus + Poseidon.zip` 476M, `jagged_alliance_2_2.0.0.4.dmg` 742M, GOG exe/zip 628M mỗi cái.
- `android/ezeus/` — binks/DLL runtime.

## Luồng mở

1. **eZeus first-frame text loader blocked** — background+interface texture render, SDL window sống, `window.exec()` chưa return, menu text chưa hiện. Không phải lỗi build/library/SDL/commercial-data/texture. Xem `HANDOFF-ezeus-first-frame-text-loader-blocked.md`.
2. **Android inventory icon mismatch** — root cause đã chứng minh (tooltip vs artwork đi hai data path độc lập), Wildfire mapping fix đã build+cài; visual inventory matrix chưa xong. Xem `HANDOFF-android-inventory-pants-magazine.md`.
3. **Multi-edition detector** — branch nền, chưa có handoff riêng.

## Next step

- Xác định luồng ưu tiên trước khi commit.
- Data dirs phải nằm ngoài Git (`.gitignore` hoặc external root) — kiểm tra trước khi stage.
