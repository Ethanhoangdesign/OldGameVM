# Handoff — Android strategic map red-frame centering

**Date:** 2026-08-14
**Branch:** `feature/multi-edition-detector`
**Target preset:** `934x480`

## Kết luận

Vùng khoanh đỏ trong screenshot là **map-screen chrome/background**: khung/nền gỗ quanh strategic map, không phải terrain map.

- Terrain map: `b_map.sti` hoặc vanilla `b_map.pcx`.
- Full-size Wildfire chrome gốc: `Map_Bord.sti` / `Mbs.sti`.
- Code hiện tại không blit nguyên `Map_Bord.sti`; dùng wood filler + khung vẽ tổng hợp trong `MapScreen.cc`.

## Code path

Terrain map blit tại:

- `src/game/Strategic/Map_Screen_Interface_Map.cc:1142-1169`

Khung/filler map-screen tại:

- `src/game/Strategic/MapScreen.cc:4997-5129`

Map origin dùng chung cho terrain, overlay, label, path, clipping, hit region:

- `src/game/UILayout.cc:287-315`
- `src/game/Strategic/Map_Screen_Interface_Map.h:134-137`

## Geometry hiện tại — 934x480

```text
map view Y       = 31
map view height  = 298
available height = 360
(360 - 298) / 2  = 31
```

`get_MAP_VIEW_START_Y()` hiện trả về `31` cho đúng preset. Đây là vị trí căn giữa map view trong vùng cao 360px phía trên bottom panel.

## Quyết định tạm thời

- Chưa sửa code.
- Không hạ map theo phỏng đoán.
- Nếu screenshot target xác nhận map phải thấp hơn, chỉ sửa nhánh `934x480` trong `UILayout::get_MAP_VIEW_START_Y()`:

```cpp
return 31 + delta;
```

Không sửa `DrawMap()` riêng; mọi thành phần bám origin dùng chung.

## Điểm cần xác minh khi quay lại

1. Lấy screenshot runtime mới sau Android rebuild.
2. Đối chiếu mép trong của khung đỏ với vùng map well thật.
3. Xác định khung đỏ là `Map_Bord.sti`/`Mbs.sti` hay filler tổng hợp.
4. Nếu lệch, đo `delta Y` theo pixel rồi chỉnh **chỉ** nhánh `934x480`.
5. Kiểm tra surface và underground map sau chỉnh.

## Verification hiện tại

Đã kiểm tra source và handoff liên quan. Chưa build/install APK trong phiên này.

Native unit-test/build trước đó bị block bởi môi trường; handoff cũ ghi expected origin `(500, 31)` cho X/Y map origin trong test layout hiện tại.

## Files cần nhớ

- `src/game/UILayout.cc`
- `src/game/UILayout_unittest.cc`
- `src/game/Strategic/MapScreen.cc`
- `src/game/Strategic/Map_Screen_Interface_Map.cc`
- `src/game/Strategic/Map_Screen_Interface_Border.cc`
- `docs/HANDOFF-06-08-2026-map-chrome.md`

**Trạng thái:** tạm dừng. Quay lại sau khi có screenshot target/runtime mới.
