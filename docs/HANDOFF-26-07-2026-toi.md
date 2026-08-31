# JA2 Stracciatella — Multi-Edition: Handoff (26/07/2026, phiên tối)

*Tiếp nối handoff sáng 26/07. Phiên này nghiệm thu thực tế toàn bộ trên máy, data Wildfire GOG 6.08, độ phân giải 1024×768.*

## 1. Trạng thái: NHỮNG GÌ ĐÃ XONG VÀ CHẠY TỐT

**Độ phân giải chuẩn cho Wildfire là 1024×768** — toàn bộ art Wildfire vẽ cho cỡ này (logo menu có offset âm baked (-380,-140); timebar/bottom bar/overhead đều 1024px; loadscreen LS_* 1024×768 16-bit).

1. **Bảng mã ký tự (2 tầng)** — commit `1e27543d9`. `EncryptedString.cc` (SE_ENGLISH) chuyển CP1252 `0x85/91–97` → Unicode; 4 bảng `translation_tables/*.json` thêm `‘’""…–—±`. Hết chữ rác + tràn khung.
2. **Vòng đỏ A9** — commit `1e27543d9`. `Hilite.sti` Wildfire là 26 frame **44×38** (cỡ ô lưới lớn). Thêm `Blt8BPPDataTo16BPPBufferTransparentHalf()`; ở chế độ map thu-nửa thì vẽ nửa cỡ, ở map full-size dùng thẳng.
3. **Mắt/miệng I.M.P.** — `IMP_Confirm.cc`: Wildfire thay art 4 chân dung 200/205/210/211, mắt thật (7,7) miệng (10,23) (đo template-match từ STI). `GetImpFacePosInfo()` chọn theo edition; save cũ tự sửa khi load (`SaveLoadGame.cc:1393` gọi `ResetIMPCharactersEyesAndMouthOffsets`).
4. **UI tactical 1024** — `bottom_bar.sti` Wildfire = 1024px, 10 ô lính, hộp nút **194px** (vanilla 142). `UILayout::getTeamPanelButtonsBoxWidth()` + ghép panel theo vùng art; radar +98, đồng hồ +109, tên sector +103 (đều tính từ cuối dãy ô). Chỉ bật khi màn đủ rộng (≥692px).
5. **Thanh Player's Turn + menu chính** — art ≥ bề ngang màn thì vẽ từ x=0 / căn giữa màn. Menu 1024×768 phủ kín, logo đúng chỗ.
6. **MAP SCREEN FULL-SIZE (M1–M4 HOÀN THÀNH)** — xem `docs/KE-HOACH-mapscreen-fullsize.md`:
   - M1: `MAP_GRID_*`, `MAP_VIEW_*` thành giá trị runtime trên `g_ui` (macro giữ nguyên tên → 76 chỗ gọi không đổi).
   - M2: `isMapFullSize()` (Wildfire + ≥1024×768) → `DrawMap()` vẽ `b_map.sti` **714×612 nguyên cỡ**, ô 42×36, view start (285,17), ô (1,1) tại START+GRID (terrain trong art bắt đầu (41,35)). Tint airspace 8-bit bị chặn (16-bit không có palette); tô tối vùng chưa thăm dùng `ShadowRect` — chạy tốt.
   - M3: locator/hilite full-size tự khớp (frame 44×38 ≤ ô 44).
   - M4 (layout responsive theo yêu cầu Ethan):
     * **Cột trái 0..261**: CharInfo/roster/inventory (origin `get_MAP_LEFT_COL_X/Y()` = (0,0)); ô đồ inventory (`m_invSlotPositionMap`) và menu assignment/contract đi theo. Tọa độ ô đồ ĐÃ ĐỐI CHIẾU với art `MapInv.sti` Wildfire — khớp pixel.
     * **History log**: (đã đổi cuối phiên — xem mục 5) từng ở đáy cột trái top y=362 (roster và panel inventory đều kết thúc 359). Wrap chữ 222px, ~35 dòng, nút cuộn x=239, thanh trượt động. `MapScreenLogTop()` trong `Map_Screen_Interface_Bottom.cc`.
     * **Thanh đáy**: art `map_screen_bottom.sti` (763px) tại (261,647); khung log cũ trong art bị phủ filler; 6 nút toggle + marker tầng chuyển vào dải đó (x 270..533, y 670/655). Compression/balance/laptop/mail/minimap/clock giữ offset gốc (họ nằm ≥390 nên tự đúng).
     * Origin theo vùng: `get_MAP_BOTTOM_BASE_X/Y()` = (261,288) full-size / STD vanilla. Anchor trong **header** cũng đã rebase: `MapScreen.h` (NAME_X..TIME_REMAINING, CONTRACT, CLOCK_ETA, TRASH_CAN), `Map_Screen_Interface.h` (Y_START, KEYRING, BAR_INFO, CHAR_ICON).
   - **Vanilla không đổi pixel nào** — mọi nhánh else giữ nguyên giá trị cũ.

## 2. Việc đang mở (ưu tiên từ trên xuống)

1. **Bảng item Wildfire lệch tên/hình** — MỚI PHÁT HIỆN: item tooltip "Beretta 92F (9mm)" nhưng icon là súng giảm thanh. Wildfire đánh số lại kho item; engine đang đọc `items.json` externalized (vanilla). Cùng họ với mã I.M.P. `CIA003` (mục 4.3 handoff sáng). Hướng: dò bảng item trong `BinaryData.slf` Wildfire, đối chiếu, externalize theo edition. **Cách đọc .slf/STI bằng Python đã có sẵn trong lịch sử phiên này** (header 532B, entry 280B ở cuối file; STI: nsub tại offset 28, sub-entry 16B: offset/len/ox/oy/h/w).
2. **Mã kích hoạt I.M.P. `CIA003`** (4.3 cũ) — chưa đụng.
3. **Nghiệm thu tương tác map screen** — militia popup (nghi lệch: `MilitiaMaps.sti` Wildfire 714×612, 12 town map 127×109 theo ô lớn — art có sẵn, cần nối vào), sector inventory (`Sector_inventory.sti` 763×647), contract/assignment menu, ETA popup trực thăng (đang nổi giữa map), turn-time-limit groove trên timebar.
4. ~~Panel inventory tactical~~ — ĐÃ XONG cuối phiên (fix bus error, xem mục 5).
5. **M5–M6 kế hoạch map screen**: rà nốt usage lưới (PreBattle, trực thăng, icon giữa 2 sector...), hồi quy vanilla 640×480, tách PR upstream.
6. **1600×800 / đa độ phân giải** — Ethan muốn hướng responsive; nền tảng đã có (mọi origin theo vùng qua `g_ui`), cần quyết nghịch: map giữ 714 căn giữa hay phóng to hơn.
7. Nhỏ: khung log đang là viền vẽ đơn giản (có thể ốp art); 2 lần fill khi restore có thể tối ưu.

## 3. Nguyên tắc đã kiểm chứng thêm trong phiên này

- Toàn bộ ước lượng vị trí PHẢI đo từ art (đã hố vài lần vì đoán từ screenshot bị scale).
- Macro đổi từ hằng sang runtime ⇒ để ý **narrowing trong brace-init** (`(UINT16)` cast) và **`std::max` lệch kiểu** (`std::max<INT32>`).
- Anchor nằm cả trong **.h** — sed .cc là chưa đủ, grep cả header.
- `ETRLEObject` định nghĩa trong `HImage.h` (Types.h chỉ forward-declare); `Get16BPPColor` trong `HImage.h`; `RectangleDraw` trong `Line.h`.
- Vùng nào không được RestoreExternBackgroundRect phủ sẽ giữ hình cũ vĩnh viễn — mở rộng restore khi mở rộng vùng vẽ.
- prof.dat giải mã được bằng Python (bảng ROT 46 byte trong `Tactical_Save.cc:982`, record 716B, eyes tại offset 262).

## 4. Build & chạy

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
cmake --build build --target ja2 -j8 && ./build/ja2
# Launcher: ./build/ja2-launcher  (đặt 1024x768)
# Log: /var/folders/ql/jwcl902557q_2v8n5bgvmv380000gn/T/ja2.log
```

Lưu ý: 4 file JSON translation-table phải đồng bộ giữa `assets/externalized/` và `build/externalized/` (copy khi đổi, không cần rebuild).

## 5. Bổ sung cuối phiên (23h00–23h45) — layout thanh đáy hoàn thiện + fix crash

Theo layout mẫu Ethan chọn (log ngang, các cụm có ngăn riêng):

- **Dải đáy 4 ngăn khung nổi**: log (x 4–600, 9 dòng, hộp lõm bevel) | cột tài chính (604–714: Current Balance / Daily Income / **Daily Expenses** — 3 hộp lõm có nhãn, Expenses đỏ) | cụm nút (giữ vị trí) | sector (khung ảnh minimap + tên + khung ngày giờ). Ở full-size KHÔNG blit art `map_screen_bottom.sti` nữa — tự vẽ toàn bộ ngăn trên filler.
- **`GetProjectedTotalDailyExpenses()`** (Bottom.cc): tổng `sSalary` của lính đang thuê còn sống — chỉ số mới, nhãn "Daily Expenses" tạm hardcode tiếng Anh (chưa localize).
- **Hàng toggle + marker tầng** nổi trên panel riêng (x 692–1024, y 592–648), vẽ cùng lượt với map trong `RenderMapRegionBackground` để map redraw không xóa.
- **Bảng roster kéo dài chạm dải đáy** (y 107–645): `newgoldpiece3.sti` blit 3 khúc (header 40px + thân giãn 190→474 + mép 24px) qua `BltStretchVideoSurface`. LƯU Ý: hiển thị vẫn tối đa 20 nhân vật (`MAX_CHARACTER_COUNT` — trần cứng dính save format; vượt 20 là hạng mục lớn riêng).
- **`CreateVideoSurfaceFromObjectFile` export** từ Interface_Panels (bỏ static, khai báo trong .h).
- **FIX CRASH (bus error)**: `InitializeSMPanel()` — panel inventory tactical. Code vanilla blit bản canh-phải giả định art 640px; art Wildfire `inventory_bottom_panel.sti` rộng 1024 → blit không clip tràn bộ đệm. Fix: đo bề rộng art thật qua `SubregionProperties(0).usWidth` + bọc blit trong `SetClippingRect` theo kích thước panel. Crash report đọc từ `~/Library/Logs/DiagnosticReports/ja2-*.ips` (cách chẩn crash nhanh — nhớ dùng lại).
- Nghiệm thu cuối: tactical mở inventory OK, map screen đầy đủ ngăn — Ethan xác nhận "ok hết rồi".

Ghi chú kỹ thuật phát sinh: `SetClippingRect` trả clip cũ (VObject_Blitters.h); mở panel/khung mới nhớ mở rộng vùng `RestoreExternBackgroundRect` tương ứng; file `_crash.ips` ở gốc repo là tạm — xóa được.
