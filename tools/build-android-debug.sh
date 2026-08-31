#!/usr/bin/env bash
# Build arm64 debug APK for Samsung / physical device.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-$ANDROID_HOME}"
export ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-$ANDROID_HOME/ndk/25.0.8775105}"
# nested CMake (SDL2/magic_enum) needs ninja on PATH
export PATH="$ANDROID_HOME/cmake/3.22.1/bin:/opt/homebrew/bin:$ANDROID_HOME/platform-tools:$PATH"

need() { command -v "$1" >/dev/null || { echo "missing: $1"; exit 1; }; }
need java
need rustc
need cargo

RUSTLIB_TARGET="${RUSTUP_HOME:-$HOME/.rustup}/toolchains/$(rustup show active-toolchain | awk '{print $1}')/lib/rustlib/aarch64-linux-android"
if ! rustup target list --installed | grep -q aarch64-linux-android \
   && [[ ! -d "$RUSTLIB_TARGET" ]]; then
  echo "install rust target aarch64-linux-android first:"
  echo "  rustup target add aarch64-linux-android"
  exit 1
fi

if [[ ! -d "$ANDROID_NDK_HOME" ]]; then
  echo "NDK missing: $ANDROID_NDK_HOME"
  exit 1
fi

if [[ ! -f "$ROOT/android/local.properties" ]]; then
  echo "sdk.dir=$ANDROID_HOME" > "$ROOT/android/local.properties"
fi

cd "$ROOT"
./android/gradlew -p android assembleDebug "$@"

APK="$ROOT/android/app/build/outputs/apk/debug/app-debug.apk"
ls -lh "$APK"
echo "install: adb install -r \"$APK\""
