# JA2 Stracciatella — Kế hoạch: Map screen full-size 1024×768 (Wildfire)

*Soạn 26/07/2026, tiếp nối handoff Multi-Edition. Mọi số đo dưới đây lấy trực tiếp từ file trong `wildfire-gog-608/Data/*.slf`, không ước lượng.*

## 0. Trạng thái đầu vào (đã nghiệm thu 26/07)

Phiên hôm nay đã dứt điểm, đều đã build và test thực tế ở 1024×768:

1. **Bảng mã ký tự (2 tầng)** — `EncryptedString.cc` giải mã CP1252 (0x85/91–97) → Unicode; 4 bảng `translation-table-*.json` thêm `‘’""…–—±`. Hết chữ rác + tràn khung thoại.
2. **Vòng đỏ A9** — `Hilite.sti` Wildfire 26 frame 44×38 (cỡ lưới full-size); thêm `Blt8BPPDataTo16BPPBufferTransparentHalf()` vẽ thu nửa khi frame > ô lưới. *(Sẽ gỡ nhánh thu nửa khi làm full-size — xem M3.)*
3. **Mắt/miệng I.M.P.** — 4 chân dung Wildfire thay mới (200/205/210/211) có mắt (7,7)/miệng (10,23); `GetImpFacePosInfo()` chọn theo edition. Save cũ tự sửa khi load (`SaveLoadGame.cc:1393`).
4. **Panel tactical** — art `bottom_bar.sti` 1024px/10 ô/hộp nút 194px; `UILayout` + `InitializeTEAMPanel` ghép theo vùng art; radar +98, đồng hồ +109, tên sector +103; layout Wildfire chỉ bật khi màn ≥ 692px ngang.
5. **Thanh Player's Turn** — art ≥ bề ngang màn hình thì vẽ từ x=0.
6. **Nền menu chính** — art > 640×480 tự căn giữa màn (logo Wildfire có offset âm baked (-380,-140), khớp pixel ở 1024×768).
7. **Nền map screen** — lấp texture 4 dải quanh khối UI (sau 2 lệnh `Fill(NEAR_BLACK)` tại `MapScreen.cc`, thứ tự quan trọng).

**Độ phân giải chuẩn cho Wildfire: 1024×768** (STD_SCREEN = (192,144)). Toàn bộ art Wildfire được vẽ cho cỡ này.

## 1. Mục tiêu

Ở 1024×768 với data Wildfire: map screen vẽ bản đồ chiến lược **nguyên cỡ 714×612** (ô 42×36) thay vì thu nửa (ô 21×18), panel dàn theo layout Wildfire gốc. Data vanilla và độ phân giải nhỏ giữ nguyên hành vi hiện tại — điều kiện kích hoạt giống các bản vá trước: có `interface/b_map.sti`, không có `b_map.pcx`, và màn hình đủ chỗ.

## 2. Dữ kiện đã đo (từ art Wildfire)

| Art | Kích thước | Ý nghĩa |
| --- | --- | --- |
| `b_map.sti` | 714×612, 16-bit | Địa hình bắt đầu (41,35); lưới 16×16 ô, bước **42×36** (41+16×42=713) |
| `CharInfo.sti` | 261×106 | Panel info merc — cột trái rộng **261** |
| `Map_screen_bottom.sti` | **763**×121 | Thanh đáy: 1024−261=763 ✓ → đặt tại (261, 647) |
| `Sector_inventory.sti` | **763×647** | Vùng phải: 768−121=647 ✓ → inventory sector phủ đúng vùng map |
| `MilitiaMaps.sti` | 714×612, 12 sub 127×109 | Map thị trấn militia theo lưới lớn (3×42+1, 3×36+1) |
| `MILITIAMAPSECTOROUTLINE2.sti` | 44×38 | Khung ô militia cỡ lớn |
| `Hilite.sti` | 26 frame 44×38 | Vòng đỏ/vàng định vị — **đúng cỡ ô lớn, dùng thẳng** |
| `MapCursr.sti` | 88 frame: 0–43 nhỏ (~22×19), **44–87 lớn (~43×37)** | Bộ khung chọn/mũi tên đường đi có sẵn cả hai cỡ |
| `MAPBORDER.sti` | 380×360 | Viền cỡ vanilla — layout full-size **không dùng** |

Suy ra layout đích: **cột trái 261×768** (CharInfo + roster + MapInv), **vùng phải 763**: bản đồ 714×612 đặt tại ~(261+24, 17), **thanh đáy 763×121 tại (261, 647)**.

## 3. Hiện trạng code (đã kiểm)

- Hằng số lưới là macro tĩnh: `MAP_GRID_X/Y` (21/18), `MAP_VIEW_START_X/Y` (STD+270, STD+10), `MAP_VIEW_WIDTH/HEIGHT` (336/298), `DMAP_GRID_*` — tại `Map_Screen_Interface_Map.h:121–137`.
- **76 chỗ** dùng `MAP_GRID_*`, **50 chỗ** dùng `MAP_VIEW_START_*`, **15 lời gọi** `GetScreenXYFromMapXY()` — gói trong 6 file: `Map_Screen_Interface_Map.cc`, `MapScreen.cc`, `Map_Screen_Interface.cc`, `Map_Screen_Interface_TownMine_Info.cc`, `PreBattle_Interface.cc`, `Interface.cc`.
- Tọa độ thuận/nghịch: `GetScreenXYFromMapXY()` (Map_Screen_Interface_Map.cc:614) và `GetSectorAtXY()` (MapScreen.cc:3277).
- Vẽ map: `DrawMap()` (Map_Screen_Interface_Map.cc:496) → `BltVideoSurfaceHalf`. Bỏ half = `BltVideoSurface` thường (đường 16bpp→16bpp đã có sẵn).
- Shade ô chưa thăm (`ShadeMapElem`) cần palette 8-bit — với b_map 16-bit đã bị tắt từ commit `5ee53d152`. Full-size không đổi tình trạng này (ghi ở mục Rủi ro).

## 4. Các chặng (mỗi chặng build xanh + test được ngay)

**M1 — Tham số hóa lưới, không đổi hành vi.**
Biến `MAP_GRID_X/Y`, `MAP_VIEW_START_X/Y`, `MAP_VIEW_WIDTH/HEIGHT` từ macro tĩnh thành getter trên `g_ui` (hoặc struct `MapScreenLayout` mới), giá trị đúng như cũ. Sửa 6 file theo danh mục grep ở mục 3 — thao tác cơ học, diff lớn nhưng rủi ro thấp.
*Nghiệm thu: build xanh, chơi thử vanilla-mode như cũ, git diff chỉ đổi cách đọc hằng số.*

**M2 — Chế độ full-size: bản đồ + chọn ô.**
Khi đủ điều kiện (art WF + màn ≥ 1024×768): grid 42×36, view start = (285,17)+offset vùng phải, view 672×576; `DrawMap()` vẽ `b_map` nguyên cỡ; `GetSectorAtXY` theo bước mới; khung chọn trắng/vàng (`RenderMapHighlight`) và chữ số hàng/cột (`DrawMapIndexBigMap`) theo lưới mới; mouse region `gMapViewRegion` theo kích thước mới.
*Nghiệm thu: bản đồ to nguyên cỡ, rê chuột — khung trắng bám đúng ô, click chọn đúng sector.*

**M3 — Bộ icon cỡ lớn.**
Vòng đỏ/vàng: dùng thẳng `Hilite.sti` 44×38 (nhánh thu-nửa hạ xuống chỉ chạy ở chế độ half). Mũi tên đường đi + khung nhấp nháy: chuyển sang frame 44–87 của `MapCursr.sti` (cần lập bảng đối chiếu chỉ số frame nhỏ↔lớn trước, bằng cách decode như đã làm hôm nay). Bullseye, SAM icon, mine icon, icon lính di chuyển: đo từng cái, cái nào chỉ có cỡ nhỏ thì tạm giữ (vẽ giữa ô) và ghi chú.
*Nghiệm thu: đầu game A9 vòng đỏ đúng một ô; vẽ đường đi có mũi tên đúng cỡ.*

**M4 — Panel theo layout Wildfire.**
Cột trái 261 (CharInfo/roster/MapInv giữ tọa độ nội bộ, chỉ dời gốc); thanh đáy `Map_screen_bottom` 763×121 tại (261,647) + dời các nút/slider/đồng hồ theo; sector inventory dùng `Sector_inventory.sti` 763×647; militia popup dùng `MilitiaMaps.sti` + `OUTLINE2` 44×38.
*Nghiệm thu: đi một vòng đủ các popup: contract, assignment, militia, inventory sector, town/mine info.*

**M5 — Rà lưới còn sót.**
Grep lại từng usage `MAP_GRID`/`GetScreenXYFromMapXY` sau M2–M4: tên thị trấn, đường trực thăng, icon merc giữa 2 sector, PreBattle interface, `Interface.cc` (fast-help vị trí), tooltip. Sửa từng cái theo ảnh chụp thực tế.

**M6 — Hồi quy + tách PR.**
Chạy data vanilla ở 640×480 và 1024×768 đối chứng; cập nhật handoff; tách các phần dùng chung (blitter, resource variants, layout API) thành PR upstream riêng.

Ước lượng: M1+M2 một phiên; M3+M4 một phiên; M5+M6 một phiên.

## 5. Rủi ro & quyết định treo

- **Shade/tint 16-bit**: vùng chưa thăm và airspace tint cần cơ chế shade cho surface 16bpp (hiện tắt với Wildfire). Việc riêng, không chặn các chặng trên — có thể viết shade 16bpp (nhân màu trực tiếp) ở M5/M6.
- **Icon chỉ có cỡ nhỏ**: chấp nhận vẽ căn giữa ô lớn trong giai đoạn đầu.
- **KHÔNG làm**: full-size ở 800×600 (không đủ chỗ, Wildfire gốc cũng vậy); không đụng laptop screens; không sửa "cột 16" (EDGEOFWORLD — đã kết luận đúng thiết kế).

## 6. Lệnh quen dùng

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
cmake --build build --target ja2 -j8 && ./build/ja2
# Log: /var/folders/ql/jwcl902557q_2v8n5bgvmv380000gn/T/ja2.log
```
