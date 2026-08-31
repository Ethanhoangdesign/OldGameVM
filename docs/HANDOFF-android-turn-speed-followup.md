# Handoff — Android tactical AI turn profiling

## Trạng thái ngày 2026-08-16

Đã đo trên Android bằng cùng một tactical save ở `1x` và `5x`, sau đó chạy thêm đúng một lượt `1x` với sub-timing trong `DecideActionRed()`.

Kết luận đã được chứng minh bằng log:

- Nút `1x`–`5x` chỉ tăng tốc animation của soldier không thuộc player team.
- Nó không tăng tốc AI decision.
- Save đang thử chậm gần như hoàn toàn do AI decision, không phải animation.
- Hotspot đã xác định: `FindBestNearbyCover()` chiếm gần 100% hai decision chậm.
- `ClosestReachableFriendInTrouble()` và các path wrapper đã đo đều dưới 1 ms.
- Chưa tối ưu AI. Bước kế tiếp là chia nhỏ timing bên trong `FindBestNearbyCover()` tại `src/game/TacticalAI/FindLocations.cc`.

## Thay đổi đang có trong working tree

### Turn-speed UI

`src/game/Tactical/Interface.cc`:

- Hiện nút speed ở player turn, enemy turn và interrupt messages.
- Tooltip: `Turn animations: ...`.
- Touch/F8 vẫn chạy:

```text
1x → 2x → 3x → 4x → 5x → 1x
```

### Animation speed

`src/game/Tactical/Soldier_Control.cc`:

- `gubEnemyTurnAnimationSpeed` chỉ áp dụng khi `INCOMBAT`.
- Chỉ áp dụng cho soldier có `bTeam != OUR_TEAM`.
- Chia animation delay cho speed, clamp tối thiểu `1`.
- Không thay đổi AI, AP, combat rules, path, damage hoặc target selection.

### Optimized Android profiling build

- `tools/build-android-relwithdebinfo.sh`: build arm64 `RelWithDebInfo`.
- `cmake/external-project-cache.cmake.in`: truyền `CMAKE_MAKE_PROGRAM`.
- `dependencies/lib-sdl2/CMakeLists.txt`: truyền Ninja path cho nested build.
- `dependencies/lib-magic_enum/CMakeLists.txt`: truyền Ninja path cho nested build.
- `android/app/build.gradle`:
  - arm64 only;
  - debug signing khi có `-PdebugSigning`;
  - profiling release trở thành debuggable khi có `-PdebugSigning`, cho phép `adb run-as` đọc `cache/ja2.log`.

### AI timing instrumentation

`src/game/TacticalAI/AIMain.cc` hiện log marker `AI_TIMING`:

```text
AI_TIMING soldier_start ...
AI_TIMING decision ... ms=...
AI_TIMING execute ... ms=...
AI_TIMING soldier_end ... total_ms=...
```

Ý nghĩa:

- `decision`: thời gian trong `DecideAction()`.
- `execute`: thời gian gọi `ExecuteAction()`.
- `soldier_end total_ms`: toàn thời gian soldier giữ AI control.

Instrumentation chỉ đo thời gian; không đổi behavior.

## Verification đã chạy

Passed:

```text
git diff --check
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)
./tools/build-android-relwithdebinfo.sh
adb install -r android/app/build/outputs/apk/release/app-release.apk
```

Desktop compile chỉ có warning cũ, gồm `fTileBar` unused trong `Interface.cc`.

Manifest profiling đã xác nhận:

```text
android:debuggable="true"
```

## Quy trình lấy log đã hoạt động

Dừng game:

```bash
adb shell am force-stop io.github.ja2stracciatella
```

Xóa log. Phải giữ dấu nháy kép ngoài; nếu không, redirection chạy sai user và báo `Permission denied`:

```bash
adb shell "run-as io.github.ja2stracciatella sh -c ': > cache/ja2.log'"
```

Mở launcher:

```bash
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Sau một enemy turn:

```bash
adb exec-out run-as io.github.ja2stracciatella cat cache/ja2.log \
  | grep -a 'AI_TIMING' \
  > /tmp/ja2-ai-1x.log
```

Đợt `5x` lưu tại:

```text
/tmp/ja2-ai-5x.log
```

Hai file `/tmp` là dữ liệu máy local, không nằm trong repo.

## Kết quả đo cùng save

Chuỗi soldier/action giữa `1x` và `5x` giống nhau.

| Metric | `1x` | `5x` | Nhận xét |
|---|---:|---:|---|
| Tổng `soldier_end total_ms` | 116,427 ms | 115,248 ms | chênh ~1%, nhiễu đo |
| Tổng `decision ms` | 115,262 ms | 114,064 ms | không được tăng tốc |
| Phần ngoài decision | 1,165 ms | 1,184 ms | gần như giống nhau |

Khoảng 99% enemy turn nằm trong AI decision.

### Hotspot 1 — soldier 22

```text
action=4 = AI_ACTION_TAKE_COVER
1x: 39,127 ms
5x: 38,719 ms
```

Nghi phạm trong `DecideActionRed()`:

- `FindBestNearbyCover()`;
- `ClosestReachableDisturbance()`;
- `InternalGoAsFarAsPossibleTowards()`;
- pathfinding được gọi bởi các hàm trên.

### Hotspot 2 — soldier 24

```text
action=2 = AI_ACTION_SEEK_FRIEND
1x: 76,133 ms
5x: 75,343 ms
```

Nghi phạm trong `DecideActionRed()`:

- `ClosestReachableFriendInTrouble()`;
- `GoAsFarAsPossibleTowards()`;
- pathfinding được gọi bởi hai hàm trên.

Hai decision này chiếm gần toàn bộ thời gian chờ.

## Kết luận kỹ thuật

Không cần đo thêm `1x` so với `5x`: kết quả đã đủ rõ. Tăng animation speed không giải quyết save này.

Không được giảm AI quality để lấy tốc độ. Không thu hẹp:

- cover-search radius;
- pathfinding scope;
- target count;
- shooting/grenade/knife evaluation;
- tactical decision quality.

Chỉ tối ưu redundant work khi action và inputs giữ nguyên.

## Kết quả sub-timing `1x`

Log đã copy vào repo root:

```text
ja2-ai-1x-followup.log
```

File có 53 dòng, 6.4 KB. Hai mẫu quyết định:

```text
soldier 22:
red_cover = 37,278 ms
decision  = 37,281 ms

soldier 24:
red_cover = 71,615 ms
decision  = 71,620 ms
```

Kết luận:

- `FindBestNearbyCover()` chiếm gần 100% cả hai decision chậm.
- Soldier 24 cuối cùng trả `action=2`, nhưng 71 giây đã tiêu trong lần thử cover trước đó; `action=2` không chứng minh seek-friend là hotspot.
- `red_seek_friend`, `red_seek_friend_path`, `red_disturbance`, `red_seek_path`, `red_seek_cautious_path`, `red_cover_disturbance` và `red_cover_path` đều dưới 1 ms trong lượt này.

Instrumentation hiện có:

- `src/game/TacticalAI/AIMain.cc`: `soldier_start`, `decision`, `execute`, `soldier_end`;
- `src/game/TacticalAI/DecideAction.cc`: tám marker `red_*` quanh search/path wrapper.

Desktop compile và Android profiling build đều pass. APK hiện tại:

```text
android/app/build/outputs/apk/release/app-release.apk
```

Build chỉ có hai warning cũ trong Lua và `src/game/Tactical/Interface.cc`.

## Việc cần làm ở session kế tiếp

### 1. Instrument bên trong `FindBestNearbyCover()`

Implementation bắt đầu tại:

```text
src/game/TacticalAI/FindLocations.cc:500
```

Đọc toàn hàm trước. Thêm timing tối thiểu quanh các vòng lặp/call đắt, nhất là pathfinding, cover-value evaluation và per-opponent/per-tile work. Marker phải chứa soldier ID, loop/call context, số candidates nếu có, và `ms`.

Mục tiêu: phân bổ 37/71 giây vào tầng con đầu tiên. Chưa tối ưu khi chưa biết operation cụ thể.

### 2. Build, cài, đo cùng save đúng một lần ở `1x`

Reuse:

```bash
./tools/build-android-relwithdebinfo.sh
adb install -r android/app/build/outputs/apk/release/app-release.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell "run-as io.github.ja2stracciatella sh -c ': > cache/ja2.log'"
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Sau enemy turn:

```bash
adb exec-out run-as io.github.ja2stracciatella cat cache/ja2.log \
  | grep -a 'AI_TIMING' \
  > /tmp/ja2-ai-cover-deep.log
```

Không cần đo `5x`.

### 3. Tối ưu nhỏ nhất có thể

Chỉ sau khi deep timing chỉ ra repeated/redundant work. Ưu tiên exact duplicate computation hoặc reusable result trong cùng decision. Giữ nguyên:

- cover-search radius và candidate set;
- pathfinding scope;
- target count;
- thứ tự random calls;
- selected action/destination;
- tactical behavior và AI quality.

Sau sửa: replay cùng save, so action sequence, destination và timing trước/sau.

## Working-tree safety

Working tree có nhiều thay đổi Android/UI liên quan và một số file handoff. Không reset, checkout hoặc overwrite toàn bộ tree. Kiểm tra trước khi sửa:

```bash
git status --short
git diff -- src/game/TacticalAI/AIMain.cc src/game/TacticalAI/DecideAction.cc
```

Backup patch cũ vẫn có tại:

```text
/tmp/ja2-speed-before-20260815-225021.patch
```
