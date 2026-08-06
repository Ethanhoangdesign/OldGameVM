# HANDOFF — 06/08/2026, Map Overview chrome (leather + frame)

Branch: `feature/multi-edition-detector`  
Repo: `Ethanhoangdesign/OldGameVM`  
Local: `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella`  
Build: `cmake --build build --target ja2 -j8` (macOS arm64)  
Data: Wildfire GOG 6.08 (full-size map via `b_map.sti`)

Android handoff (khác session): `OGVM_ANDROID.md`.

---

## 0. Mục tiêu session

Làm **Map Overview / strategic map** full-size (Wildfire 1024+) trông giống game gốc:

1. Nền da thuộc nâu ấm (không xám lạnh / không vân gỗ đỏ to).
2. Khung viền 4 cạnh (molding lõm) quanh map, rail `1–16` / `A–P` tối như mẫu.

User xác nhận: **màu nền OK**, **khung gần giống** — đã đẩy molding dày + darken rail.

---

## 1. Nguyên nhân gốc

### 1.1 Nền lạnh / đỏ

Full-size **không** blit `mbs.sti` (vanilla border). Margin vẽ bằng `DrawFillerOnSurface()`:

| Art | Crop filler RGB (gần đúng) | Cảm giác |
|---|---|---|
| Vanilla `OverheadInterface.sti` | ~(27, 14, 10) | Da thuộc ấm |
| Wildfire `OverheadInterface.sti` | ~(26, 25, 22) | Xám lạnh |

WF art lạnh. Stretch crop 560×112 full màn → vân gỗ **to + đỏ mahogany** (screenshot session).

### 1.2 Khung thiếu

`RenderMapBorder()` (blit `mbs.sti`) **bị skip** khi `g_ui.isMapFullSize()`:

```cc
// MapScreen.cc — fMapPanelDirty path
else if (!g_ui.isMapFullSize())
{
    RenderMapBorder();
}
```

`b_map.sti` (714×612) chỉ có chrome rail **trên + trái**. Full-size thiếu molding ngoài + rail dưới/phải.

### 1.3 Art tham chiếu (trong `Data/InterFace.slf` WF)

| File | Size | Vai trò |
|---|---|---|
| `b_map.sti` | 714×612 (terrain @ 41,35) | Map art + rail 1–16 / A–P |
| `Mbs.sti` | 763×647 | Border full WF (top rail ~53px, left ~57px) — **không** blit full-size |
| `Map_Bord.sti` | 1024×648 | Overview / wood bar y609–648 |
| `OverheadInterface.sti` | 1024×160 (WF) | Filler texture (cold) |

---

## 2. Diff session (chưa commit)

### 2.1 `src/game/Tactical/Interface_Panels.cc` — `DrawFillerOnSurface`

**LEATHER-TINT + TILE:**

1. Load `interface/overheadinterface.sti` sub 0.
2. Tint pixel 16bpp (bỏ near-black `< 18`):
   - `r = min(255, r*110/100 + 6)` rồi `*70/100`
   - `g = min(255, g*55/100 + 2)` rồi `*70/100`
   - `b = min(255, b*35/100 + 1)` rồi `*70/100`
3. **Tile 1:1** crop `{80, 42, ≤560, ≤112}` qua `BltVideoSurface` — **không** `BltStretchVideoSurface` (tránh grain khổng lồ).

Dùng chung: map margins, bottom band, tactical letterbox.

### 2.2 `src/game/Strategic/MapScreen.cc` — full-size `MAPFRAME`

Chạy sau wood filler, trước `RenderMapLevelSelectorFullSize` / `RestoreExternBackgroundRect`.

**Well** = blit `b_map` (`MAP_VIEW_START + 1`, size `714×612 * MAPZOOM_NUM/2`).

**Outer molding ~14px** (4 vòng, 4 cạnh):

| Vòng | RGB gần đúng | Vai trò |
|---|---|---|
| ink | (3,2,1) | Crack ngoài |
| deep | (10,6,3) | Shadow |
| body | (22,13,7) | Thân da |
| mid | (36,22,11) | Mid fill |
| hi / warm | (78,52,26) / (58,38,18) | Bevel sáng trên-trái |
| deep / ink | | Bevel tối dưới-phải |

**Index rails:**

- Top/left: **darken** buffer ×42% + bias warm black (giữ glyph `b_map`).
- Bottom/right: fill `deep` + darken (b_map không có chrome 2 cạnh này).
- Rail thickness: top `35 * MAPZOOM/2`, left `41 * MAPZOOM/2` (neo art terrain).

**Không** đè solid mid-brown lên số/chữ (bug phiên trước).

---

## 3. File đụng

```
src/game/Tactical/Interface_Panels.cc   DrawFillerOnSurface — tint + tile
src/game/Strategic/MapScreen.cc         MAPFRAME full-size block
```

Chưa commit. Cùng branch còn dirty khác (edition detector) — **đừng** gộp bừa.

---

## 4. Smoke

```bash
cmake --build build --target ja2 -j8
# Chạy WF, res >= 1024x768, vào map screen
```

**OK khi:**

- Nền margin nâu socola tối, vân **mịn** (tile), không xám / không vân đỏ to.
- Khung 4 cạnh rõ, lõm.
- Rail `1–16` / `A–P` tối; số/chữ vẫn đọc được.
- Bottom + right có dải tối cân với top/left.

**User status (cuối session):** “ok rồi ấy” — nền + khung đạt mức chấp nhận; tinh chỉnh nhỏ vẫn mở.

---

## 5. Chỉnh nhanh (nếu user kêu)

| Muốn | Chỗ |
|---|---|
| Nền đậm/nhạt | `Interface_Panels.cc` hệ số `110/55/35` và darken `70` |
| Khung dày/mỏng | `MapScreen.cc` offset molding `14/13/9/4/2` |
| Rail tối hơn | `darken(..., 42)` → giảm % (vd 35) |
| Bỏ rail giả bottom/right | Xóa block fill `railB` / `railR` |
| Quay stretch filler | Đổi tile loop → `BltStretchVideoSurface` (không khuyến nghị) |

---

## 6. Backlog liên quan (chưa làm)

1. **Blit hẳn `Map_Bord.sti` / `Mbs.sti` full-size** thay vẽ tay — handoff 27/07 đã ghi; cần xử lý cột roster x0–261.
2. Sector inventory / militia map full-size (đã có fix riêng khác session).
3. Commit riêng: `OGVM-MAPCHROME: leather filler + full-size map frame`.
4. Android: rebuild APK nếu ship native map chrome (engine C++ chung desktop/Android).

---

## 7. Ghi chú kỹ thuật

- `STCI_ETRLE_COMPRESSED = 0x0020`, `STCI_INDEXED = 0x0008` → flag `0x28` (không phải zlib `0x10`).
- `GetRGBColor` / `Get16BPPColor` / `SGPGetRValue` qua `HImage.h` (đã include `MapScreen.cc` + `Interface_Panels.cc`).
- `ColorFillVideoSurfaceArea` friend `SGPVSurface` — gọi OK từ game code.
- Darken rail: lock `guiSAVEBUFFER`, clamp `Width/Height` tránh OOB.
- `MAPZOOM_NUM` local `#define` trong `MapScreen.cc` (`grid >= 60 ? 3 : 2`); zoom lớn đang lock off (`JA2_MAPZOOM_ALLOW_LARGE 0` trong `UILayout.cc`).

---

## 8. Handoff tiếp theo (gợi ý)

1. User screenshot cuối → tinh chỉnh 1 vòng nếu cần.
2. Commit map chrome **tách** edition-detector dirty.
3. (Tuỳ chọn) thử blit `Map_Bord.sti` sub 1024×648 làm ground truth thay synthetic molding.
