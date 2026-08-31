# HANDOFF — 06/08/2026, Mac crash (TCC) + SM inventory radar chrome

Branch: `feature/multi-edition-detector`  
Repo: `Ethanhoangdesign/OldGameVM` → `origin`  
Local: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`  
Máy: macOS arm64  

Mac package handoff trước: `docs/HANDOFF-06-08-2026-mac-package.md`  
Map chrome: `docs/HANDOFF-06-08-2026-map-chrome.md`

---

## 0. TRẠNG THÁI

| Việc | Trạng thái |
|---|---|
| Phân tích Abort trap: 6 (launcher dialog) | **DONE** — intentional `std::abort` sau VFS fail |
| Root cause: TCC / Documents `game_dir` | **DONE** — log `Operation not permitted (os error 1)` |
| Import installer: chỉ chọn `.exe`, unpack `~/.ja2/imported/` | **DONE** |
| SM panel radar bay chrome (WF 1024 art) | **DONE** — user confirm “ngon rồi” |
| Rebuild `build-mac-release/OldGameVM.app` | **DONE** (local only, không commit) |
| Commit + push branch | **DONE** (session này) |
| Bundle static `innoextract` trong app | **Chưa** — vẫn brew / cạnh exe |
| Signed Developer ID / notarize | **Chưa** |

---

## 1. CRASH — Abort trap: 6

### 1.1 Hiện tượng

Launcher: *“JA2 Stracciatella crashed with error: Subprocess terminated by signal: Abort trap: 6”*.

Crash report:

```
Exception: EXC_CRASH (SIGABRT)
Thread 0: TerminationHandler() (SGP.cc:492) ← main (SGP.cc:449)
abort() called
```

### 1.2 Không phải

- Binary corrupt / bundle “damaged” (đã fix plist session trước `88920e135`)
- Map chrome / MAPFRAME
- Thiếu `Data/*.slf` (folder GOG đủ SLF)
- Bug abort ngẫu nhiên

### 1.3 Cơ chế

`main` catch exception → `TerminationHandler()` log `SLOGE` → `std::abort()` (non-Android). Launcher chỉ báo signal.

### 1.4 Log thật

`Logger_initialize("ja2.log")` → macOS temp, ví dụ:

`/var/folders/.../T/ja2.log`

Dòng lỗi (cùng giây crash):

```
Failed to build virtual file system (VFS):
Vfs_init_from_engine_options: Error initializing VFS for
".../Documents/GOG/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)/data":
Operation not permitted (os error 1)
```

Stack code:

1. `DefaultContentManager` ctor → `Vfs_init` fail → `runtime_error`  
   (`src/externalized/DefaultContentManager.cc`)
2. `loadGameData` không kịp chạy
3. `SGP.cc` catch → `TerminationHandler` → `abort()`

### 1.5 Root cause

`game_dir` nằm dưới **Documents**. App ad-hoc/unsigned thường bị macOS TCC chặn đọc → errno **EPERM**. Shell/`ls` vẫn đọc được; binary game thì không.

Config lúc crash: `~/.ja2/ja2.json` → Documents GOG path.  
Data an toàn đã có sẵn: `/Users/ethan/JA2/wildfire-gog-608` (smoke VFS OK sau khi trỏ lại).

### 1.6 Workarounds user (không cần code)

1. Trỏ `game_dir` ra ngoài Documents/Desktop (vd. `~/JA2/...`)
2. Hoặc Privacy & Security → Files and Folders / Full Disk Access cho OldGameVM
3. Dùng flow Import mới (mục 2) — unpack vào `~/.ja2/imported/`

---

## 2. IMPORT UX — chỉ chọn installer

### 2.1 Trước

1. Chọn `setup_*.exe`  
2. **Chọn folder đích** → user hay chọn Documents/Desktop → cùng TCC crash sau import  
3. Cần `innoextract` (brew / cạnh exe)

### 2.2 Sau (`OGVM-IMPHOME`)

1. Chọn `setup_*.exe` only (`.bin` cùng folder, tên gốc `…-1.bin`)  
2. Unpack **cố định**: `~/.ja2/imported/<installer-stem>/`  
3. Không hỏi folder đích  
4. Auto fill game directory + edition detect (logic cũ)

### 2.3 File

| File | Đổi |
|---|---|
| `src/launcher/Launcher.cc` | `importGameDataCb`: dest = home/`imported`/stem; message nhắc `.bin` |
| `src/launcher/StracciatellaLauncher.cc` | tooltip nút Import |

### 2.4 Còn thiếu cho “chỉ 2 file GOG, zero brew”

`innoextract` **chưa** bundle trong `OldGameVM.app` (boost dylib phức tạp).  
User Mac vẫn cần `brew install innoextract` hoặc đặt binary cạnh launcher.  
Backlog: ship static/bundled `innoextract` trong Resources + PATH trong startup.

---

## 3. SM PANEL — radar bay chrome (inventory merc)

### 3.1 Hiện tượng

Panel inventory single-merc (SM): góc phải radar/clock — nền chrome **mất** (tối), sau fix 1 còn **cột tối** giữa túi đồ và radar.

### 3.2 Art

Wildfire `interface/inventory_bottom_panel.sti` sub 0: **1024×140**

| Vùng art (x) | Nội dung |
|---|---|
| ~0–532 | Inventory merc (face, body, pockets) — khớp `SM_INVINTERFACE_WIDTH` |
| ~532–915 | Spacer / filler giữa (tối nếu cắt nhầm) |
| ~916–1023 | Radar + nút bay (chrome đúng) |

Vanilla path giả định art ~640; blit trái-only trên surface 674px → chỉ thấy x0–639, **mất bay phải**.

### 3.3 Fix (`OGVM-SMPANEL`)

`InitializeSMPanel()` trong `src/game/Tactical/Interface_Panels.cc`:

- Load art qua `CreateVideoSurfaceFromObjectFile` (crop source rect)
- Nếu `artWidth > smPanelWidth` (WF 1024):
  - **Left:** `0 .. SM_INVINTERFACE_WIDTH` (532) → full inventory + pocket column  
  - **Right:** `(artWidth - boxW) .. artWidth` với `boxW = smPanelWidth - 532` (= 108 @ 6-slot) → bay radar mép phải art  
- Else: path cũ (art ngắn hơn panel / vanilla 1:1)

Giống ý `InitializeTEAMPanel` crop `bottom_bar` buttons box.

### 3.4 Sai lầm phiên giữa (đã sửa)

Lấy left = `smPanelWidth - 142` và right = 142px cuối art:

- Cắt sớm inventory / dính spacer tối (screenshot user khoanh khe dọc)
- 142 = `TEAMPANEL_BUTTONSBOX_WIDTH` không phải bề rộng bay thật trên art 1024 SM

### 3.5 Smoke

Res 1366×768 (hoặc ≥1024), WF data, vào tactical → inventory merc:

- Radar có nền chrome  
- Không cột tối giữa pocket và radar  
- Nút ✓ / map + clock vẫn đúng hitbox (offsets SM_* không đổi; chỉ blit nền)

Build local:

```bash
cmake --build build-mac-release -j8 --target ja2
cp build-mac-release/ja2 build-mac-release/OldGameVM.app/Contents/Resources/ja2
codesign --force --deep --sign - build-mac-release/OldGameVM.app
open build-mac-release/OldGameVM.app
```

---

## 4. FILE ĐỤNG (session)

```
src/launcher/Launcher.cc                 OGVM-IMPHOME
src/launcher/StracciatellaLauncher.cc   tooltip import
src/game/Tactical/Interface_Panels.cc    OGVM-SMPANEL
docs/HANDOFF-06-08-2026-mac-crash-sm-panel.md  handoff này
```

**Không commit:** `build-mac-release/`, `layout controller/`, `ogvm-smoke*.png`, `~/.ja2/ja2.json` (user local).

---

## 5. BACKLOG

1. Bundle `innoextract` trong Mac app (zero brew).  
2. Developer ID + notarize.  
3. Optional: alert thân thiện khi VFS `EPERM` (đừng chỉ abort + logs tab) — gợi ý “move data out of Documents / use Import”.  
4. Optional: `resversion` auto WILDFIRE sau detect (user config còn `ENGLISH` + WF data vẫn chơi được nhưng không lý tưởng).  
5. CI Mac package verify SM panel path / import dest string.

---

## 6. LỆNH NHANH

```bash
# rebuild launcher + game
cd ".../ja2-stracciatella/build-mac-release"
cmake --build . -j8 --target ja2-launcher ja2
cp -f ja2 ja2-launcher OldGameVM.app/Contents/Resources/
codesign --force --deep --sign - OldGameVM.app

# log crash
tail -f /var/folders/*/T/ja2.log   # path exact in Logger banner

# push
git push origin feature/multi-edition-detector
```

---

## 7. SESSION TÓM TẮT

1. User crash Abort trap: 6 → đọc handoff Mac + crash report + `ja2.log` → VFS EPERM Documents.  
2. Fix import: dest `~/.ja2/imported/` — user chỉ chọn setup exe.  
3. Rebuild package; game chạy với `~/JA2/wildfire-gog-608`.  
4. User: nền radar inventory sai → art 1024 chỉ blit trái.  
5. Ghép left 532 + right bay mép art; user confirm OK.  
6. Handoff này + commit/push.
