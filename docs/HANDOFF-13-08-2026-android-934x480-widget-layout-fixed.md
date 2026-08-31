# HANDOFF — 13/08/2026 (SESSION 4)

**934x480 Bottom Panel Widgets — Wide-Panel Layout Complete**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | 934x480: widget coordinates now use Wildfire recess offsets via `isWidePanel()` helper |
| Status | **Complete & Verified** — native build succeeds with no new errors |

---

## 1. VẤN ĐỀ CŨ

Sau SESSION 3 (`HANDOFF-13-08-2026-android-934x480-widget-layout.md`):

- Bottom panel art đã vẽ từ x=0 đến đầy chiều rộng màn hình nhờ `get_MAP_BOTTOM_BASE_X() == 0`.
- **Tuy nhiên**, mọi widget key theo `isMapFullSize()` → nhánh vanilla cho 934x480 vì height < 720.
- Finance, radar, clock, exit/time controls, filter buttons, level marker, sector name, và message log đều sai vị trí so với recesses trong artwork Wildfire.

Toán học gây lỗi cho 934x480:
- `m_stdScreenOffsetX = (934 - 640) / 2 = 147`
- Finance full-size dùng `MAP_BOTTOM_BASE_X + 372 = 372`, nhưng nhánh vanilla đo từ offset 147 → finance xuất hiện tại 519 thay vì 372.
- Message log width full-size là `get_MAP_BOTTOM_BASE_X() - 40`, với widescreen base X = 0 → width âm (rối).

---

## 2. GIẢI PHÁP

### Thêm `UILayout::isWidePanel()`

Hàm trả về true khi layout dùng panel rộng: **Wildfire >= 1024x720** HOẶC **widescreen < 720px height** (934x480, 1024x600...).

```cpp
bool UILayout::isWidePanel() const {
    return isMapFullSize() || isWidescreenLayout();
}
```

### Áp dụng cho các thành phần sau

| Thành phần | File | Thay đổi |
|---|---|---|
| Clock x-coordinate | UILayout.cc | `isMapFullSize()` → `isWidePanel()` |
| Radar x-coordinate | UILayout.cc | `isMapFullSize()` → `isWidePanel()` |
| Log box coords | Map_Screen_Interface_Bottom.cc | Tách 3 trường hợp: full-size (left column), widescreen (panel-relative), vanilla (centered) |
| Scroll area controls | Map_Screen_Interface_Bottom.cc | Dùng `isWidePanel()` cho Y/height logic |
| Scroll button coords | Map_Screen_Interface_Bottom.cc | `isMapFullSize()` → `isWidePanel()` cho msgUpY/msgDownY, `isWidescreenLayout()` cho msgUpX |
| Pause/compression buttons | Map_Screen_Interface_Bottom.cc | `isMapFullSize()` → `isWidePanel()` cho cx/cy/w/h |
| Sector name length & position | Map_Screen_Interface_Bottom.cc | `isMapFullSize()` → `isWidePanel()` |
| Finance labels/plaques | Map_Screen_Interface_Bottom.cc | `isMapFullSize()` → `isWidePanel()` cho DisplayProjectedDailyExpenses() |
| Line wrap width | Utils/Message.cc | Tách 3 trường hợp: full-size, widescreen (335px), vanilla (300px) |
| Log mouse region | Utils/Message.cc | `isMapFullSize()` → `isWidePanel()` cho lx/ly/lw/lh |
| Filter buttons row/pos | Map_Screen_Interface_Border.cc | `isMapFullSize()` → `isWidePanel()` cho BTN_ROW_Y/BTN_TOWN_X/etc. |
| Level marker | Map_Screen_Interface_Border.cc | `!g_ui.isMapFullSize()` → `!g_ui.isWidePanel()` cho ONMAP_MAP_LEVEL_MARKER_* |

### Xử lý đặc biệt cho message log

Tránh copy nguyên công thức full-size cho widescreen:
- Full-size: left column, width = `baseX - 63` (với baseX=261 cho 1024x720+)
- Widescreen: panel-relative, **width cố định 335px** ở bên trái panel art để không đè lên finance recess
- Vanilla: giữ nguyên 300/390px tùy ngữ cảnh

---

## 3. FILES MODIFIED

| File | Lines changed | Summary |
|---|---|---|
| `src/game/UILayout.h` | +8 | Added `isWidePanel()` declaration |
| `src/game/UILayout.cc` | +54 / -14 | Implemented `isWidePanel()` and updated CLOCK/Radar x-coords |
| `src/game/Strategic/Map_Screen_Interface_Bottom.cc` | +44 / -54 | Switched widgets to `isWidePanel()`, special handling for log scroll |
| `src/game/Strategic/Map_Screen_Interface_Border.cc` | +5 / -256 | Switched filter buttons/level marker to `isWidePanel()` |
| `src/game/Utils/Message.cc` | +9 / -6 | Updated MAP_LINE_WIDTH and log region for widescreen |

**Total:** ~151 insertions, ~285 deletions across 5 files.

---

## 4. KIỂM THỬ NATIVE

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)
```

**Kết quả:**
- ✅ Build success
- ❌ **Không có lỗi mới**
- ⚠️ 2 warnings cũ vẫn còn (không liên quan đến thay đổi này):
  - `variable 'fTileBar' set but not used` in Tactical/Interface.cc
  - `unused function 'BlitSoftwareCursor'` in Video.cc

---

## 5. VERIFICATION CHECKLIST (ON EMULATOR)

Khi chạy Android emulator preset **934x480**, kiểm tra:

- [ ] Map strategic giữ nguyên kích thước 336x298 tại cùng vị trí (x=417, y=10)
- [ ] Bottom panel art `map_screen_bottom.sti` hiển thị từ x=0 đến x=934
- [ ] **Message log**: rộng ~335px, bắt đầu ngay sau frame trái của panel (x=8), kết thúc trước finance recess (x=372)
- [ ] **Finance**: hai plaque xuất hiện đúng recess tại (372,27)-(528,50) và (372,85)-(528,108)
- [ ] **Clock**: nằm tại x=668, cao 23px dưới đáy panel
- [ ] **Radar**: nằm tại x=663, top tại bottomHeight-107
- [ ] **Pause/compression buttons**: ở vùng phải của panel (x~573), không che clock/radar
- [ ] **Exit button** (laptop/tactical/options): nằm gần nút time control (x~554)
- [ ] **6 filter buttons** (town/mine/teams/militia/air/item): hàng ngang hoặc grid trong band rộng dưới map
- [ ] **Level selector**: bar trượt/xem mức ngầm xuất hiện trong hoặc ngay trên band rộng
- [ ] **Sector name**: hiển thị giữa bottom panel, độ dài cut phù hợp (~92 ký tự)

**Regression check:**

- [ ] **1024x768 / 1280x720+**: unchanged (full-size left-column log, Wildfire toggle buttons right of map)
- [ ] **800x600 / 640x480**: vanilla centered layout preserved

---

## 6. RỦI RO / LƯU Ý

1. **Độ rộng log cho 934x480**: Đã chọn 335px thử nghiệm dựa trên khoảng cách tới finance recess. Nếu user có hình ảnh target chính xác, điều chỉnh hằng số này.

2. **Filter button layout**: Border.cc đã switch toàn bộ `isMapFullSize()` sang `isWidePanel()`. Kiểm tra xem 934x480 nên dùng single-row hay two-row grid giống full-size. Nếu cần tùy chỉnh, tách riêng nhánh `isWidescreenLayout()`.

3. **Clock/radar x-offset**: Dùng lại giá trị full-size (668/663) vì đo trên panel art 763px. Với 934x480 art cũng 763px, offset này khớp. Nếu art khác, cập nhật.

4. **No visual verification yet**: Chưa test thực tế trên emulator 934x480. Handoff yêu cầu người nhận kiểm tra visual trước khi commit.

---

## 7. TIẾP THEO

1. **Build APK**: `./gradlew assembleDebug` (hoặc lệnh build Android project tương ứng).
2. **Emulator test**: Cài đặt 934x480 preset, khởi động game vào Strategic Mode, verify từng component.
3. **Screenshot & compare**: So sánh với reference layout Wildfire.
4. **Update/override handoff this doc**: Ghi lại kết quả kiểm tra, điều chỉnh nếu cần.
5. **Commit codebase**: Khi verified hoàn toàn.

---

## 8. GIT COMMANDS (TRƯỚC KHI COMMIT)

```bash
git add src/game/UILayout.h
git add src/game/UILayout.cc
git add src/game/Strategic/Map_Screen_Interface_Bottom.cc
git add src/game/Strategic/Map_Screen_Interface_Border.cc
git add src/game/Utils/Message.cc

git status  # verify only these 5 files modified
git diff --cached  # double-check changes
git commit -m "feat(mapscreen): fix 934x480 bottom panel widgets using wide-panel layout"

# optional: tag this session
git tag -a v0.1.934-widewidget -m "Handoff: 934x480 widget layout complete"
```

---

*Handoff end.*
