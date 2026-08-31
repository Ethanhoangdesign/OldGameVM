# HANDOFF: ChanceToGetThrough() Internals Profile

**Date:** 2026-08-16
**Branch:** feature/multi-edition-detector
**Status:** Profile complete, instrumentation planned

## Summary

Profiled `ChanceToGetThrough()` call chain to identify internal hotspots after wide cache rejection (0% duplicates in 7,212 calls — see [HANDOFF-android-ctgt-wide-cache-rejected.md](HANDOFF-android-ctgt-wide-cache-rejected.md)).

## Call Chain

```
ChanceToGetThrough(SOLDIERTYPE* pFirer, GridNo end_pos, FLOAT dEndZ)
  Location: src/game/Tactical/LOS.cc:3426
  Purpose: Thin wrapper, determines weapon type (buckshot), calls FireBulletGivenTarget()

FireBulletGivenTarget(pFirer, dEndX, dEndY, dEndZ, usHandItem, sHitBy, fBuckshot, fFake=TRUE)
  Location: src/game/Tactical/LOS.cc:3201
  Purpose: Sets up bullet parameters (start/end positions, angles, increments, weapon flags)
  Key operations:
    - CalculateSoldierZPos() for start/end Z
    - Distance2D() + atan2() for angles
    - CreateBullet() with fFake=TRUE
    - CalculateFiringIncrements() for trajectory
    - Calls FireBullet() then RemoveBullet()

FireBullet(pBullet, fFake=TRUE)
  Location: src/game/Tactical/LOS.cc:3141
  Purpose: When fFake=TRUE, delegates to CalcChanceToGetThrough()
  Key operations:
    - Initializes tile coordinates, LOS indices, cube levels
    - Sets pBullet->target = pFirer->CTGTTarget
    - Returns CalcChanceToGetThrough(pBullet)

CalcChanceToGetThrough(pBullet)
  Location: src/game/Tactical/LOS.cc:2437
  Purpose: Core computation — traverses tiles, checks structures, calculates hit probability
```

## CalcChanceToGetThrough() Internals

### Main Loop Structure

```cpp
do
{
  // 1. Calculate current grid position
  iGridNo = pBullet->iCurrTileX + pBullet->iCurrTileY * WORLD_COLS;

  // 2. Early exit checks
  if (!GridNoOnVisibleWorldTile(iGridNo) || z too high)
  {
    return(0);  // bullet outside world
  }

  // 3. Get map element and calculate heights
  pMapElement = &(gpWorldLevelData[iGridNo]);
  qLandHeight = ...;
  qWallHeight = gqStandardWallHeight + qLandHeight;

  // 4. Check if ground is in way
  iCurrAboveLevelZ = FIXEDPT_TO_INT32(pBullet->qCurrZ - qLandHeight);
  if (iCurrAboveLevelZ < 0)
  {
    return(0);  // ground blocking
  }

  // 5. Assemble local structures list
  iNumLocalStructures = 0;
  pStructure = pMapElement->pStructureHead;
  uiChanceOfHit = ChanceOfBulletHittingStructure(...);

  if (iGridNo == pBullet->sTargetGridNo)
  {
    uiChanceOfHit = 100;  // want to hit target structure
  }

  // 6. Iterate through all structures in tile
  while (pStructure)
  {
    // Structure type branching:
    if (ALWAYS_CONSIDER_HIT)
    {
      // Fences: density checks, special cases for top of fence
      if (STRUCTURE_ANYFENCE)
      {
        if (density < 100) → probability check
        else if (near firer && at top) → probability check
        else if (near target && at top) → probability check
        else → always add
      }
    }
    else if (STRUCTURE_ROOF)
    {
      // Roof crossing check
      if (fCheckForRoof && z crosses wall height)
      {
        return(0);  // roof blocking
      }
    }
    else if (STRUCTURE_PERSON)
    {
      // Person in line of fire
      if (visible && standing && distance thresholds)
      {
        add to local structures with probability
      }
    }
    else if (STRUCTURE_CORPSE)
    {
      // Corpse (only at target or large corpses)
      if (target tile || large corpse)
      {
        add to local structures
      }
    }
    else
    {
      // Generic structure (walls, etc.)
      if (far from firer || intended target)
      {
        add to local structures
      }
    }

    pStructure = pStructure->pNext;
  }

  // 7. Process local structures (calculate impact, resolve hits)
  // ... (not shown in profile, likely another loop)

  // 8. Advance bullet to next tile
  // ... (increment position by qIncrX/Y/Z)

} while (bullet hasn't reached target);
```

### Identified Hotspots

1. **Tile traversal loop**
   - Iterates from firer to target (distance-dependent)
   - Each tile: full structure list traversal
   - Early returns: ground blocking, roof crossing, out of world

2. **Structure iteration per tile**
   - Linked list: `pMapElement->pStructureHead` → `pNext`
   - Number of structures per tile varies (0 to many)
   - Each structure: type checks (fence/roof/person/corpse/generic)

3. **Structure type branching**
   - Fence: density checks, proximity checks (CLOSE_TO_FIRER), height checks
   - Roof: Z-height crossing detection
   - Person: visibility, animation state, distance thresholds (MIN_DIST_FOR_HIT_FRIENDS)
   - Corpse: target tile check, size check
   - Generic: distance check, intended target check

4. **Per-tile calculations**
   - `ChanceOfBulletHittingStructure()` called once per tile
   - Height calculations (qLandHeight, qWallHeight, iCurrAboveLevelZ)
   - Grid position validation

5. **Local structure processing**
   - After assembling list, another loop processes hits
   - Probability accumulation: `iChanceToGetThrough` starts at 100, reduced per hit
   - Impact calculations, structure damage

### Early Return Paths

- Ground blocking: `iCurrAboveLevelZ < 0` → return 0
- Roof crossing: Z crosses wall height → return 0
- Out of world: `!GridNoOnVisibleWorldTile()` or Z too high → return 0

## Baseline Metrics (from previous handoff)

- 7,212 calls in 10 turns
- 0% duplicate (src, gridno, z) tuples
- Wide cache rejected

## Next Steps

### Instrumentation Plan

Add counters to `CalcChanceToGetThrough()`:

```cpp
// Global counters
static UINT32 guiCTGT_TileIterations = 0;
static UINT32 guiCTGT_StructureIterations = 0;
static UINT32 guiCTGT_EarlyReturn_Ground = 0;
static UINT32 guiCTGT_EarlyReturn_Roof = 0;
static UINT32 guiCTGT_EarlyReturn_OutOfWorld = 0;
static UINT32 guiCTGT_MaxTilesPerCall = 0;
static UINT32 guiCTGT_MaxStructuresPerTile = 0;

// In CalcChanceToGetThrough():
UINT32 uiTilesThisCall = 0;
UINT32 uiStructuresThisTile = 0;

do
{
  uiTilesThisCall++;
  guiCTGT_TileIterations++;

  // ... early return checks (increment respective counters)

  // Structure loop
  uiStructuresThisTile = 0;
  while (pStructure)
  {
    uiStructuresThisTile++;
    guiCTGT_StructureIterations++;
    // ...
  }

  if (uiStructuresThisTile > guiCTGT_MaxStructuresPerTile)
  {
    guiCTGT_MaxStructuresPerTile = uiStructuresThisTile;
  }

  // ... advance bullet
} while (...);

if (uiTilesThisCall > guiCTGT_MaxTilesPerCall)
{
  guiCTGT_MaxTilesPerCall = uiTilesThisCall;
}
```

### Metrics to Collect

1. **Tile count distribution**
   - Average tiles per CTGT call
   - Max tiles per call
   - Histogram: 1-5, 6-10, 11-20, 21+ tiles

2. **Structure count distribution**
   - Average structures per tile
   - Max structures per tile
   - Tiles with 0 structures vs many structures

3. **Early return frequency**
   - Ground blocking: how often?
   - Roof crossing: how often?
   - Out of world: how often?

4. **Structure type distribution**
   - Fences: how many per tile?
   - Persons: how many interventions?
   - Roofs: how many crossing checks?

5. **Distance correlation**
   - Tile count vs distance (expected linear)
   - Structure count vs map density

### Implementation Steps

1. Add global counters to LOS.cc (static variables)
2. Instrument `CalcChanceToGetThrough()` with counter increments
3. Add debug page display (Tactical/Interface.cc debug page)
4. Run 10-turn baseline, collect metrics
5. Analyze distribution, identify dominant hotspot
6. Target optimization based on data

### Expected Findings

Hypothesis (ranked by likelihood):

1. **Tile count dominates** — long-range shots traverse many tiles, each with structure checks
   - Optimization: early termination if probability drops below threshold?

2. **Structure iteration dominates** — dense maps have many structures per tile
   - Optimization: spatial indexing? structure type filtering?

3. **Early returns rare** — most calls traverse full path
   - If frequent: optimize early return checks (cheaper height calculations?)

4. **Person interventions rare** — STRUCTURE_PERSON checks infrequent
   - If frequent: cache visibility? simplify distance checks?

## Files to Modify

- `src/game/Tactical/LOS.cc` — add counters to `CalcChanceToGetThrough()`
- `src/game/Tactical/Interface.cc` — add debug page display for metrics

## Reproduction Steps

1. Build with instrumentation
2. Run 10 turns in tactical combat
3. Check debug page for metrics
4. Document distribution in this handoff
5. Identify top hotspot for optimization

## Success Criteria

- Quantified tile count distribution
- Quantified structure count distribution
- Quantified early return frequency
- Clear identification of dominant hotspot (tiles vs structures vs branching)
- Data-driven optimization target
