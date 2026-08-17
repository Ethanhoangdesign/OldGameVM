#!/usr/bin/env bash
# Build arm64 RelWithDebInfo APK for physical-device profiling.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$ANDROID_HOME}"
export ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-$ANDROID_HOME/ndk/25.0.8775105}"
export PATH="$ANDROID_HOME/cmake/3.22.1/bin:/opt/homebrew/bin:$ANDROID_HOME/platform-tools:$PATH"

if [[ ! -d "$ANDROID_NDK_HOME" ]]; then
  echo "NDK missing: $ANDROID_NDK_HOME"
  exit 1
fi

if [[ ! -f "$ROOT/android/local.properties" ]]; then
  echo "sdk.dir=$ANDROID_HOME" > "$ROOT/android/local.properties"
fi

cd "$ROOT"
./android/gradlew -p android clean
./android/gradlew -p android assembleRelease -PCMAKE_BUILD_TYPE=RelWithDebInfo -PdebugSigning "$@"

APK="$ROOT/android/app/build/outputs/apk/release/app-release.apk"
ls -lh "$APK"
echo "install: adb install -r \"$APK\""
