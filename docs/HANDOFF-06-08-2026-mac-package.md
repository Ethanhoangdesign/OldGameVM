# HANDOFF — 06/08/2026, Mac official package + bundle fix

Branch: `feature/multi-edition-detector`  
Repo: `Ethanhoangdesign/OldGameVM` → `origin`  
Local: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`  
May: macOS arm64  

Android handoff: `OGVM_ANDROID.md`  
Controller handoff: `OGVM_HANDOFF.md`  
Map chrome: `docs/HANDOFF-06-08-2026-map-chrome.md`

---

## 0. TRẠNG THÁI

| Việc | Trạng thái |
|---|---|
| Push branch (MAPCHROME + SHORTERR + MAC fix) | **DONE** → `origin/feature/multi-edition-detector` |
| Map chrome desktop = mobile | **DONE** — cùng C++ (`56a34966a`), không port riêng |
| Mac DMG official (`CPACK_GENERATOR=Bundle`) | **DONE** |
| Fix “damaged or incomplete” | **DONE** (`88920e135`) |
| User smoke open OldGameVM.app | **DONE** — “ok được rồi” |
| Commit `build-mac-release/` | **Không** — local only |
| Signed Developer ID / notarize | **Chưa** — ad-hoc / unsigned; cần `xattr -cr` lần đầu |

HEAD (lúc handoff): `88920e135` (hoặc mới hơn trên branch).

---

## 1. BỐI CẢNH SESSION

1. User: nền + khung map mobile OK → apply desktop.  
   → Đã có sẵn trong engine C++ chung (`MapScreen.cc` MAPFRAME + `Interface_Panels.cc` LEATHER-TINT). Rebuild `build/ja2` là đủ.
2. User: push GitHub + build launcher Mac chính thức.
3. DMG mở được nhưng app báo **damaged or incomplete** → mismatch tên bundle.

---

## 2. COMMITS ĐẨY TRONG SESSION

| Hash | Message |
|---|---|
| `56a34966a` | `OGVM-MAPCHROME: leather filler + full-size map frame` |
| `602808f79` | `OGVM-SHORTERR: short edition-detector status lines` |
| `88920e135` | `OGVM-MAC: fix OldGameVM.app bundle (plist + startup)` |

Remote: `https://github.com/Ethanhoangdesign/OldGameVM`  
Branch: `feature/multi-edition-detector`

**Không commit:** `layout controller/`, `ogvm-smoke*.png`, `build-mac-release/`, `*.sti`, game `Data/`.

---

## 3. NGUYÊN NHÂN “DAMAGED OR INCOMPLETE”

CPack:

```
CPACK_BUNDLE_NAME = OldGameVM
→ MacOS/OldGameVM
→ icon OldGameVM.icns
```

Nhưng `assets/distr-files-mac/` còn tên cũ upstream:

| File | Sai | Đúng |
|---|---|---|
| `BundleInfo.plist` `CFBundleExecutable` | `JA2 Stracciatella` | `OldGameVM` |
| `CFBundleIconFile` / `CFBundleName` | `JA2 Stracciatella` | `OldGameVM` |
| `ja2-startup.sh` strip path | `…/JA2 Stracciatella` | `dirname` → bundle root |

LaunchServices tìm executable sai tên → **damaged or incomplete** (không phải binary corrupt).

---

## 4. DIFF FIX MAC

### 4.1 `assets/distr-files-mac/BundleInfo.plist`

- Executable / Icon / Name: **OldGameVM**
- Identifier: `io.github.ja2-stracciatella.oldgamevm`
- Version strings: `0.23.0`
- `LSMinimumSystemVersion` 11.0, `NSHighResolutionCapable`

### 4.2 `assets/distr-files-mac/ja2-startup.sh`

```sh
BUNDLE="$(cd "$(dirname "$0")/../.." && pwd)"
RESOURCES="$BUNDLE/Contents/Resources"
# DYLD_FRAMEWORK_PATH → Frameworks/ hoặc Resources/SDL2.framework
exec "$RESOURCES/ja2-launcher"
```

---

## 5. BUILD LẠI DMG (Mac arm64)

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"

rm -rf build-mac-release
mkdir build-mac-release && cd build-mac-release

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=./cmake/toolchain-macos.cmake \
  -DCPACK_GENERATOR=Bundle \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_LAUNCHER=ON \
  -DLOCAL_FLTK_LIB=ON \
  -DWITH_UNITTESTS=OFF

cmake --build . -j8 --target ja2-launcher ja2
cmake --build . --target package -j8

# Artifact CPack:
#   ja2-stracciatella_0.23.0-git+<hash>_macos.dmg
```

**Lưu ý configure:**

| Option | Vì sao |
|---|---|
| `LOCAL_FLTK_LIB=ON` | `toolchain-macos.cmake` `FIND_ROOT_PATH_MODE_LIBRARY ONLY` → brew FLTK không thấy; local FLTK OK |
| `WITH_UNITTESTS=OFF` | GTest brew cũng fail cùng root-path |
| `CPACK_GENERATOR=Bundle` | giống CI mac (`.ci/ci-build.sh`) |

**Sau package (khuyến nghị):** ad-hoc sign staged app rồi đóng lại DMG nếu Gatekeeper khó tính:

```bash
APP=build-mac-release/_CPack_Packages/Darwin/Bundle/ja2-stracciatella_*_macos/OldGameVM.app
# (path đủ tên file CPack)
codesign --force --deep --sign - "$APP"
# hdiutil create -volname OldGameVM -srcfolder <parent-of-app> -ov -format UDZO out.dmg
```

Session này: copy DMG → `~/Downloads/`, staged app → `build-mac-release/OldGameVM.app`.

---

## 6. CÀI / MỞ (user)

```bash
# Xóa bản hỏng (plist cũ)
rm -rf /Applications/OldGameVM.app

open ~/Downloads/ja2-stracciatella_0.23.0-git+*_macos.dmg
# Kéo OldGameVM.app → Applications

xattr -cr /Applications/OldGameVM.app
open /Applications/OldGameVM.app
```

Hoặc không cài:

```bash
open ".../ja2-stracciatella/build-mac-release/OldGameVM.app"
```

Vẫn chặn: **Privacy & Security → Open Anyway**, hoặc Right-click → Open.

Smoke launcher: `ja2-launcher -help` trong `Contents/Resources/` in log / usage.

---

## 7. CẤU TRÚC BUNDLE

```
OldGameVM.app/
  Contents/
    Info.plist          ← CFBundleExecutable = OldGameVM
    MacOS/OldGameVM     ← startup.sh → ja2-launcher
    Resources/
      ja2-launcher
      ja2
      ja2-resource-pack
      SDL2.framework/   ← @rpath; startup set DYLD_FRAMEWORK_PATH
      externalized/     ← incl. controller_*.png
      mods/ unittests/
      OldGameVM.icns
      README.txt changes.md
```

CPack còn symlink `Applications → /Applications` trong volume DMG.

---

## 8. MAP CHROME (desktop) — tóm tắt

Không diff riêng mobile/desktop. Full-size khi:

- `UsesWildfireInterfaceArt()` (`b_map.sti` có, `b_map.pcx` không)
- res ≥ 1024×768

Code: `DrawFillerOnSurface` (tile + leather tint), block `MAPFRAME` trong `MapScreen.cc`.  
Chi tiết: `docs/HANDOFF-06-08-2026-map-chrome.md`.

Desktop dev binary: `./build/ja2` (config user `~/.ja2/ja2.json` WF + 1366 OK).

---

## 9. FILE LIÊN QUAN

```
assets/distr-files-mac/BundleInfo.plist
assets/distr-files-mac/ja2-startup.sh
assets/distr-files-mac/README.txt
assets/icons/logo.icns
CMakeLists.txt                    CPACK_BUNDLE_* / install mac SDL2.framework
cmake/toolchain-macos.cmake
src/launcher/CMakeLists.txt       BUILD_LAUNCHER, FLTK
.ci/ci-build.sh                   CI mac: Bundle + toolchain-macos
docs/HANDOFF-06-08-2026-mac-package.md   handoff này
build-mac-release/                LOCAL — DMG + OldGameVM.app (không commit)
```

---

## 10. BACKLOG

1. Developer ID sign + notarize (bỏ `xattr -cr` / Open Anyway).
2. Optional: `CFBundleShortVersionString` lấy từ `version` file / CMake thay hardcode `0.23.0`.
3. Optional: chuyển `SDL2.framework` vào `Contents/Frameworks` (chuẩn hơn Resources) + rpath.
4. CI artifact Mac từ branch OGVM (verify plist name trên CI package).
5. `build-mac-release/` có thể thêm `.gitignore` nếu hay đụng local.

---

## 11. LỆNH NHANH

```bash
# push
git push origin feature/multi-edition-detector

# rebuild package
cd ".../ja2-stracciatella/build-mac-release"   # hoặc configure mới như §5
cmake --build . -j8 --target ja2-launcher ja2 package

# install clean
rm -rf /Applications/OldGameVM.app
# mount DMG, copy app, rồi:
xattr -cr /Applications/OldGameVM.app && open /Applications/OldGameVM.app
```

---

## 12. SESSION TÓM TẮT

1. Map chrome: đã ship C++ chung — desktop không cần port.
2. Commit SHORTERR detector + push MAPCHROME/SHORTERR.
3. Package Mac Bundle DMG (~25–32MB) RelWithDebInfo + local FLTK.
4. User: app “damaged” → fix plist/startup tên OldGameVM, rebuild DMG, ad-hoc sign staged app.
5. User confirm mở được; handoff này.
