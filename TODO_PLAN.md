# Kế hoạch: căn đồng nhất strategic map giữa các tầng tại 934x480

## Context

Ở Android `934x480`, map mặt đất hiển thị theo layout half-size (`21x18` px mỗi ô), nhưng các tầng ngầm 1–3 chiếm gần gấp đôi vùng map, bị cắt mép; hang nhìn như đổi vị trí giữa tầng. Screenshot xác nhận đây không phải lỗi `MAP_VIEW_START_X/Y`: map mặt đất, lưới, cursor dùng chung origin `(500,31)`. Nguyên nhân là Wildfire cung cấp `mine_1/2/3.sti` kích thước thật `676x580`; nhánh non-full đang blit trực tiếp, thay vì thu 1/2 thành `338x290`. Nhãn `Sublevel` còn dùng `STD_SCREEN_X/Y`, nên không theo origin đặc biệt của `934x480`.

Kết quả cần đạt: surface và sublevel 1–3 cùng lưới `336x288`, cùng origin/cursor; art vanilla `338x290` không bị thu nhỏ lần nữa; full-size Wildfire không đổi.

## Cách sửa đề xuất

1. Sửa nhánh non-full trong `HandleLowerLevelMapBlit()` tại `src/game/Strategic/Map_Screen_Interface_Map.cc`.
   - Đọc kích thước thật của subimage 0 bằng `CreateVideoSurfaceFromObjectFile()`.
   - Nếu art là bản Wildfire lớn `676x580`, dùng `BltVideoSurfaceHalf()` tại anchor hiện có `MAP_VIEW_START + (21,17)`.
   - Nếu art đã là vanilla `338x290`, giữ `BltVideoObject()` hiện tại; tránh double-scale.
   - Giữ nguyên nhánh `isMapFullSize()`: bản `676x580` đã đúng tại anchor `(42,34)`.
   - Không thêm offset riêng cho từng tầng; cả ba tầng phải dùng cùng phép scale và anchor.

2. Neo nhãn tầng theo map view trong cùng file.
   - Thay nhánh non-full của `MAP_LEVEL_STRING_X/Y` từ `STD_SCREEN + (432,305)` thành `MAP_VIEW_START + (162,295)`.
   - Giá trị vanilla giữ nguyên tuyệt đối vì origin vanilla là `(270,10)`; `934x480` tự đi theo `(500,31)`.

3. Không sửa layout toàn cục.
   - Giữ `UILayout::get_MAP_VIEW_START_X/Y()` và test `(500,31)` hiện tại.
   - Không sửa `DrawMap()`, `GetScreenXYFromMapXY()`, `MapScreenRect`, clipping, cursor hoặc hit region: chúng đã dùng chung origin đúng.

## File trọng yếu

- `src/game/Strategic/Map_Screen_Interface_Map.cc` — `HandleLowerLevelMapBlit()`, `MAP_LEVEL_STRING_X/Y`; tái sử dụng `CreateVideoSurfaceFromObjectFile()` và `BltVideoSurfaceHalf()`.
- `src/sgp/VSurface.cc` — `BltVideoSurfaceHalf()` đã hỗ trợ nguồn 16-bit; chỉ tái sử dụng, không sửa.
- `src/game/UILayout.cc` và `src/game/UILayout_unittest.cc` — nguồn geometry `(500,31)`; không dự kiến sửa.

## Verification

1. Chạy kiểm tra layout hiện có:

```sh
cmake --build build --target unittest -j$(sysctl -n hw.logicalcpu) 2>&1 | grep -E "error:|warning:|FAILED|Passed" | tail -30
```

2. Build Android:

```sh
./tools/build-android-debug.sh
```

3. Cài/chạy APK tại preset `934x480`; load cùng save trong screenshot. Chụp bốn trạng thái: surface, sublevel 1, 2, 3.
   - Cả bốn có rail `1–16`, `A–P`, cursor vàng, grid đúng cùng tọa độ.
   - Art ngầm nằm trong khung `338x290`, không bị zoom 2x/cắt phải-dưới.
   - Hang mỗi tầng khác hình theo dữ liệu, nhưng sector cave khớp đúng ô lưới.
   - `Sublevel: N` nằm cùng vị trí tương đối trong map ở cả ba tầng.
   - Chạm cùng sector trước/sau đổi tầng chọn đúng ô.

4. Regression: kiểm tra `640x480` hoặc `800x600` với art vanilla, rồi `1024x768`/`1280x720` full-size Wildfire. Xác nhận vanilla không bị thu còn 1/4; full-size không đổi kích thước/anchor.

5. Chạy:

```sh
git diff --check -- src/game/Strategic/Map_Screen_Interface_Map.cc
```
