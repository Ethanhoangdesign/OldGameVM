# CLAUDE.md — ja2-stracciatella

## Context hygiene

- NEVER `Read` `android/build.log` — use `tail -50 android/build.log` or `grep -E "error:|ERROR" android/build.log`
- NEVER `Read` `*.bak` or `*.ogvm-bak` files — they are stale backups
- cmake build output: pipe through `grep -E "error:|warning:" | tail -30`, never dump full output
- Large C++ files: use `Read` with `offset`+`limit` when only one section needed, not full file
- NEVER `Read` raw emulator screenshots. Resize to max 768px or crop the relevant UI region first.
- Read each screenshot at most once.
