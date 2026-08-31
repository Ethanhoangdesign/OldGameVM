# HANDOFF — 14/08/2026, Android strategic-map leather background + frame

Branch: `feature/multi-edition-detector`
Repo: `Ethanhoangdesign/OldGameVM`
Target: Android emulator, logical layout `934x480`
Data: Wildfire interface art

---

## Trạng thái

**DONE — user xác nhận runtime đạt và chốt “tạm vậy đi”.**

Strategic map Android hiện có:

- Pattern da nâu thay toàn bộ nền đen lộ ra quanh map.
- Map giữ origin `(500, 31)`, căn giữa trong vùng trên cao 360px.
- Khung nổi dùng texture thật từ `interface/mbs.sti`, không còn border line tổng hợp.
- Spacing, gờ sáng/tối và độ bất quy tắc gần artwork mẫu Wildfire.

Diff đã sẵn sàng commit/push.

---

## Nguyên nhân gốc

Android nhìn rộng nhưng engine dùng logical layout `934x480`:

- `g_ui.isWidescreenLayout()` = `true`
- `g_ui.isMapFullSize()` = `false`

Map được dời sang phải (`MAP_VIEW_START_X = 500`) nhưng nhánh này vẫn gọi `RenderMapBorder()` tại vị trí vanilla. Việc đó làm lộ backdrop đen cũ quanh map. Khung tổng hợp bằng các đường màu phẳng cũng quá đều, không giống molding có texture của ảnh mẫu.

---

## Fix cuối

File: `src/game/Strategic/MapScreen.cc`
Hàm: `RenderMapRegionBackground()`

### 1. Leather backdrop

Ngay sau `RenderMapBorder()` trong nhánh non-full-size, preset widescreen phủ toàn bộ vùng phải roster bằng `DrawFillerOnSurface()`:

```cpp
UINT16 const x = MAP_LEFT_COL_X + 261;
SGPBox const backdrop = {x, 0, (UINT16)(SCREEN_WIDTH - x), 359};
DrawFillerOnSurface(guiSAVEBUFFER, backdrop);
```

Thứ tự render:

1. `RenderMapBorder()` vẽ border cũ.
2. Leather filler phủ backdrop phải roster.
3. `DrawMap()` vẽ terrain và index rails lên trên.
4. Khung Mbs được vẽ cuối quanh map.
5. Restore region copy `guiSAVEBUFFER` sang `FRAME_BUFFER`.

### 2. Raised frame từ artwork thật

Riêng preset `934x480`, code load `interface/mbs.sti` bằng `CreateVideoSurfaceFromObjectFile()`.

Geometry nguồn đã đo:

- Artwork: `763x647`.
- Map well: origin `(49, 27)`.
- Well: `714x612`.

Code crop bốn dải top/bottom/left/right của Mbs, scale 50% bằng `BltStretchVideoSurface()`, rồi đặt quanh map half-size `357x306`. Chỉ bốn dải được blit; terrain và index rails không bị phủ.

Không thêm asset mới. Không thêm abstraction/config mới.

### 3. Full-size filler cùng diff

Nhánh full-size hiện còn các chỉnh sửa nền có sẵn trong working tree:

- Filler trái mở từ `x=261` thành `x=136`.
- Filler full-size vẽ vào cả `guiSAVEBUFFER` và `FRAME_BUFFER`.
- `RestoreExternBackgroundRect()` mở tương ứng từ `x=261` thành `x=136`.

---

## Nguồn texture

- Leather filler: `DrawFillerOnSurface()` trong `src/game/Tactical/Interface_Panels.cc`.
- Raised molding: `interface/mbs.sti` Wildfire.
- Terrain/index rails: `b_map.sti`, blit trong `DrawMap()`.

Handoff liên quan:

- `docs/HANDOFF-06-08-2026-map-chrome.md`
- `docs/HANDOFF-14-08-2026-android-map-red-frame-centering.md`

---

## Verification

User đã rebuild Android, mở save, vào Map Screen và gửi screenshot runtime sau mỗi vòng chỉnh. Screenshot cuối xác nhận:

- Không còn nền đen thô quanh map.
- Khung có spacing.
- Molding dùng texture gốc, gần ảnh mẫu hơn.
- Terrain, labels, index rails và highlight sector vẫn đúng vị trí.

Kiểm tra source:

```bash
git diff --check
cmake --build build --target ja2 -j8 2>&1 | grep -E "error:|warning:" | tail -30
```

Chạy Android:

```bash
android/gradlew -p android installDebug &&
adb shell am force-stop io.github.ja2stracciatella &&
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

---

## Nếu chỉnh tiếp

Chỉ tinh chỉnh các crop/placement trong block `SCREEN_WIDTH == 934 && SCREEN_HEIGHT == 480` ở `RenderMapRegionBackground()`.

- Muốn tăng spacing: tăng khoảng từ map origin đến `dstTop`/`dstLeft`.
- Muốn border dày hơn: tăng destination strip width/height.
- Nếu Mbs edition khác geometry: xác minh `frame->Width()`/`Height()` trước khi đổi hằng `edgeX=49`, `edgeY=27`.

Không sửa `UILayout::get_MAP_VIEW_START_X/Y()` riêng cho việc tinh chỉnh border; origin hiện đã khớp input, overlay và terrain.
