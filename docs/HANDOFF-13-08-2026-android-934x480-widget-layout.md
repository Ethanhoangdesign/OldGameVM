# HANDOFF — 13/08/2026 (SESSION 3)

**934x480 Bottom Panel — Widget Layout Mismatch (Background vs Widgets)**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | 934x480 widescreen: background đã đúng, widget vẫn phân bố theo vanilla — chưa hoàn thành |
| Status | Dừng để chờ ảnh mục tiêu chi tiết |

---

## 1. TÌNH TRẠNG HIỆN TẠI

Sau HANDOFF-13-08-2026-android-934x480-fullpanel-completed:

- `isWidescreenLayout()` (UILayout.cc) → bottom panel art `map_screen_bottom.sti` kéo full-width x=0..934.
- [Map_Screen_Interface_Bottom.cc:280-292](src/game/Strategic/Map_Screen_Interface_Bottom.cc#L280-L292) chỉ **vẽ background** rộng.
- **Widget vẫn dùng nhánh vanilla**: vì `isMapFullSize() == false` với 934x480.

Kết quả: background Wildfire rộng đúng, nhưng finance/radar/clock/log/nút vẫn nằm theo tọa độ vanilla → phân bố lệch, không khớp các ô lõm trong artwork.

---

## 2. ROOT CAUSE

Mọi widget key theo `isMapFullSize()` — nhị phân. `isWidescreenLayout()` chỉ được dùng cho filler/background trong `RenderMapScreenInterfaceBottom()`. Không có khái niệm "panel rộng nhưng map vanilla" cho widget.

Danh sách điểm dùng nhánh `!fs` (vanilla) vẫn sai cho 934x480:

| Thành phần | File | Dòng |
|---|---|---|
| Log frame (MESSAGE_BOX_X/Y/W/H) | Map_Screen_Interface_Bottom.cc | 72-75 |
| Scrollbar + nút scroll | Map_Screen_Interface_Bottom.cc | 82-86, 423-425 |
| Exit buttons (laptop/tactical/options) | Map_Screen_Interface_Bottom.cc | 414-416 |
| Time arrows + pause | Map_Screen_Interface_Bottom.cc | 419-420, 663-667, 697-699, 1053-1054 |
| Finance labels + figures | Map_Screen_Interface_Bottom.cc | 1009-1015, 1029-1036, 1098-1104 |
| Sector name | Map_Screen_Interface_Bottom.cc | 478-482 |
| Message text | Utils/Message.cc | 442-444 |
| Clock / radar | UILayout.cc | 81-84 |
| Filter buttons + level selector | Map_Screen_Interface_Border.cc | 37-62 |

Ví dụ toán học cho 934x480: `m_stdScreenOffsetX = (934-640)/2 = 147`.
- MAP_BOTTOM_BASE_X = 0 (widescreen) → background x=0..763.
- Nhưng finance dùng `MAP_BOTTOM_BASE_X + 359` = 359, trong khi art vẽ ô lõm finance tại panel (372,27)… → lệch.
- Message log lw = 390, đè ra ngoài khung.

---

## 3. MỤC TIÊU (TỪ ẢNH USER)

Background Wildfire giữ nguyên. Widget cần phân bố theo **panel rộng** khớp artwork:

- Log rộng, chiếm vùng trái đến trước finance.
- Finance/radar/clock nằm đúng ô lõm trong art.
- Nút exit/time/scroll đặt đúng recess.
- Filter buttons + level selector theo panel rộng.

---

## 4. GIẢI PHÁP ĐỀ XUẤT

Tách khái niệm "panel rộng":

```cpp
bool UILayout::isWidePanel() const { return isMapFullSize() || isWidescreenLayout(); }
```

Rồi thay `isMapFullSize()` → `isWidePanel()` tại các điểm widget đang dùng nhánh `!fs`:

1. **Map_Screen_Interface_Bottom.cc** — log frame/scroll/buttons/finance/sector name/time: dùng nhánh wide-panel.
2. **Message.cc** — lx/ly/lw/lh: theo panel rộng.
3. **UILayout.cc** — clock/radar: `get_MAP_BOTTOM_BASE_X()` đã trả 0 cho widescreen; cần kiểm tra `+554/+543` có khớp art rộng không.
4. **Map_Screen_Interface_Border.cc** — filter buttons/level selector: chuyển `isMapFullSize()` → `isWidePanel()`.

Lưu ý: các giá trị +X trong nhánh full-size (VD `+554`, `+668`, `+663`, `+372`) được đo trên art 763px; với 934x480 art nằm tại x=0..763 nên **có thể dùng lại chính xác** giá trị full-size. Không cần offset mới — chỉ cần chọn đúng nhánh.

---

## 5. RỦI RO / CẦN XÁC NHẬN

1. **Nút lọc (filter)**: nhánh full-size dùng `BTN_ROW_Y = BASE_Y + 369`. Với 934x480 `BASE_Y = m_stdScreenOffsetY = 0` (480 == 480) → y=369. Art panel rộng đặt nút ở đâu? Cần xác nhận.
2. **Clock/radar**: nhánh full-size `get_CLOCK_X = BASE_X + 668` = 668. Art 763px kết thúc tại 763, clock bay đo tại panel (668,27). Khớp?
3. **Level selector**: nhánh full-size `MAP_LEVEL_MARKER_X = BASE_X + 500` — cần verify trên 934x480.
4. **m_mapScreenHeight**: `currentHeight()` dùng `get_MAP_BOTTOM_BASE_Y() + m_mapScreenHeight`; với widescreen `m_mapScreenHeight` mặc định 480 → có thể cần khởi tạo lại.

---

## 6. NEXT STEPS

1. Nhận ảnh mục tiêu chi tiết (finance/radar/clock/log/nút cụ thể ở đâu).
2. Thêm `isWidePanel()` hoặc dùng trực tiếp nhánh full-size cho từng widget.
3. Build: `cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)` (lọc lỗi qua grep).
4. Chạy Android emulator 934x480, chụp so sánh target.
5. Regression: 1024x768, 800x600.
6. Cập nhật/ghi đè handoff này khi hoàn thành.

---

## 7. FILES MODIFIED (SESSION NÀY)

Chưa sửa code — mới đọc/so sánh. Handoff này là kết quả.
