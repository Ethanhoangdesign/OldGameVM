# Android Inventory Icon Mismatch — Handoff

**Ngày cập nhật:** 2026-08-18  
**Branch:** `feature/multi-edition-detector`  
**Thiết bị:** `R5GL31H83QX`  
**Trạng thái:** **Root cause đã chứng minh. Shared Wildfire mapping fix đã build và cài. Launcher/native activity pass; visual inventory matrix vẫn chưa hoàn tất.**

## 1. Phạm vi lỗi

Android từng hiển thị icon sai item trong khi tooltip đúng. Runtime evidence trực tiếp:

- `.357 Magazine, AP` hiển thị artwork giống quần/leggings;
- lỗi được báo trên tactical merc inventory, strategic map merc inventory và strategic sector inventory pool;
- ordinary icons có outline/highlight ngoài ý muốn.

Không có runtime evidence gốc đủ mạnh cho các nghi vấn `súng hiển thị như dao` hoặc `Computer Diskette` sai. Lawful asset audit cho thấy metadata hiện đúng; không thêm fix cho hai trường hợp này.

## 2. Root cause

Tooltip và artwork dùng hai đường dữ liệu độc lập:

```cpp
GetHelpTextForItem(o)
```

so với:

```cpp
const ItemModel* item = GCM->getItem(o.usItem);
auto graphic = GetSmallInventoryGraphicForItem(item);
```

Tooltip đúng chỉ chứng minh `OBJECTTYPE`/item identity đúng. Icon còn phụ thuộc:

- small graphic path;
- `subImageIndex`;
- active edition resource;
- shared `SGPVObject` cache.

Wildfire giữ các item ID liên quan nhưng thay đổi semantics/order so với vanilla JSON. Lawful decode của Wildfire `BinaryData.slf/ITEMDESC.edt` và `InterFace.slf/Mdp1Items.sti` chứng minh:

```text
ID 86  .357 Speed Loader, AP  -> frame 12
ID 87  .357 Magazine, AP      -> frame 18
ID 88  .357 Speed Loader, HP  -> frame 13
ID 89  .357 Magazine, HP      -> frame 19
ID 90  5.45mm Magazine        -> frame 9
ID 91  5.45mm Magazine, HP    -> frame 10
```

Vanilla metadata đưa ID 87 tới frame 17. Wildfire frames 11 và 17 là artwork quần/leggings. Đây là valid index nhưng sai artwork: tooltip `.357 Magazine, AP` đúng, icon lại giống quần.

Evidence bổ sung:

- Wildfire frame 11 mask trùng frame quần 67;
- Wildfire frame 17 cũng trùng artwork quần;
- correct `.357 Magazine, AP` frame là 18;
- hai installed Wildfire roots chứa archive byte-identical.

Không phân tích hoặc dịch ngược `WF6.exe`.

## 3. Shared fixes đã áp dụng

### 3.1 Ordinary icon không dùng outline blitter

File: `src/game/Tactical/Interface_Items.cc`

```cpp
if (gamepolicy(f_draw_item_shadow))
{
    BltVideoObjectOutlineShadow(buffer, item_vo, gfx_idx, cx - 2, cy + 2);
}
if (outline_colour == SGP_TRANSPARENT)
{
    BltVideoObject(buffer, item_vo, gfx_idx, cx, cy);
}
else
{
    BltVideoObjectOutline(buffer, item_vo, gfx_idx, cx, cy, outline_colour);
}
```

Kết quả đã được user xác nhận:

- unwanted outline/highlight đã hết;
- explicit compatibility/new-item outlines vẫn dùng outline branch.

Lý do: outline blitter xử lý source palette index `254` đặc biệt; plain blit giữ ordinary artwork pixels.

### 3.2 Tooltip luôn bám item đang render

File: `src/game/Tactical/Interface_Items.cc`

```cpp
// Keep hover text tied to the object currently rendered in this slot.
r.SetFastHelpText(GetHelpTextForItem(o));
```

Tooltip refresh nằm ngoài full-redraw-only path. Thay đổi này tránh stale hover text; không phải root cause artwork.

### 3.3 Wildfire-only metadata correction

Files:

```text
src/externalized/DefaultContentManager.cc
src/externalized/ItemModel.cc
src/externalized/ItemModel.h
```

Edition detection dùng cùng resource signal với Wildfire interface art:

```cpp
doesGameResExists("interface/b_map.sti") &&
!doesGameResExists("interface/b_map.pcx")
```

Fixups:

```cpp
{ 86, "bigitems/p1item12.sti", 12, ".357 Speed Loader, AP" },
{ 87, "bigitems/p1item18.sti", 18, ".357 Magazine, AP" },
{ 88, "bigitems/p1item13.sti", 13, ".357 Speed Loader, HP" },
{ 89, "bigitems/p1item19.sti", 19, ".357 Magazine, HP" },
{ 90, "bigitems/p1item09.sti",  9, "5.45mm Magazine" },
{ 91, "bigitems/p1item10.sti", 10, "5.45mm Magazine, HP" },
```

Unified model API:

```cpp
void ItemModel::overrideInventoryGraphics(
    const ST::string& bigPath,
    uint16_t smallSubImageIndex)
{
    inventoryGraphics.big = GraphicModel(bigPath, 0);
    inventoryGraphics.small = GraphicModel(
        inventoryGraphics.small.getPath(),
        smallSubImageIndex);
}
```

Tác dụng:

- giữ existing small-sheet path;
- sửa exact small frame;
- repoint matching standalone big artwork;
- dùng `optionalById()` nếu item table thiếu entry;
- chạy trước magazine/weapon/armour container setup;
- chỉ chạy cho Wildfire;
- vanilla/Gold không đổi;
- không blanket frame offset;
- không item-ID rendering hack;
- không per-screen patch.

Ba inventory surfaces đều gọi `INVRenderItem(...)`; metadata correction tự áp dụng cho tactical, map merc và sector pool.

## 4. Lawful asset audit

Chỉ dùng installed commercial resources ở chế độ read-only và historical source hợp pháp. Không copy commercial assets vào repo.

Verified Wildfire archive hashes, giống nhau giữa hai installed roots:

```text
BigItems.slf
27c910075714108d53ebae359d48b6be3e2c63fec09e3f8214bbbe21d0572412

BinaryData.slf
7737dbf6103ce1b90981618ec85cac4da14ab8218e8d6e6471c2b26db5046d40

InterFace.slf
28c5c00b83ad54f1c36e52aa2d1f70671f1e5e50787602be9e8236e45b9618fe
```

`Mdp1Items.sti`:

```text
Wildfire size: 55763
Wildfire frames: 138
Wildfire SHA-256: e1d72fee99bb1b9f82c97f9acc3a4722e6122ccb912dee4586d02f92e90021f0

Vanilla size: 53174
Vanilla frames: 138
Vanilla SHA-256: 69a0e1f5d9be23247046d802085e7c12c7f449815efe3bbbc27e12c5449c138a
```

### Gun/knife audit

- Enumerated toàn bộ 284 `BigItems.slf` entries;
- decoded toàn bộ `gun00.sti`–`gun90.sti`;
- 14 historical Wildfire gun fixups hiện có trỏ tới matching `gunNN.sti`;
- không thấy gun item nào còn trỏ knife artwork;
- Combat Knife, Throwing Knife và Bloody Knife cố ý dùng `p1item` sheets;
- không thêm gun fixup.

### Computer Diskette audit

Wildfire `ITEMDESC.edt` record 228:

```text
shortName: Diskette
name: Computer Diskette
```

Repo mapping:

```text
small: interface/mdp1items.sti frame 59
big:   bigitems/p1item59.sti
```

Item-index/name mapping và filename correspondence đúng. Không thêm fix; không tuyên bố perceptual equivalence chỉ từ near-solid masks/palettes.

## 5. Diagnostics và build

Temporary `INV-DIAG` instrumentation đã gỡ hoàn toàn:

- không còn `INV-DIAG` marker;
- không còn `DrawKey`/`logged_draws`;
- temporary diagnostic includes đã gỡ.

Validation:

```text
git diff --check: passed
Android release build: BUILD SUCCESSFUL in 5s
APK: android/app/build/outputs/apk/release/app-release.apk
APK mtime: 2026-08-18 10:38:46
APK size: 37843734 bytes
```

Build warnings về Android SDK package locations/XML version và existing C/C++ warnings không chặn APK creation. Không xử lý trong scope này.

## 6. Device status

Device:

```text
R5GL31H83QX device usb:0-1.2 product:a17xx model:SM_A175F device:a17
```

Đã hoàn tất:

```text
APK install: Success
LauncherActivity start: passed
Start JA: passed
StracciatellaActivity foreground: passed
```

Latest foreground evidence:

```text
topResumedActivity=ActivityRecord{55325898 u0 io.github.ja2stracciatella/.StracciatellaActivity t1375}
```

Một screenshot mới đã capture, resize đúng hygiene rule và đọc một lần. Screenshot không hiển thị affected item đủ rõ; không được coi là visual pass. OCR không chạy vì `tesseract` unavailable.

## 7. Verification còn thiếu

Chưa được runtime visual-confirm trên final APK:

1. `.357 Magazine, AP` hiển thị frame 18/magazine, không còn pants;
2. IDs 86–91 hiển thị đúng artwork;
3. tactical merc inventory;
4. strategic map merc inventory;
5. strategic sector inventory pool;
6. Kevlar/Spectra leggings regression;
7. firearm, knife và Computer Diskette regression;
8. intentional compatibility/new-item outlines;
9. touch, controller, `Ctrl+S`, `Alt+S`, RMB overlay và save behavior.

Không khai báo visual success khi chưa mở đúng save/inventory và nhìn thấy affected items.

## 8. Procedure tiếp theo

Game từng giữ foreground sau final install. Bước tiếp theo:

1. Mở save/inventory chứa `.357 Magazine, AP`.
2. Kiểm tra tactical inventory trước.
3. Ghi nhận tooltip, visible icon, screen và slot.
4. Chụp screenshot mới; resize/crop tối đa 768 px trước khi đọc; mỗi screenshot chỉ đọc một lần.
5. Lặp lại ở strategic map merc inventory và sector pool.
6. Kiểm tra IDs 86–91 cùng regression items/invariants ở mục 7.
7. Nếu artwork vẫn sai, mới thêm narrow diagnostics tại shared `INVRenderItem()` boundary; không thêm screen-specific correction.
8. Gỡ diagnostics trước final build nếu phải dùng lại.

## 9. Constraints bắt buộc

```text
KHÔNG đào ra từ `WF6.exe` — giấy phép cấm dịch ngược.
```

```text
Dứt khoát KHÔNG làm: Dịch ngược `WF6.exe` để lấy thông số.
```

Ngoài ra:

- không copy/commit commercial Wildfire artwork hoặc data;
- không blanket subimage/frame offset;
- không item-ID rendering hack;
- không per-screen icon patch;
- không mở rộng thành full Wildfire item-table migration khi chưa có scope/evidence;
- preserve vanilla/Gold behavior;
- preserve intentional compatibility/new-item outlines;
- preserve unrelated Android/EZeus work;
- không xóa unrelated untracked files.

## 10. Working-tree caution

Repository đã có nhiều unrelated modified/untracked Android và EZeus files trước inventory work. Không sửa, xóa hoặc revert chúng.

Relevant inventory files:

```text
src/game/Tactical/Interface_Items.cc
src/externalized/DefaultContentManager.cc
src/externalized/ItemModel.cc
src/externalized/ItemModel.h
docs/HANDOFF-android-inventory-pants-magazine.md
```

Final known state:

```text
root cause: proven
Wildfire IDs 86–91 mapping: corrected
plain-blit outline fix: retained
unwanted outline: user-confirmed fixed
gun/knife audit: no additional defect found
Computer Diskette mapping: already correct
temporary diagnostics: removed
git diff --check: passed
Android build: passed
APK install: passed
launcher/native activity: passed
affected inventory visual matrix: pending
```
