#!/bin/zsh
set -euo pipefail

export ANDROID_HOME="${ANDROID_HOME:-$HOME/Library/Android/sdk}"
export PATH="$ANDROID_HOME/platform-tools:$ANDROID_HOME/emulator:$PATH"

REPO="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO"

AVD="${1:-Pronunciation_API_35}"
APK="$REPO/android/app/build/outputs/apk/debug/app-debug.apk"
PKG="io.github.ja2stracciatella"
ACT="$PKG/.LauncherActivity"

echo "== SDK =="
echo "ANDROID_HOME=$ANDROID_HOME"
command -v adb
command -v emulator
echo "== AVDs =="
emulator -list-avds || true
echo "== adb devices (before) =="
adb devices || true

if ! adb devices 2>/dev/null | awk 'NR>1 && $2=="device"{found=1} END{exit !found}'; then
  echo "== starting emulator: $AVD =="
  nohup emulator -avd "$AVD" -netdelay none -netspeed full -no-snapshot-save >/tmp/ogvm-emulator.log 2>&1 &
  echo "emulator pid $!"
  echo "== waiting for device =="
  adb wait-for-device
  # wait boot
  for i in {1..90}; do
    boot="$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')"
    if [[ "$boot" == "1" ]]; then
      echo "boot completed"
      break
    fi
    sleep 2
  done
else
  echo "device already online"
fi

echo "== adb devices =="
adb devices

echo "== rebuild =="
./android/gradlew -p android clean
./tools/build-android-debug.sh

echo "== install/start =="
adb install -r "$APK"
adb shell am force-stop "$PKG"
adb shell am start -n "$ACT"
echo "DONE"
