# HANDOFF — 27/07/2026, buổi tối

Dự án: **OldGameVM** — bản mở rộng phi thương mại của ja2-stracciatella, chạy được nhiều bản Jagged Alliance 2 (Gold và Wildfire).

Kho: `https://github.com/Ethanhoangdesign/OldGameVM`
Nhánh làm việc: `feature/multi-edition-detector`
HEAD lúc viết: `e7ebd058e`

---

## 1. Việc làm xong hôm nay

### 1.1 Đổi tên dự án thành OldGameVM — xong, đã đẩy lên

| Commit | Nội dung |
|---|---|
| `f5efdda6` | Tạo `NOTICE.md`, chèn khối chú thích sửa đổi lên đầu 37 tệp `.cc`/`.h` (39 tệp, 228 thêm / 1 bớt) |
| `3bc12d3a0` | `src/game/Local.h` `APPLICATION_NAME` → `"OldGameVM"`; `CMakeLists.txt` 4 chỗ tên đóng gói + 1 chỗ lối tắt |

Đã đổi tên kho trên GitHub (Settings → General → Repository name → Rename), sửa phần About, và trỏ lại remote:

```
git remote set-url origin https://github.com/Ethanhoangdesign/OldGameVM.git
```

Giữ nguyên có chủ ý: `project(ja2-stracciatella)` trong CMake, biến `CONTACT`, tên đích dựng `ja2` / `ja2-launcher`, thư mục cấu hình `~/.ja2`, tên thư mục kho trên máy. Đổi sâu hơn thì phải để riêng một lượt.

Dòng "forked from ja2-stracciatella/ja2-stracciatella" trên GitHub **không xoá được**; muốn mất phải tách hẳn thành kho độc lập.

### 1.2 Huỷ kế hoạch gửi bản vá lên kho gốc

Quyết định của chủ dự án: **không gửi gì lên kho gốc**, giữ riêng trong kho của mình. Bảng phân nhánh 4 đợt trước đây **không dùng nữa**.

Remote `upstream` vẫn giữ, nhưng chỉ một chiều — để lấy bản sửa lỗi về, không đẩy gì lên:

```
upstream = https://github.com/ja2-stracciatella/ja2-stracciatella.git
upstream/master = 442b71a1a
```

### 1.3 Nghĩa vụ giấy phép — đã đọc và làm theo

Giấy phép duy nhất ở gốc kho: `SFI Source Code license agreement.txt` (45 dòng, Strategy First Inc., gọi tắt SFI-SCLA). Không có tệp `LICENSE` hay `COPYING` nào khác.

Những điều ràng buộc việc mình làm:

- Chỉ dùng cho **mục đích phi thương mại**. Được sửa và phát hành bản đã sửa, miễn phi thương mại.
- **Không được dịch ngược phần nhị phân** (điều 2).
- Phát hành phải kèm **nguyên văn** giấy phép, không cấp quyền rộng hơn.
- Tệp đã sửa phải mang ghi chú nổi bật: (i) đã thay đổi, (ii) ngày thay đổi. → `NOTICE.md` + khối chú thích đầu 37 tệp.
- Không xoá phần khai bản quyền có sẵn.
- Mọi quyền không cấp rõ đều giữ lại ⇒ **không có quyền dùng thương hiệu**.

Lập trường đã chốt: không đưa tranh vẽ hay dữ liệu thương mại vào kho mã; engine đọc từ thư mục game trên máy người dùng.

### 1.4 Chỉ số súng Wildfire — commit `e7ebd058e`

Tệp sửa: `assets/externalized/weapons.json` (229 thêm / 78 bớt, 66 mục → 69 mục).

**Sáu ô đặt lại** (hình đã đúng từ chặng trước, nhưng chỉ số vẫn là của khẩu cũ):

| Ô | Khẩu Wildfire | Trước | Sau |
|---|---|---|---|
| 1 | Calico M950 | súng ngắn, dmg 21, băng 15 | súng bắn loạt, dmg 27, băng 50 |
| 3 | H&K USP9 | dmg 22, tầm 120 | dmg 27, tầm 110 |
| 4 | Beretta 92F | súng bắn loạt | súng ngắn thường, dmg 22 |
| 55 | SVU | ROCKET_RIFLE, dmg 38, tầm 600 | bắn tỉa, dmg 47, tầm 420, băng 10 |
| 56 | H&K MP7 | súng ngắn AUTOMAG, băng 5 | tiểu liên, dmg 30, tầm 190, băng 20 |
| 65 | AS Val | AUTO_ROCKET_RIFLE, giá 10000 | súng trường, dmg 30, tầm 350, giá 3700 |

**Ba ô thêm mới** (trước đó hoàn toàn trống ở cả `weapons.json`, `items.json`, `magazines.json`):

| Ô | Khẩu | Sao từ | dmg | tầm | băng | giá |
|---|---|---|---|---|---|---|
| 66 | VSS Vintorez | 18 Dragunov | 46 | 200 | 10 | 3400 |
| 67 | V-94 | 19 M24 | 72 | 850 | 5 | 5500 |
| 68 | FN F2000 | 24 FAMAS | 36 | 600 | 30 | 3200 |

**Nguồn số liệu:** trang wiki Jagged Alliance của từng khẩu, mục "Jagged Aliance 2:WF". KHÔNG đào ra từ `WF6.exe` — giấy phép cấm dịch ngược.

**Hai hệ số quy đổi đã xác lập:**

- Tầm bắn trong mã = **tầm wiki × 10** (súng ngắn wiki 11–15 ↔ mã 120–135).
- Sát thương là **1:1**, không nhân (wiki JA2 Calico 22 ↔ mã súng ngắn 21–24).
- Cân nặng trong mã = **kg × 10** (Beretta 11 = 1,1 kg; M24 66 = 6,6 kg).

**Chỗ ước lượng, cần biết rõ:** những trường wiki không có — tốc độ bắn, thời gian rút súng, độ bền, độ dễ sửa, tiếng nổ, giá — đều lấy từ khẩu cùng lớp trong bản gốc rồi nhích theo. Chỉ số sẽ **không trùng khít** bản Wildfire gốc; đây là cách cân của OldGameVM.

**Một chỗ cố ý làm khác:** AS Val (65) và VSS (66) có `ubAttackVolume` = 45 thay vì 80, vì hai khẩu này gắn giảm âm liền nòng.

**Cách xử lý loại đạn:** game chỉ có 19 loại đạn, không có 4,6 mm / 9×39 / 12,7 mm. Thêm loại mới thì phải thêm băng đạn, mã hoá loại đạn, chỗ bán — dài. Nên ghép vào loại gần nhất đã có:

- MP7 → `AMMO57` (đạn 5,7 của P90)
- AS Val, VSS → `AMMO762W` (đạn 9×39 vốn cải từ vỏ này)
- V-94 → `AMMO762N`, bù lại bằng sát thương 72
- SVU → `AMMO762W`; F2000 → `AMMO556`; Calico, USP → `AMMO9`

**Không đổi `internalName`** của các ô cũ (vẫn `GLOCK_17`, `BERETTA_92F`, `ROCKET_RIFLE`…) vì mã C++ và trường `standardReplacement` của khẩu khác tham chiếu tới những tên này. Tên hiển thị lấy từ `itemdesc.edt` của bản Wildfire.

**Hệ quả bất ngờ nhưng tốt:** sau khi thêm ô 66/67/68, log đổi từ `WF-ITEMART: repointed 11` thành **`repointed 14`**. Bảng tranh Wildfire trong `DefaultContentManager.cc` tự gán đúng cả tranh lớn và biểu tượng nhỏ cho ba khẩu mới ⇒ **việc nắn tranh cho 66/67/68 coi như xong, không cần làm thêm**. Giá trị tạm trong `weapons.json` (mượn hình Dragunov, M24, FAMAS) chỉ còn tác dụng khi chạy JA2 Gold.

Nghiệm thu: engine nạp không một lỗi vật phẩm nào; game vào được bản đồ, laptop, Bobby Ray.

### 1.5 Bản vá độ phân giải khuyến nghị — commit `00b18cb76`

`src/launcher/Launcher.cc`: đưa `1366x768` lên đầu danh sách và ghi thêm " (recommended)". Lưu ý `setPredefinedResolution` dùng `sscanf("%dx%d")` ⇒ **số phải đứng đầu nhãn**.

---

## 2. Bẫy mới phát hiện hôm nay

**Lệnh dựng KHÔNG sao tệp JSON sang `build/externalized/`.** Sau khi `cmake --build` xong, `build/externalized/weapons.json` vẫn giữ dấu thời gian cũ và game vẫn nạp bản cũ. Mỗi lần sửa tệp trong `assets/externalized/` phải sao tay:

```
cp assets/externalized/*.json build/externalized/
```

**Bộ cài Wildfire không có tệp XML nào.** `find "$W" -iname "*.xml" | wc -l` = 0. Bản 6.08 này không dùng nền dữ liệu mở của 1.13; thông số nhúng trong `WF6.exe` (1025536 B). Wiki fandom mục Weapons chỉ có **danh sách tên**, không có bảng thông số — phải vào trang của từng khẩu mới có số.

**Tên trường JSON phải lấy từ danh sách thật, không đoán.** Đã in ra toàn `?` vì dùng `impact`/`range`/`magSize`. Tên đúng: `ubImpact`, `usRange`, `ubMagSize`, `ubShotsPerBurst`, `ubDeadliness`, `rateOfFire`.

**`.git/info/exclude` không có tác dụng với tệp đã theo dõi.** Muốn git thôi báo tệp đã commit thì dùng `git update-index --skip-worktree <tệp>`. Đã áp cho `.claude/settings.local.json`.

**Khi chủ dự án chọn một việc lạ, phải giải thích khái niệm TRƯỚC khi lập kế hoạch.** Đã dựng cả bảng phân nhánh gửi lên kho gốc rồi mới biết là không ai muốn dính tới kho gốc.

---

## 3. Quy tắc thường trực

Mọi hướng dẫn phải kèm hai lệnh dựng:

```
cmake --build build --target ja2-launcher -j8 2>&1 | tail -8
cmake --build build --target ja2 -j8 2>&1 | tail -8
```

- Sau mỗi lần dựng phải xác nhận thấy `[100%] Built target ja2`.
- Script gửi đi phải kèm khối `ls -la ~/Downloads/<tên>.py` và đòi kết quả chạy thật.
- Script phải tự kiểm đang ở gốc kho (`CMakeLists.txt` + `src/sgp`), tự lưu bản `.bak`, và **không ghi gì** nếu có phép kiểm nào trượt.
- Đếm mốc trong chính khối văn bản mình chèn trước khi viết `assert`; `assert` đứng TRƯỚC khi ghi tệp.
- Kiểm ô / itemIndex có tồn tại thật trước khi hứa con số.
- Parse JSON externalized phải bỏ dòng comment `//` trước.
- Không escape `{` `}` khi sinh mã C++.
- Chỉ đường trên GitHub phải nêu tên mục cụ thể (tab → mục → tên ô → tên nút).

---

## 4. Việc còn lại

### Ưu tiên gần

1. **Xoá tệp dự phòng** `assets/externalized/weapons.json.bak` nếu còn (đừng ghi vào kho).
2. **Sửa tay hai dòng mô tả lỗi thời** trong launcher: `StracciatellaLauncher.cc:121` ("map screen are always rendered at 640x480.") và `StracciatellaLauncher.fl:139`. Máy không có `fluid` nên phải sửa cả `.fl` và `.cc` sinh ra từ nó.
3. **Xác nhận `~/.ja2/ja2.json`**: `res` = `1366x768`, `fullscreen` false, `scaling` PERFECT.
4. **Nghiệm thu bảng mã sau khi chơi nhiều thoại**: `grep -c "Invalid character given" <log>`.
5. **Chơi sâu để gặp VSS / V-94 / F2000**, xem chỉ số thực chiến có vênh không. Riêng ô 1 giờ là Calico 50 viên nên lính mới thuê mạnh hơn trước — để ý xem đầu game có quá dễ.

### Việc dài hơi

6. **Import Game Data — chặng 2**: nguồn Steam (dò `drive_c`), tiến độ thật + tab Logs, chặn nút khi đang chạy, nhận diện mảnh `.bin` sai tên theo kích thước.
7. **Regression bản gốc 640×480** — dùng JA2 Gold (Wineskin) làm bản đối chứng.
8. **Backlog phần nhìn**: popup dân quân `MilitiaMaps.sti`; kho vật phẩm theo ô `Sector_inventory.sti`; vị trí popup giờ đến của trực thăng; dịch "Daily Expenses".
9. **Truy nguồn mã kích hoạt I.M.P.** — thư nói `CIA003`, engine nhận `XEP624`.
10. **Thử 1920×1080 toàn màn hình** (chưa đo màn hình bằng `system_profiler`).

### Dứt khoát KHÔNG làm

- Gửi bất cứ gì lên kho gốc.
- Dịch ngược `WF6.exe` để lấy thông số.
- Gắn giấy phép kiểu MIT/Apache, hay xoá phần khai bản quyền đầu tệp.
- Dùng cho mục đích thương mại.
- Đưa tranh vẽ / dữ liệu thương mại vào kho mã.
- Nhắm 1280×720 (chiều dọc tối thiểu là 733 px), hay 1920×1080 chế độ cửa sổ.
- Vá cột 16 của bản đồ.
- Hứa hỗ trợ đọc dữ liệu 1.13.

---

## 5. Số liệu tra nhanh

**Máy và đường dẫn**

```
Kho:      /Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella
Wildfire: /Users/ethan/Documents/kimi/setup_jagged_alliance_2_wildfire_6.08dlc_(67213)
JA2 Gold: /Applications/Jagged Alliance 2.app/.../drive_c/Program Files/GOG.com/Jagged Alliance 2/Data
Log:      /var/folders/ql/jwcl902557q_2v8n5bgvmv380000gn/T/ja2.log
Cấu hình: /Users/ethan/.ja2/ja2.json
```

**Dựng**

```
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLOCAL_FLTK_LIB=ON
```

**Chuỗi commit gần đây**

```
63dd76e2c  bảng mã CP1252
3bcc0cae6  bốn ô chọn tầng
b59fc8743  tranh vật phẩm Wildfire (repointed 11)
00b18cb76  launcher: 1366x768 khuyến nghị
f5efdda6   NOTICE.md + chú thích 37 tệp
3bc12d3a0  đổi tên thành OldGameVM (đã đẩy lên)
e7ebd058e  chỉ số súng Wildfire + VSS, V-94, F2000  ← HEAD
```

**Hình học màn hình bản đồ ở 1366×768**

```
b_map.sti 714×612, 16-bit, blit tại (START_X+1, START_Y+1), địa hình từ (41,35)
bước lưới 42 × 36 | bảng dưới cao 121 | tối thiểu dọc 733 px
map x 456..1170, y 0..612 | nền bảng x 261..1024
khối 6 nút x 1034..1184 | thanh tầng x 1192..1343, y 659..747
```

**Chuỗi vẽ**

```
MapScreenHandle() → BlitBackgroundToSaveBuffer() → RenderMapRegionBackground()
                  → RenderMapBorder() → DrawMap()
```

**Cấu trúc tệp dữ liệu**

```
SLF: mục 280 B (name[256] + offset u32 + length u32) ở CUỐI tệp; count u32 @512; header 532 B
STI: "STCI"; flags u32@16 (0x08 INDEXED / 0x20 ETRLE); h u16@20; w u16@22; nsub u16@28;
     palette 768 B@64; sub-entry 16 B; dữ liệu từ 64+768+nsub*16; trong suốt = chỉ số 0
EDT: UTF-16LE, ROT-1; itemdesc.edt = 351 bản ghi × 400 ký tự
     (shortName 0-79, name 80-159, description 160-399)
```

**Số mục các tệp dữ liệu ngoài**

```
weapons.json   69 mục (itemIndex 1..331)
items.json     126 mục (201..327)
magazines.json 45 mục (71..115)
armours.json   38 mục
explosives.json 30 mục
```

**Lỗi lành tính, bỏ qua được**

```
Music Play Error 4294967295 2/3/4/5/8
Failed to open 'intro/splashscreen.smk'
Missing interface/sirtechsplash.sti
[WARN] WorldDef.cc: Map is a ja2 wildfire map, expect problems
[WARN] Map_Information.cc: Map version is greater than the current version
AI CIVILIAN failed to get path for dialogue-related move!
```
