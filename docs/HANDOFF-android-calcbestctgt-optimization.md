# Android CalcBestCTGT() Optimization — Handoff

**Date**: 2026-08-16  
**Branch**: feature/multi-edition-detector  
**Status**: ✅ Complete, merged to branch

---

## Problem

`CalcBestCTGT()` in `FindBestNearbyCover()` was a hotspot on Android:
- ~75% of `red_cover` time spent in `CalcWorstCTGTForPosition()`
- Each candidate called it 8 times (once per direction)
- Directions 1 (NORTH) and 8 (NORTHEAST) both checked the same adjacent tile when moving NORTHEAST → duplicate computation

## Solution

Local cache within each `CalcBestCTGT()` call:
- Cache first `CalcWorstCTGTForPosition()` result (direction 1)
- Reuse for direction 8 if same coordinates
- Scope: single call only (no cross-candidate or cross-world-state cache)
- Added counters: `best_cache_hits`, `best_cache_misses`

## Results

| Soldier | Total red_cover | best_ms (before) | best_ms (after) | Improvement | Cache hit rate |
|---------|-----------------|------------------|-----------------|-------------|----------------|
| 22      | 45,487 ms       | 33,878 ms        | 28,105 ms       | −5,773 ms (−17.0%) | 12.4% (110/886) |
| 24      | 86,087 ms       | 61,723 ms        | 51,522 ms       | −10,201 ms (−16.5%) | 12.4% (207/1,673) |

- AI behavior unchanged (same candidate set, order, random sequence, AP handling)
- Safety: No regression in cover quality or tactical decisions

## Files Changed

- `src/game/TacticalAI/FindLocations.cc` — cache implementation + counters
- `src/game/TacticalAI/AIMain.cc` — counter reporting
- `src/game/TacticalAI/DecideAction.cc` — counter reset

## Rejected Approaches

- `std::map` cache across candidates: Added overhead, no perf gain, caused regression

## Future Work

- **Wider cache scope**: Cache across multiple `CalcBestCTGT()` calls within same `FindBestNearbyCover()` if more perf needed
- **Additional duplicates**: Check other direction pairs that may compute same tile
- **Long replay test**: Run extended gameplay to confirm no hidden behavior changes

## Logs

- Baseline deep profile: `ja2-ai-cover-deep.log`
- Optimization validation: `/tmp/ja2-ai-cover-adj-cache.log`
