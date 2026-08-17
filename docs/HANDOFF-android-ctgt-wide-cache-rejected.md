# Android Tactical AI: Cache Rộng CTGT Bị Từ Chối

**Ngày**: 2026-08-16  
**Branch**: feature/multi-edition-detector  
**Trạng thái**: ❌ Từ chối — 0% duplicate, không có cơ hội tối ưu

---

## Tóm tắt

Đã thử mở rộng cache CTGT ngoài direction 1/8 trong `CalcBestCTGT()`. Sau khi đo baseline 10 turns và thêm probe fixed-size, **phát hiện 0% exact duplicate** trong 7,212 lệnh gọi `SoldierToLocationChanceToGetThrough()` raw. Cache rộng chỉ thêm overhead mà không có lợi ích. Đã gỡ probe, giữ tối ưu direction 1/8.

---

## Đo Baseline (10 Enemy Turns)

Build: `tools/build-android-relwithdebinfo.sh` → `app-release.apk` (debuggable)

### Số liệu chính

| Metric | Count | Tổng | Median | Max |
|--------|-------|-------|--------|-----|
| `soldier_start` | 107 | - | - | - |
| `soldier_end` | 105 | 860,893 ms | 161 ms | 85,692 ms |
| `decision` | 143 | 822,904 ms | 1 ms | 85,676 ms |
| `red_cover` | 25 | 785,490 ms | 19,680 ms | 85,671 ms |
| `cover_deep` calls | 34 | - | - | - |
| `best_ms` | 34 | 558,939 ms | 11,024 ms | 61,330 ms |
| `worst_ms` | 34 | 104,647 ms | 2,658 ms | 8,698 ms |
| `average_ms` | 34 | 155,483 ms | 3,476 ms | 15,454 ms |

### Hiệu năng Cache Direction 1/8

- `best_cache_hits`: 1,520
- `best_cache_misses`: 13,331
- **Hit rate**: 10.24%
- Xác nhận tối ưu direction 1/8 vẫn có giá trị

### Crash/Deadlock

- **Không crash** trong 10 turns
- Mọi `soldier_start` đều có `soldier_end` tương ứng (2 chưa đóng lúc trích xuất log)
- Destination logging: 143/143 decisions được log với `usActionData`

---

## Test Probe Cache Rộng

Đã thêm probe fixed-capacity (1024 entries, hash-based) để đếm exact duplicate `SoldierToLocationChanceToGetThrough()` trong toàn bộ lệnh gọi `FindBestNearbyCover()`.

### Probe Key

```cpp
struct CTGTProbeKey {
  const SOLDIERTYPE* shooter;
  const SOLDIERTYPE* target;
  INT16 shooterGridNo;
  INT16 targetGridNo;
  UINT16 shooterAnimState;
  INT8 shooterLevel;
  INT8 targetLevel;
  INT8 cubeLevel;
};
```

### Kết quả (Một Enemy Turn)

| Metric | Count | Tổng | Median | Max |
|--------|-------|-------|--------|-----|
| `cover_deep` calls | 14 | - | - | - |
| `ctgt_probe_calls` | 14 | 7,212 | 561 | 1,687 |
| `ctgt_probe_duplicates` | 14 | **0** | 0 | 0 |
| `ctgt_probe_collisions` | 14 | 2,417 | 91 | 844 |
| `red_cover` | 8 | 161,435 ms | 18,181 ms | 33,232 ms |

### Kết luận

- **Duplicate rate**: 0.00% (0 / 7,212)
- **Collision rate**: 33.5% (2,417 / 7,212) — hash collisions, không phải duplicates
- Cache sẽ thêm overhead (tính hash, truy cập memory) mà không có lợi ích
- Thử `std::map` trước đó đã thất bại vì cùng lý do

---

## File đã thay đổi

### Thêm (Baseline Instrumentation)

- `src/game/TacticalAI/AIMain.cc` (+16 dòng)
  - `soldier_start`: log `ubID`, `bTeam`, `bAlertStatus`, `bActionPoints`
  - `decision`: log `ubID`, `bTeam`, `bAlertStatus`, `bAction`, **`usActionData`** (mới), `ms`
  - `execute`: log `ubID`, `selectedAction`, `ms`, `bActionInProgress`
  - `soldier_end`: log `ubID`, `bTeam`, `bAlertStatus`, `total_ms`

- `src/game/TacticalAI/FindLocations.cc` (+78 dòng)
  - `CoverTiming` struct: track `path`, `worstCtgt`, `bestCtgt`, `averageCtgt` durations + counters
  - `CoverTimingLog`: RAII wrapper log khi hủy
  - Cache direction 1/8 trong `CalcBestCTGT()`: `sFirstSpot` / `bFirstCTGT`
  - Counters: `best_cache_hits`, `best_cache_misses`, `candidates`, `reachable`, `evaluated`, `calls`
  - Timing instrumentation: `CalcWorstCTGTForPosition`, `CalcBestCTGT`, `CalcAverageCTGTForPosition`, `FindBestPath`

### Đã Revert

- `CTGTProbe` struct, `CTGTProbeKey`, `ProbeChanceToGetThrough()` wrapper
- Probe counters: `ctgtProbeCalls`, `ctgtProbeDuplicates`, `ctgtProbeCollisions`
- Tất cả probe instrumentation đã gỡ, code trở về gọi `SoldierToLocationChanceToGetThrough()` trực tiếp

### Trạng thái Working Tree

```
 M src/game/TacticalAI/AIMain.cc        (+16 -0)
 M src/game/TacticalAI/FindLocations.cc (+78 -10)
 M src/game/TacticalAI/DecideAction.cc  (+35 -1)  [sub-timing từ session trước]
```

---

## Bước tái hiện

### 1. Build Baseline APK

```bash
./tools/build-android-relwithdebinfo.sh > /tmp/build.log 2>&1
grep -E "error:|warning:|app-release.apk" /tmp/build.log | tail -30
```

### 2. Cài & Xóa Log

```bash
adb install -r android/app/build/outputs/apk/release/app-release.apk
adb shell am force-stop io.github.ja2stracciatella
adb shell "run-as io.github.ja2stracciatella sh -c ': > cache/ja2.log'"
adb logcat -c
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

### 3. Chạy Baseline (10 Enemy Turns)

Trong game:
- Load tactical save có hotspot soldiers (22, 24)
- Đặt speed `1x`
- Chạy 10 enemy turns hoàn chỉnh
- Không save/reload giữa các turns

### 4. Trích xuất Log

```bash
adb exec-out run-as io.github.ja2stracciatella cat cache/ja2.log \
  | grep -a 'AI_TIMING' \
  > /tmp/ja2-ai-cover-baseline.log

adb logcat -d \
  | grep -iE 'FATAL|AndroidRuntime|SIGSEGV|deadlock' \
  | tail -80 \
  > /tmp/ja2-ai-cover-crash.log
```

### 5. Phân tích

```bash
wc -l /tmp/ja2-ai-cover-baseline.log
grep -c 'AI_TIMING soldier_start' /tmp/ja2-ai-cover-baseline.log
grep -c 'AI_TIMING soldier_end' /tmp/ja2-ai-cover-baseline.log

# Tổng hợp metrics chính
python3 - <<'PY'
import re, statistics, pathlib
p = pathlib.Path('/tmp/ja2-ai-cover-baseline.log')
lines = p.read_text(errors='replace').splitlines()
metrics = {}
for key in ['red_cover', 'decision', 'soldier_end', 'best_ms', 'worst_ms', 'average_ms', 
            'best_cache_hits', 'best_cache_misses']:
  metrics[key] = []
for line in lines:
  if 'AI_TIMING red_cover ' in line:
    m = re.search(r'ms=(\d+)', line)
    if m: metrics['red_cover'].append(int(m.group(1)))
  elif 'AI_TIMING decision ' in line:
    m = re.search(r'ms=(\d+)', line)
    if m: metrics['decision'].append(int(m.group(1)))
  elif 'AI_TIMING soldier_end ' in line:
    m = re.search(r'total_ms=(\d+)', line)
    if m: metrics['soldier_end'].append(int(m.group(1)))
  elif 'AI_TIMING cover_deep ' in line:
    for key in ['best_ms', 'worst_ms', 'average_ms', 'best_cache_hits', 'best_cache_misses']:
      m = re.search(rf'{key}=(\d+)', line)
      if m: metrics[key].append(int(m.group(1)))

for key, arr in metrics.items():
  if arr:
    print(f"{key}: n={len(arr)} total={sum(arr)} median={statistics.median(arr):.1f} max={max(arr)}")
PY
```

---

## Bước tiếp theo

### Ngay lập tức: Profile `ChanceToGetThrough()` bên trong

Cache direction 1/8 đã cạn (10% hit rate, 0% wider duplicates). Hotspot còn lại nằm trong chính `ChanceToGetThrough()`:

1. **Thêm instrumentation mức tile** trong `CalcChanceToGetThrough()` (`LOS.cc:2437`):
   - Đếm tổng số tiles đã traverse
   - Đếm empty vs non-empty tiles (có structures)
   - Đếm structure scans (số lần lặp qua `gpLocalStructure`)
   - Đếm early returns (ground collision, roof collision, max range)

2. **Đo một hot decision đơn lẻ**:
   ```bash
   # Build với ChanceToGetThrough() instrumentation
   # Chạy 1 enemy turn
   # Trích xuất: grep 'AI_TIMING ctgt_tiles' /tmp/ctgt-tile-profile.log
   ```

3. **Ứng viên tối ưu** (chỉ khi dữ liệu hỗ trợ):
   - Empty-tile fast-path đã có (`LOS.cc:2669-2734`)
   - Structure scan loop (`LOS.cc:2756-2808`) là O(local_structures × cubes)
   - Early returns đã được tối ưu (ground tại 2654, roof tại 2662)

### Trì hoãn

- **Cross-call cache**: Chỉ khi có world state mutation tracking (vd: combat events invalidate cache)
- **Multi-threading**: `FindBestNearbyCover()` không thread-safe (mutate `pSoldier->sGridNo`, `pSoldier->usAnimState`)
- **Thay đổi algorithm**: Sẽ thay đổi AI behavior, vi phạm ràng buộc "không giảm chất lượng"

---

## Checklist xác minh

- [x] Desktop build: `cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)` — PASS
- [x] Android build: `./tools/build-android-relwithdebinfo.sh` — PASS (chỉ có warnings cũ)
- [x] APK install: `adb install -r` — SUCCESS
- [x] Baseline 10 turns: không crash, 105/107 soldiers hoàn thành
- [x] Probe test: xác nhận 0 duplicates
- [x] Gỡ probe: code sạch, không còn instrumentation dư
- [x] Cache direction 1/8: vẫn hoạt động, 10.24% hit rate

---

## Handoffs liên quan

- `HANDOFF-android-calcbestctgt-optimization.md` — triển khai cache direction 1/8
- `HANDOFF-android-turn-speed-followup.md` — deep timing instrumentation cho `FindBestNearbyCover()`

---

## Log Files

- Baseline 10 turns: `/tmp/ja2-ai-cover-baseline-10turns.log` (830 dòng, 29 KB)
- Crash log: `/tmp/ja2-ai-cover-baseline-crash.log` (0 dòng, rỗng)
- Probe test: `/tmp/ja2-ai-cover-probe.log` (14 `cover_deep` calls)

**Ghi chú**: Logs không commit vào repo. Tái tạo bằng các bước trên.
