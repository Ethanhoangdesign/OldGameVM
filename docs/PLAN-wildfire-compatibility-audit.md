# PLAN — Wildfire 6.08 Compatibility Audit

## Mục tiêu

Xây lớp tương thích Wildfire dựa trên evidence:

```text
Wildfire identity
→ engine model
→ graphics
→ tile
→ behavior
→ item provenance
```

Không vá riêng từng màn hình. Không đoán từ tên hoặc Vanilla ID. Không ảnh hưởng Vanilla/Gold.

## Phase 0 — Freeze baseline

### Việc làm

- Ghi nhận desktop `135/135`.
- Giữ Wildfire gate hiện tại.
- Giữ các row đã xác nhận.
- Giữ fallback IDs `77`, `104`, `105`, `111`, `112`, `113` ở trạng thái explicit.
- Không thêm calibre native giả.
- Không copy archive assets.

### Output

- Baseline test log.
- Handoff hiện tại.
- Danh sách confirmed/fallback/unknown.

### Gate

Không sửa model trước khi baseline được ghi nhận.

## Phase 1 — Resource collision audit

### Status

Completed read-only archive/STCI audit. User-provided labeled Wildfire guns reference confirms Machete `gun47` and ID `77` `gun73` artwork. Apply exact shared gun art mappings.

### Input

- `BinaryData.slf`
- `InterFace.slf`
- `BigItems.slf`
- externalized JSON tables
- `tools/audit_wildfire_items.py`

### Audit schema

Mỗi item ID cần có:

```text
itemIndex
Wildfire display name
source table
internalName
item class
internal type
calibre
capacity
ammo type
small path/frame
big path/frame
 tile type/index
attachment flags
classification
 evidence
```

### Collision rules

Báo cáo khi:

- hai identity khác nhau dùng cùng small path/frame;
- hai identity khác nhau dùng cùng big path;
- small/big/tile trỏ tới family khác nhau;
- item class không khớp artwork;
- magazine calibre/capacity không khớp weapon family;
- confirmed Wildfire identity dùng resource Vanilla của identity khác.

### Priority

```text
54 Machete ↔ 66 VSS
77 4.6mm Magazine
78 7.62×54mm Magazine, 10 AP
71–115 magazines
first-mission weapons/items
armour/explosives/generic items
attachments
```

### Output

```text
/tmp/wildfire-collision-audit.json
```

### Gate

Mỗi proposed change phải có `confirmed`, `fallback`, hoặc `unknown`.

## Phase 2 — Resolve Machete/VSS collision

### Việc làm

1. Xác định native Machete small/big/tile association read-only.
2. So sánh với VSS association.
3. Thêm correction tại `ItemModel`/model deserialization hoặc overlay load.
4. Không sửa `INVRenderItem()`.
5. Không thêm item-ID branch vào renderer.
6. Chỉ thêm tile override nếu tile mismatch đã chứng minh.

### Test

```text
Wildfire ID 54 != gun41
Wildfire ID 66 giữ artwork VSS
Vanilla ID 54 giữ mapping cũ khi gate off
```

### Gate

Machete và VSS có resource identity khác nhau ở mọi shared consumer.

## Phase 3 — Trace item provenance

### Đích đầu tiên: ID 78

Tìm mọi đường tạo item:

- map placement data;
- map replacement/reward;
- random item pool;
- sector inventory generation;
- dealer inventory;
- merc starting inventory;
- tactical item creation;
- reload/default magazine selection.

### Trace format

```text
edition
map/sector
source file/record
creation function
item ID
quantity
status
owning weapon/calibre
```

### Phân loại

#### Hợp lệ

ID `78` xuất hiện theo Wildfire placement/pool. Giữ source; chỉ xử lý fallback semantics/art nếu có evidence.

#### Sai pool

ID `78` sinh từ Vanilla item pool hoặc calibre mapping sai. Sửa shared source/pool.

#### Chưa đủ evidence

Không patch. Ghi unknown, tạo reproduction requirement.

### Gate

Giải thích được vì sao ID `78` xuất hiện trong first mission.

## Phase 4 — Audit model semantics

Audit từng nhóm:

### Weapons

- item class;
- calibre;
- magazine compatibility;
- burst/auto flags;
- attachments;
- reload behavior;
- inventory art;
- tile art.

### Magazines

- capacity;
- ammo type;
- calibre fallback;
- small/big art;
- native family identity;
- weapon compatibility.

### Generic items

- class;
- use action;
- attachment role;
- small/big/tile art.

### Armour

- armour type;
- protection values;
- placement slot;
- art collision.

### Explosives

- explosive class;
- launcher compatibility;
- action behavior;
- art/tile.

### Attachments

- Wildfire-only compatibility;
- silencer behavior;
- upgrade/bolt/spring compatibility;
- no speculative broadening.

### Gate

Chỉ confirmed deltas mới vào implementation.

## Phase 5 — Minimal shared overlay

### Design

Dùng một Wildfire-only overlay áp dụng sau khi load JSON, trước khi tạo containers/indexes.

Order:

```text
load base JSON
→ detect Wildfire
→ apply confirmed overlay
→ construct models/containers
→ expose shared consumers
```

### Overlay contents

Chỉ gồm:

- confirmed metadata;
- confirmed small/big graphics;
- confirmed tile graphics;
- confirmed compatibility/behavior;
- explicit fallback annotations.

### Không làm

- per-screen patch;
- renderer item-ID switch;
- blanket frame offset;
- đổi toàn bộ stats theo `ITEMDESC.edt`;
- tạo calibre mới chỉ để khớp tên;
- thay unknown rows bằng closest Vanilla item.

### API rule

Chỉ mở rộng `ItemModel` khi có confirmed tile/graphics use case. Không tạo factory/profile abstraction thừa.

## Phase 6 — Tests

### Unit tests

Mở rộng `DefaultContentManager_unittests.cc`:

- Wildfire gate on/off;
- Machete/VSS collision;
- confirmed weapon rows;
- magazine rows;
- ID `77` fallback classification;
- ID `78` provenance/metadata behavior;
- small/big/tile consistency;
- unaffected Vanilla rows;
- silencer IDs `265`, `269`, `305`, `306`.

### Audit self-check

Mở rộng `tools/audit_wildfire_items.py --self-check` cho:

- duplicate small references;
- duplicate big references;
- invalid archive bounds;
- malformed externalized rows;
- evidence classification completeness.

### Behavior test

Thêm test nhỏ nhất có thể cho item source/pool correction nếu Phase 3 chứng minh cần sửa.

## Phase 7 — Verification

### Desktop

```sh
cmake --build build -j2 2>&1 | grep -E "error:|warning:" | tail -30
cd build
./ja2 -unittests 2>&1 | tee /tmp/ja2-unittests.log
```

Expected:

```text
[  PASSED  ] 135 tests.
```

### Audit

```sh
python3 tools/audit_wildfire_items.py --self-check
python3 tools/audit_wildfire_items.py \
  "/Users/ethan/JA2/wildfire-gog-608/Data" \
  > /tmp/wildfire-audit.json
```

### Android

Chỉ sau desktop pass:

```sh
./tools/build-android-debug.sh
adb -s R5GL31H83QX install -r android/app/build/outputs/apk/debug/app-debug.apk
adb -s R5GL31H83QX exec-out run-as io.github.ja2stracciatella \
  cat cache/ja2.log > /tmp/ja2-android-after-test.log
```

Filter:

```sh
grep -E "WF-MAG|WF-ITEMART|ERROR|Exception|crash|reload|Reload|magazine|Magazine" \
  /tmp/ja2-android-after-test.log | tail -100
```

### Runtime surfaces

Kiểm tra:

1. tactical inventory;
2. strategic merc inventory;
3. sector inventory pool;
4. ground/tile item;
5. tooltip name/icon;
6. reload;
7. magazine compatibility;
8. attachments;
9. first mission item timing/source.

### Cleanup

- remove temporary trace;
- `git diff --check`;
- inspect `git status`;
- confirm no commercial assets;
- confirm no personal path;
- no commit unless requested.

## Milestones

### M1 — Evidence baseline

- audit report generated;
- collisions listed;
- Machete/VSS confirmed;
- ID `78` source candidates listed.

### M2 — First shared correction

- Machete no longer uses VSS art;
- VSS unaffected;
- unit coverage passes.

### M3 — Provenance correction

- ID `78` source explained;
- shared pool/source fixed only if needed;
- regression test added.

### M4 — Category audit

- weapons, magazines, generic items, armour, explosives, attachments classified.

### M5 — Release verification

- desktop green;
- Android surfaces verified;
- fallback/unknown list documented;
- working tree clean except intentional source/docs changes.

## Definition of done

```text
Không còn confirmed collision chưa xử lý.
Machete không render thành VSS.
VSS vẫn đúng.
ID 78 có provenance explanation.
ID 77 không bị claim native art khi thiếu evidence.
Overlay chạy ở shared model layer.
Vanilla/Gold không đổi.
Desktop tests pass.
Android kiểm tra đủ inventory surfaces.
Không có binary thương mại trong diff.
```
