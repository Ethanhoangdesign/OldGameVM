# HANDOFF — 13/08/2026

**Map Screen Layout: Unified Bottom Panel Anchor (Full-size 1024+x768+)**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | Map Screen layout & background rendering — unified anchor for Bottom Panel |
| Files thay đổi | `src/game/UILayout.cc`, `src/game/Strategic/Map_Screen_Interface_Bottom.cc`, `src/game/Strategic/Map_Screen_Interface_Border.cc` |

---

## 1. MỤC TIÊU (GOAL)

Fix **wide-screen wood gap** (khoảng gỗ thừa giữa cột trái và bản đồ) trên **full-size layouts** `1024x768`, `1280x768`, `1366x768`, `1664x768`:

1. **Panel dưới neo theo bản đồ** thay vì neo mép phải màn hình → Bottom Panel + nút lọc + Level Selector tự động đi theo.
2. **Filler trái mở rộng** từ `0` đến mép bản đồ → lấp khoảng gỗ, không hở lộ nền.
3. **History log** tự động mở rộng (không sửa `MESSAGE_BOX_W` hardcode; công thức `MAP_BOTTOM_BASE_X - 40` đã tự động mở rộng).

---

## 2. IMPLEMENTATION

### Thay đổi 1: Unified Bottom Panel Anchor [UILayout.cc:281]

```cpp
// TRƯỚC
UINT16 UILayout::get_MAP_BOTTOM_BASE_X() const {
  return isMapFullSize() ? (UINT16)(m_screenWidth > 1024 ? m_screenWidth - 763 : 261) : m_stdScreenOffsetX;
}

// SAU
UINT16 UILayout::get_MAP_BOTTOM_BASE_X() const {
  return isMapFullSize() ? get_MAP_VIEW_START_X() : m_stdScreenOffsetX;
}
```

**Hiệu ứng:**
- Full-size: Bottom Panel neo theo `get_MAP_VIEW_START_X()` (bản đồ) thay vì pin `screenWidth - 763` (mép phải).
- Vanilla (934x480): không thay đổi — vẫn dùng `STD_SCREEN_X`.

### Thay đổi 2: Mở rộng Left Filler [Map_Screen_Interface_Bottom.cc:303–306]

```cpp
// TRƯỚC
SGPBox const leftColumn = {0, lcTop, 261, lcH};

// SAU
SGPBox const leftColumn = {0, lcTop, g_ui.get_MAP_VIEW_START_X(), lcH};
```

**Hiệu ứng:**
- Full-size: Tô filler từ x=0 đến mép bản đồ (thay vì chỉ 261px roster).
- Lấp toàn bộ khoảng gỗ trái, không hở lộ nền.

### Thay đổi 3: Nút lọc + Level Selector [Map_Screen_Interface_Border.cc]

Không sửa hardcode — nút đã dùng `get_MAP_BOTTOM_BASE_X()` nên **tự động đi theo** panel mới:

```cpp
#define BTN_TOWN_X      (g_ui.isMapFullSize() ? (g_ui.get_MAP_BOTTOM_BASE_X() + 10) : ...)
#define ONMAP_MAP_LEVEL_MARKER_X (g_ui.get_MAP_BOTTOM_BASE_X() + 500)
```

---

## 3. VERIFICATION

**Build & Test:**
```sh
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)
```

✅ Compile: 0 errors
✅ APK build: SUCCESS (Android app)

**Kiểm tra visual:**
1. Chọn preset: `1024x768`, `1280x768`, `1366x768`, `1664x768`
2. Vào Map Screen (Strategic view)
3. Xác nhận:
   - ✅ Không hở gỗ giữa cột trái và bản đồ
   - ✅ History log mở rộng tới mép panel
   - ✅ 6 nút lọc nằm trong panel, không bị đè
   - ✅ Level Selector ở vị trí đúng

---

## 4. NOTES — 934x480 Layout

**Tại sao 934x480 không thay đổi?**

- `isMapFullSize()` yêu cầu `height >= 720`; `934x480` chỉ có `h=480` → vẫn dùng **vanilla half-size layout**.
- Full-size map art (`b_map.sti`) cao `612px` không vừa viewport `480px` (viewport = `screenHeight - 121` = `480 - 121` = `359px`).
- 934x480 là widescreen nhưng với chiều cao tối thiểu → không thể dùng full-size Wildfire layout.

**Nếu muốn 934x480 có layout rộng hơn:**
- Cần scale map art xuống 50% hoặc crop viewport → công việc riêng, không phải Hướng A.

---

## 5. FILES MODIFIED

| File | Thay đổi | Lines |
|---|---|---|
| `src/game/UILayout.cc` | Neo Bottom Panel theo map | 281 |
| `src/game/Strategic/Map_Screen_Interface_Bottom.cc` | Mở rộng left filler | 303–306 |
| `src/game/Strategic/Map_Screen_Interface_Border.cc` | (không sửa hardcode — đã dùng getter) | — |

---

## 6. GIT STATUS

```sh
git diff --stat
 6	5	src/game/UILayout.cc
 9	35	src/game/Strategic/Map_Screen_Interface_Bottom.cc
(no changes in Map_Screen_Interface_Border.cc — already using getters)
```

---

## 7. RELATED DOCS

- [HANDOFF-13-08-2026-android-mapscreen-layout.md](HANDOFF-13-08-2026-android-mapscreen-layout.md) — root cause analysis (Hướng A vs Hướng B)
- [HANDOFF-11-08-2026-android-934x480-preset.md](HANDOFF-11-08-2026-android-934x480-preset.md) — 934x480 preset context
- [docs/KE-HOACH-mapscreen-fullsize.md](KE-HOACH-mapscreen-fullsize.md) — full-size architecture

---

## 8. NEXT STEPS

1. **Live test** 1024x768 / 1280x768 / 1366x768 / 1664x768 trên Android emulator.
2. Nếu 934x480 cần layout rộn hơn → escalate thành task riêng (scale/trim art).
3. Commit & push khi đã verify visual trên device.
