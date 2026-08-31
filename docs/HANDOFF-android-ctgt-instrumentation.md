# HANDOFF: CTGT Instrumentation & Build Instructions

**Date:** 2026-08-16
**Branch:** feature/multi-edition-detector
**Status:** Instrumentation added, ready for build & test

## Summary

Added profiling counters to `CalcChanceToGetThrough()` in `src/game/Tactical/LOS.cc` to measure:
- Total calls
- Tile iterations per call
- Structure iterations per tile
- Early return frequency (ground/roof/out-of-world)
- Max tiles per call
- Max structures per tile
- Empty tiles (no structures)

## Files Modified

1. **src/game/Tactical/LOS.cc**
   - Added 10 global counters (lines ~2437-2446)
   - Added per-call counters in `CalcChanceToGetThrough()` (lines ~2468-2470)
   - Incremented counters at key points:
     - Tile loop entry (line ~2510)
     - Structure list traversal (lines ~2517-2523)
     - Early returns: ground (line ~2561), roof (line ~2712), out-of-world (line ~2884)
     - Max tiles tracking (line ~2927)

2. **src/game/Tactical/LOS.h**
   - Declared 10 extern counters for debug page access (lines ~62-71)

## Counters Added

```cpp
// Global counters (LOS.cc)
static UINT32 guiCTGT_TotalCalls = 0;
static UINT32 guiCTGT_TotalTileIterations = 0;
static UINT32 guiCTGT_TotalStructureIterations = 0;
static UINT32 guiCTGT_EarlyReturn_Ground = 0;
static UINT32 guiCTGT_EarlyReturn_Roof = 0;
static UINT32 guiCTGT_EarlyReturn_OutOfWorld = 0;
static UINT32 guiCTGT_MaxTilesPerCall = 0;
static UINT32 guiCTGT_MaxStructuresPerTile = 0;
static UINT32 guiCTGT_EmptyTiles = 0;
```

## Build Instructions

### Android (Termux)

```bash
cd /data/data/com.termux/files/home/ja2-stracciatella
./tools/build-android.sh 2>&1 | tee android/build.log
```

**Expected output:**
```
BUILD SUCCESSFUL in Xs
```

**Check for errors:**
```bash
grep -E "error:|ERROR" android/build.log | tail -20
```

### Desktop (Linux/macOS)

```bash
cd /path/to/ja2-stracciatella
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## Test Instructions

### 1. Run 10 Turns in Tactical Combat

1. Launch game
2. Start tactical combat (any scenario with multiple enemies)
3. Play 10 turns (or until combat ends)
4. Exit to main menu

### 2. View Counters

**Option A: Debug Page (if implemented)**

Press `Ctrl+D` to cycle debug pages. CTGT metrics should appear on a debug page.

**Option B: Log Output (current implementation)**

Counters are currently only tracked in memory. To view them, add a debug page or log output.

**Temporary: Add log output to LOS.cc**

After the `CalcChanceToGetThrough()` function ends (line ~2939), add:

```cpp
// Log every 1000 calls
if (guiCTGT_TotalCalls % 1000 == 0)
{
    SLOGI("CTGT stats: calls={}, tiles/call={:.1f}, structs/tile={:.1f}, empty={}%, ground={}, roof={}, outofworld={}, max_tiles={}, max_structs={}",
        guiCTGT_TotalCalls,
        guiCTGT_TotalCalls > 0 ? (float)guiCTGT_TotalTileIterations / guiCTGT_TotalCalls : 0.0f,
        guiCTGT_TotalTileIterations > 0 ? (float)guiCTGT_TotalStructureIterations / guiCTGT_TotalTileIterations : 0.0f,
        guiCTGT_TotalTileIterations > 0 ? (guiCTGT_EmptyTiles * 100.0f / guiCTGT_TotalTileIterations) : 0.0f,
        guiCTGT_EarlyReturn_Ground,
        guiCTGT_EarlyReturn_Roof,
        guiCTGT_EarlyReturn_OutOfWorld,
        guiCTGT_MaxTilesPerCall,
        guiCTGT_MaxStructuresPerTile);
}
```

Then check log:
```bash
adb logcat | grep "CTGT stats"
```

### 3. Expected Metrics

**Baseline (10 turns):**
- `guiCTGT_TotalCalls`: ~7,000-8,000 (from previous handoff: 7,212)
- `guiCTGT_TotalTileIterations`: ~50,000-100,000 (estimate)
- `guiCTGT_TotalStructureIterations`: ~200,000-500,000 (estimate)
- `guiCTGT_EarlyReturn_Ground`: rare (<1%)
- `guiCTGT_EarlyReturn_Roof`: rare (<1%)
- `guiCTGT_EarlyReturn_OutOfWorld`: rare (<1%)
- `guiCTGT_MaxTilesPerCall`: 10-30 (distance-dependent)
- `guiCTGT_MaxStructuresPerTile`: 5-20 (map-dependent)
- `guiCTGT_EmptyTiles`: 30-70% (map-dependent)

## Analysis Plan

### Identify Dominant Hotspot

1. **Tile iterations dominate?**
   - If `TotalTileIterations / TotalCalls > 10`: long-range shots traverse many tiles
   - Optimization: early termination if probability drops below threshold?

2. **Structure iterations dominate?**
   - If `TotalStructureIterations / TotalTileIterations > 5`: dense maps have many structures per tile
   - Optimization: spatial indexing? structure type filtering?

3. **Early returns rare?**
   - If `EarlyReturn_* < 1%`: most calls traverse full path
   - Optimization: optimize early return checks (cheaper height calculations?)

4. **Empty tiles frequent?**
   - If `EmptyTiles / TotalTileIterations > 50%`: many tiles have no structures
   - Optimization: skip structure checks for empty tiles?

### Next Steps

Based on data:

1. **If tiles dominate:** profile tile traversal loop (distance calculation, increment updates)
2. **If structures dominate:** profile structure iteration (linked list traversal, type branching)
3. **If early returns rare:** optimize height calculations (qLandHeight, qWallHeight)
4. **If empty tiles frequent:** add early exit for empty tiles

## Success Criteria

- Build succeeds without errors
- Game runs 10 turns without crashes
- Counters show meaningful distribution
- Clear identification of dominant hotspot (tiles vs structures vs branching)
- Data-driven optimization target identified

## Rollback

If issues:
```bash
git checkout src/game/Tactical/LOS.cc src/game/Tactical/LOS.h
```
