# HANDOFF — 27/07/2026 (sáng)

Tiếp nối `HANDOFF-26-07-2026-toi.md`. Branch: `feature/multi-edition-detector`.
Build: `cmake --build build --target ja2 -j8` (macOS Apple Silicon).
Data Wildfire GOG 6.08: `/Users/ethan/JA2/wildfire-gog-608`.

## Đã làm trong phiên này

### 1. Sửa map ngầm (sublevel 1–3) bị lệch trong chế độ full-size
Triệu chứng: chuyển level bằng thanh chọn level thì lưới hầm mỏ lệch khỏi ô,
sót viền map mặt đất quanh mép, chữ "Sublevel" rơi giữa bản đồ.

Nguyên nhân & fix (`Map_Screen_Interface_Map.cc`):
- Wildfire có sẵn art hầm **bản to**: `Mine_1/2/3.sti` header ghi 338×290 nhưng
  subimage 0 thật là **676×580** (đúng gấp đôi bản gốc). Code cũ vẫn blit ở neo
  half-scale (+21,+17). `HandleLowerLevelMapBlit()` giờ có nhánh full-size:
  blit ở **(MAP_VIEW_START_X + 42, MAP_VIEW_START_Y + 34)** (neo gốc × 2).
- Art hầm (676×580) nhỏ hơn art mặt đất (714×612) → trước khi blit, phủ toàn
  vùng map bằng màu nền mỏ `FROMRGB(2,2,0)` (kích thước lấy từ
  `guiBIGMAP->Width()/Height()`) để không sót pixel map mặt đất cũ.
- `MAP_LEVEL_STRING_X/Y` có nhánh full-size: `(MAP_VIEW_START_X + 324,
  MAP_VIEW_START_Y + 590)` — gấp đôi offset gốc so với gốc view.

### 2. Bố trí lại dải nút toggle + thanh chọn level (theo art gốc WF)
Phát hiện quan trọng: **`Map_Bord.sti` của WF có subimage 1024×648** — chính là
khung viền map screen full-size WF thiết kế sẵn: khung mảnh quanh mép màn hình
+ thanh gỗ ngang **y609–648**. Tức layout chuẩn WF là map nằm SÁT ĐỈNH màn
hình, thanh nút nằm DƯỚI đáy map (nút chờm nhẹ lên mép map vài px).
Đã đo thêm: nút toggle WF (`Map_border_buttons.sti` frames 1–5, 8 + down-frame
= idx+9) là **50×44** (không phải 43px như bản gốc); marker chọn level
(`GreenArr.sti`) là khung trắng **151×23**.

Thay đổi:
- `UILayout.cc`: `get_MAP_VIEW_START_Y()` full-size **17 → 0**. Đáy lưới ô
  (hàng P) rơi đúng y612, art map chiếm y1–613. Mọi thứ (nhãn A–P, số cột,
  icon lính, locator, shade…) ăn theo getter nên tự dời.
- `MapScreen.cc`: backdrop dải gỗ `{262, 613, W-262, 35}` + bevel 2 lớp —
  chạy suốt bề ngang cột phải, không còn che ô map nào.
- `Map_Screen_Interface_Border.cc`:
  - `BTN_ROW_Y` 601; các nút cách nhau **64px** (50 + 14 hở): TOWN 467,
    MINE 531, TEAMS 595, MILITIA 659, AIR 723, ITEM 787.
  - Mỗi nút có khung lõm riêng (vẽ trong `RenderMapLevelSelectorFullSize()`,
    mảng `btnX[]` cần cast `(INT16)` — narrowing).
  - `MAP_LEVEL_MARKER_X/Y` = (861, 601), `MAP_LEVEL_MARKER_WIDTH` full-size
    **151** (khớp GreenArr); selector kết thúc x1012, không tràn mép.

### 3. Ghi chú kỹ thuật phiên này
- VM Cowork trên máy KHÔNG có cmake và không build được binary macOS → luôn
  đưa lệnh build cho người dùng chạy. `nohup` không sống qua từng lệnh
  device_bash (mỗi lệnh là sandbox bwrap riêng).
- `pgrep -f "cmake --build"` tự khớp chính shell bao ngoài → dùng
  `pgrep -f "cmake --buil[d]"`.
- Cách soi art trong SLF nhanh: entries 280B ở cuối file (name[256] + offset
  u32 + length u32), count u32 ở offset 512; STI: "STCI", h/w u16 tại 20,
  flags tại 16, nsub u16 tại 28, palette 768B tại 64, sub-entry 16B
  (dataOffset, dataLength, ox i16, oy i16, h u16, w u16). Header w/h có thể là
  số cũ 640×480 — **kích thước thật nằm ở subimage** (vd Map_Bord, Mine_N).

## Trạng thái
- Mặt đất + ngầm 1–3 đều thẳng hàng, không sót viền khi chuyển level.
- Dải nút rộng rãi, có ngăn riêng, selector 151px nằm gọn góc phải, không che map.
- User xác nhận "cũng được" trên screenshot 07:32 27/07.

## Backlog (giữ nguyên từ handoff trước)
1. Bảng tên/icon item Wildfire lệch ("Beretta" sai icon/tên).
2. IMP activation code CIA003.
3. Militia popup (`MilitiaMaps.sti` 714×612, 12 sub 127×109) chưa adapt full-size.
4. Sector inventory (`Sector_inventory.sti` 763×647) chưa adapt.
5. Vị trí popup ETA trực thăng.
6. M5–M6: quét lại toàn bộ chỗ dùng grid + regression vanilla 640×480, tách PR
   gửi upstream.
7. Đa phân giải 1600×800 (layout đã tham số hóa theo hướng responsive).
8. Localize nhãn "Daily Expenses".
9. Cân nhắc dùng hẳn `Map_Bord.sti` (blit cả khung 1024×648 thay vì tự vẽ
   backdrop/bevel) — cần xử lý phần khung trái đè lên cột roster x0–261.
