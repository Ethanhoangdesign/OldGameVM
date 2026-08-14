# Handoff 2026-08-13 — Android Wildfire attachment audit

## Context
- Previous fix: `docs/HANDOFF-12-08-2026-android-attachment-touch-fix.md` fixed `Silencer P90` (`269`) attaching to `FN P90` (`15`) in Wildfire 6.08.
- Follow-up request: audit the remaining guns / attachments for the same Wildfire item-ID mismatch.

## Data source used
- Wildfire install: `/Users/ethan/JA2/wildfire-gog-608/Data/BinaryData.slf`
- Extracted/decoded `ITEMDESC.edt` from the SLF directory entry:
  - offset `1269809`
  - length `280800`
  - 351 records × 800 bytes
  - UTF-16LE, each non-null char ROT-1 decoded by subtracting 1

## Attachment audit result
Wildfire 6.08 attachment-related item names:

| ID | Wildfire item | Result |
| ---: | --- | --- |
| 207 | Silencer 9mm | Same as vanilla; already OK |
| 208 | Sniper Scope | Same as vanilla; already OK |
| 209 | Bipod | Same as vanilla; already OK |
| 241 | Laser Scope | Same as vanilla; already OK |
| 244 | Duckbill | Same as vanilla; already OK |
| 265 | Silencer .45 | Wildfire-only silencer; needed fix |
| 269 | Silencer P90 | Wildfire-only silencer; existing narrow fix expanded |
| 305 | Silencer MP7 | Wildfire-only silencer; needed fix |
| 306 | Silencer Commando | Wildfire-only silencer; needed fix |
| 307 | Collimator | Wildfire item exists, but not covered by vanilla attachment flags; left untouched |
| 310 | Extender | Same function slot as vanilla `GUN_BARREL_EXTENDER`; already OK |

## Fix applied
File changed:
- `src/game/Tactical/Items.cc`

Changes:
- Replaced `IsWildfireP90Silencer()` with generic `IsWildfireSilencer()`.
- Kept the existing Wildfire gate:
  - `interface/b_map.sti` exists
  - `interface/b_map.pcx` does not exist
- Added confirmed Wildfire 6.08 silencer compatibility:
  - `265 Silencer .45` → `5 Colt 1991`, `8 UMP`, `10 MAC-10`
  - `269 Silencer P90` → `15 P90`
  - `305 Silencer MP7` → `56 MP7`
  - `306 Silencer Commando` → `14 Colt Commando`
- Updated `FindSilencerAttachment()` so these Wildfire silencers also affect weapon behavior:
  - range penalty
  - silenced burst sound
  - silenced shot sound
  - volume reduction

## Why this is still minimal
- Vanilla behavior unchanged.
- No broad Wildfire item-table rewrite.
- Only confirmed Wildfire 6.08 silencer IDs were added.
- `Collimator` was not guessed into laser/scope behavior because its exact engine behavior was not confirmed.

## Verification
Desktop build:

```bash
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)
```

Result: passed.

Items test attempt:

```bash
ctest --test-dir build -R "Items" --output-on-failure
```

Result: no tests registered in this build.

Android debug APK:

```bash
./tools/build-android-debug.sh
```

Result: `BUILD SUCCESSFUL in 20s`.

APK:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

Size: ~49 MB.

## Remaining risk
- This does not solve the full Wildfire externalized item/weapon table mismatch.
- Other non-attachment behavior may still be wrong if it depends on vanilla item IDs.
- If users report `Collimator` behavior or other Wildfire-only attachment behavior, handle it with confirmed data, not guesses.

## Useful decode snippet

```python
from pathlib import Path
raw = Path('/Users/ethan/JA2/wildfire-gog-608/Data/BinaryData.slf').read_bytes()
edata = raw[1269809:1269809 + 280800]

def item_name(i):
    rec = edata[i * 800:(i + 1) * 800]
    s = rec.decode('utf-16le', errors='ignore')
    s = ''.join(chr(ord(c) - 1) if c != '\x00' else '\x00' for c in s)
    return s.split('\x00', 1)[0].strip().replace('\x1f', ' ')

for i in [207, 208, 209, 241, 244, 265, 269, 305, 306, 307, 310]:
    print(i, item_name(i))
```
