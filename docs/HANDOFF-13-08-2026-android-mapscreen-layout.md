# HANDOFF — 13/08/2026

**Báo cáo Phân tích & Handoff: Bố cục Map Screen (Strategic Map) trên màn hình siêu rộng (Ultra-wide / 1664x768)**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | Map Screen layout & background rendering cho Wildfire (Full-size Map) |
| Files liên quan | `src/game/UILayout.cc`, `src/game/Strategic/Map_Screen_Interface_Border.cc`, `src/game/Strategic/Map_Screen_Interface_Bottom.cc`, `src/game/Strategic/MapScreen.cc`, `src/game/Strategic/Map_Screen_Interface_Map.cc` |

---

## 1. MỤC TIÊU (GOAL)

Khi chạy game ở độ phân giải ultra-wide (như `1664x768` trên Android):
1. **Bản đồ & UI không bị co cụm / hở gỗ quá nhiều:** Tối ưu hóa khoảng trống background (gỗ/wood filler) hoặc giãn rộng các thành phần UI (danh sách đệ nhị/roster, bảng nhật ký/history log, thanh công cụ) để lấp đầy khung hình `1664x768`.
2. **Sắp xếp nút bấm chuẩn xác:** 6 nút lọc (Town, Mine, Teams, Militia, Air, Items) và thanh chọn tầng (Level Selector) phải nằm đúng vị trí khay/ô chìm (recesses) trên ảnh nền, không bị chèn đè lên các khung thông tin khác.

---

## 2. BỐI CẢNH & HIỆN TRẠNG (CURRENT BEHAVIOR)

- Game sử dụng bộ art **JA2: Wildfire** (`b_map.sti` kích thước gốc `714x612`, `map_screen_bottom.sti` kích thước gốc `763x121`).
- Khi độ phân giải màn hình là `1664x768`:
  - `m_screenWidth = 1664`, `m_screenHeight = 768`.
  - Cột bên trái (Linh/Roster): Cố định chiều ngang `261px` (`X = 0..261`).
  - Khung bản đồ chiến thuật: Ảnh bản đồ cố định `714px` chiều ngang. Hàm `get_MAP_VIEW_START_X()` căn giữa bản đồ trong vùng bên phải `(1664 - 261 - 714) / 2 + 261 = 605px`.
  - Thanh UI bên dưới (`map_screen_bottom.sti` rộng `763px`): Hàm `get_MAP_BOTTOM_BASE_X()` ghim thanh này vào mép phải màn hình: `1664 - 763 = 901px` (`X = 901..1664`).
  - **Hệ quả:** Tạo ra các khoảng trống gỗ lớn (`DrawFillerOnSurface`) nằm xen kẽ giữa cột trái, bản đồ và panel dưới. 6 nút bấm lọc và Level Selector bị đẩy vào các tọa độ cố định đè lên thanh bottom panel hoặc nằm lơ lửng trên nền gỗ.

---

## 3. CÁC THAY ĐỔI ĐÃ THỬ NGHIỆM (WHAT WAS ATTEMPTED)

1. **Thử nghiệm 1: Thu gọn nút bấm & Level Selector vào thanh Bottom Panel**
   - **File:** `Map_Screen_Interface_Border.cc`
   - **Cách làm:** Định vị 6 nút lọc ở tọa độ `BASE_X + 10` đến `BASE_X + 260`, Y = `BASE_Y + 369` (nằm ở dải tối bên trái của `map_screen_bottom.sti`). Level Selector ghim ở `BASE_X + 500`.
   - **Kết quả:** Nút bấm bị nằm đè lên khung History Log / Trung tâm điều khiển của thanh bottom panel, không khớp với bố cục tổng thể.

2. **Thử nghiệm 2: Giãn khoảng cách các nút (Pitch 50px -> 59px)**
   - **File:** `Map_Screen_Interface_Border.cc`
   - **Cách làm:** Tăng khoảng cách pitch giữa các nút từ 50px lên 59px để khớp với 3 ô chìm (`x=10..60`, `x=69..120`, `x=127..178`) trong file ảnh `map_screen_bottom.sti`.
   - **Kết quả:** Các nút không còn dính liền nhau nhưng vị trí tổng thể vẫn sai lệch do thanh bottom panel bị lệch `901px` so với bản đồ.

---

## 4. PHÂN TÍCH NGUYÊN NHÂN CHƯA ĐẠT (ROOT CAUSE ANALYSIS)

Tại sao giao diện vẫn "y chang" hoặc bị lệch nghiêm trọng trên màn hình `1664x768`?

1. **Cơ chế neo tọa độ bị phân tán (Split Anchoring):**
   - Bản đồ (`b_map.sti`) được căn giữa vùng bên phải: `X = 605`.
   - Panel dưới (`map_screen_bottom.sti`) được ghim sang phải: `X = 901`.
   - Cột danh sách quân (Left Col) cố định ở mép trái: `X = 0`.
   - Việc 3 khối chính này neo theo 3 công thức khác nhau khiến cho khi màn hình kéo dài ra `1664px`, khoảng cách giữa chúng bị xé lẻ ra, nền gỗ tô tràn vào giữa, làm mất tính liên kết của giao diện.

2. **Kích thước Art cứng (Fixed Asset Dimensions):**
   - Engine JA2 Stracciatella thiết kế dựa trên pixel-perfect rendering cho ảnh 2D tĩnh (`714x612` và `763x121`).
   - Engine **không tự động stretch/scale** các tấm ảnh UI này theo chiều ngang màn hình mà chỉ render tỷ lệ 1:1, sau đó fill màu gỗ vào vùng thừa.

3. **Chưa có cơ chế Scale toàn bộ Map Screen (hoặc Stretch Viewport):**
   - Ở màn hình `1664x768`, biến `MAPZOOM_NUM` trong `UILayout.cc` bị khóa cứng = 2 (do macro `JA2_MAPZOOM_ALLOW_LARGE = 0`).
   - Do đó, bản đồ không được phóng to (zoom 1.5x), giữ nguyên `714x612px`, dẫn đến thừa rất nhiều diện tích trên màn hình `1664px`.

---

## 5. KIẾN TRÚC LAYOUT VÀ TỌA ĐỘ NÒNG CỐT (CODE ARCHITECTURE)

Dưới đây là các hàm quyết định vị trí UI trong Map Screen (`UILayout.cc`):

| Hàm | Công thức hiện tại (`isMapFullSize() == true`) | Ý nghĩa |
|---|---|---|
| `get_MAP_LEFT_COL_X()` | `0` | Vị trí X cột danh sách bên trái (rộng 261px) |
| `get_MAP_VIEW_START_X()` | `261 + (screenWidth - 261 - 714) / 2` | Tọa độ X bắt đầu vẽ bản đồ `b_map.sti` (714px) |
| `get_MAP_VIEW_START_Y()` | `(screenHeight - 121 - 612) / 2` | Tọa độ Y bắt đầu vẽ bản đồ (cao 612px) |
| `get_MAP_BOTTOM_BASE_X()`| `screenWidth - 763` | Tọa độ X của thanh UI bottom (`map_screen_bottom.sti`) |
| `get_MAP_BOTTOM_BASE_Y()`| `screenHeight - 480` | Tọa độ Y gốc của thanh UI bottom |

---

## 6. GỢI Ý HƯỚNG GIẢI QUYẾT CHO AI TIẾP THEO (RECOMMENDATIONS FOR NEXT AI)

Để giải quyết triệt để vấn đề giao diện trên `1664x768`, AI tiếp theo nên tiếp cận theo một trong hai hướng sau:

### Hướng A: Căn chỉnh liên kết nhất quán (Unified Alignment - Khuyên dùng nếu không scale)
1. **Neo Panel Dưới theo Bản Đồ (Anchor Bottom Panel to Map):**
   - Thay vì ghim Panel Dưới vào mép phải màn hình (`screenWidth - 763`), hãy neo tọa độ X của Panel Dưới trùng hoặc lệch chuẩn theo `get_MAP_VIEW_START_X()`.
   - Giúp thanh Bottom Panel và Bản đồ luôn di chuyển cùng nhau khi màn hình thay đổi độ rộng.
2. **Mở rộng Cột Left Column / History Log:**
   - Để Cột Left Column kéo dài từ `X = 0` đến điểm bắt đầu của Bản đồ (`get_MAP_VIEW_START_X()`).
   - Cho phép khung History Log tự động mở rộng chiều ngang (`MESSAGE_BOX_W = get_MAP_VIEW_START_X() - offset`) để lấp đầy khoảng trống bên trái mà không bị hở gỗ.
3. **Đặt nút bấm đúng vạch:**
   - Đặt 6 nút bấm lọc và Level Selector vào đúng dải thanh gỗ ngay dưới mép dưới của Bản đồ (`MAP_VIEW_START_Y + 612`), căn theo `MAP_VIEW_START_X`.

### Hướng B: Bật chế độ Map Zoom (Zoom 1.5x cho Màn hình Siêu Rộng)
1. Trong `src/game/UILayout.cc`, đổi `#define JA2_MAPZOOM_ALLOW_LARGE 1`.
2. Khi đó với chiều rộng `>= 1600` và chiều cao `>= 1000` (hoặc hạ ngưỡng xuống `>= 768`), bản đồ sẽ được scale 1.5x (`714 * 1.5 = 1071px`, `612 * 1.5 = 918px`).
3. Lưu ý: Cần kiểm tra kỹ các màn hình phụ khác (như Auto-Resolve / Pre-Battle) xem có bị lỗi hiển thị khi bật Zoom hay không.

---

## 7. FILE KIỂM TRA & LỆNH BUILD VERIFY

Biên dịch dự án trên macOS:
```sh
cd /Users/ethan/Documents/Ethan_repo/JA\ for\ all/ja2-stracciatella
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)
```

Chạy file script phân tích ảnh nền Wildfire (đã tạo sẵn trong `/tmp`):
```sh
python3 /tmp/analyze_panel.py /Users/ethan/JA2/wildfire-gog-608
```
