# HANDOFF — Android Crash Reporting

## Context

Android previously retained `Error.sav`, but native crash text lived only in process memory. Replaying `Error.sav` therefore did not necessarily reproduce the original failure. SIGSEGV/SIGABRT also bypassed the C++ exception path.

## Implemented

- `src/sgp/SGP.cc`
  - Android run marker: `.ja2/game-running`.
  - Signal marker: `.ja2/crash-signal` for `SIGSEGV`, `SIGABRT`, `SIGBUS`, `SIGILL`, `SIGFPE`.
  - C++ exception report: `.ja2/crash-report`.
  - Normal SDL shutdown removes the run marker.
  - Signal handler writes only a small async-signal-safe numeric marker, restores default handlers, then re-raises.
- `rust/stracciatella/src/logger.rs`
  - Android `ja2.log` now uses durable app-private `.ja2` storage instead of cache storage.
  - Previous log copied to `.ja2/ja2.log.last` before a new run truncates the active log.
- `android/app/src/main/java/io/github/ja2stracciatella/NativeException.kt`
  - Reads durable crash/signal/run files.
  - Creates and clears the run marker.
- `android/app/src/main/java/io/github/ja2stracciatella/LauncherActivity.kt`
  - Detects the previous exception, native signal, or stale run marker at launch/resume.
  - Posts one Android notification per report content hash.
  - Keeps report files; does not upload automatically.
- `android/app/src/main/java/io/github/ja2stracciatella/ui/main/LogsTabFragment.kt`
  - Displays crash report, signal marker, current log, and previous log.

## User flow

1. Start game: `.ja2/game-running` is created.
2. Normal quit: marker is deleted.
3. C++ exception: `crash-report` plus `Error.sav` remain.
4. Hard native crash/process death: marker remains; signal marker may contain the signal number.
5. Open launcher: notification and Toast identify the previous crash; Logs tab exposes evidence for copying.

Notification cannot be delivered after process death. It appears when the launcher is opened/resumed next. SIGKILL, power loss, or filesystem failure can only be classified as an unexpected process death if the run marker survives.

## Verification

- `git diff --check`: passed.
- Android Gradle build: not completed; environment rejected the build command.
- Run:

```bash
./gradlew -p android :app:assembleDebug
```

- Manual test: normal quit, managed C++ exception, forced process kill, launcher restart, notification, Logs tab copy.

## Follow-up

- Verify APK compilation.
- Test Android 13 notification permission; current target SDK is 26, so no runtime request is required for the configured target.
- Confirm signal marker behavior on a real arm64 device.
- Consider replacing the simple signal marker with platform tombstone integration only if native stack traces become necessary.
