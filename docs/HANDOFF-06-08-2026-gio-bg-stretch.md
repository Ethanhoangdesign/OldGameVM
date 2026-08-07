# HANDOFF — 06/08/2026, GIO background stretch full-width

Branch: `feature/multi-edition-detector`  
Repo: `Ethanhoangdesign/OldGameVM`  
Phone: Samsung **SM_A175F** `R5GL31H83QX` arm64-v8a  

## 1. Code Status (DONE)
File: `src/game/GameInitOptionsScreen.cc`
- Đổi blit background GIO từ 1:1 (`BltVideoObject` tại `STD_SCREEN_X/Y`) sang stretch (`BltStretchVideoSurface`).
- Phóng to `optionsscreenbackground.sti` (640x480) lấp đầy toàn bộ `FRAME_BUFFER`.
- Thêm `InvalidateScreen()` để update toàn màn hình.
- Các nút, checkbox, text và shadow panel giữ nguyên ở giữa màn hình (`STD_SCREEN_*`).
- Đã thêm include `HImage.h`, `VObject.h`.

## 2. Build Status (BLOCKED)
- Code C++ đã sửa và lưu.
- AI shell bị chặn quyền chạy lệnh build. APK hiện tại (`app-debug.apk`) trên máy là bản cũ.

## 3. Next Steps (USER MANUAL RUN)
Mở terminal Mac, copy paste:

```bash
export ANDROID_HOME="$HOME/Library/Android/sdk"
export ANDROID_SDK_ROOT="$ANDROID_HOME"
export ANDROID_NDK_HOME="$ANDROID_HOME/ndk/25.0.8775105"
export PATH="$ANDROID_HOME/cmake/3.22.1/bin:$ANDROID_HOME/platform-tools:$PATH"

cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"

./tools/build-android-debug.sh
adb -s R5GL31H83QX install -r android/app/build/outputs/apk/debug/app-debug.apk
adb -s R5GL31H83QX shell am force-stop io.github.ja2stracciatella
adb -s R5GL31H83QX shell am start -n io.github.ja2stracciatella/.LauncherActivity \
  -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
```

## 4. Smoke Test
- Vào New Game -> Initial Game Settings.
- Nền rừng cây phải fill đầy khung logic game.
- Nếu còn viền đen sát mép màn hình điện thoại: đó là viền SDL letterbox (do aspect ratio máy ≠ độ phân giải). Đổi sang res wide hơn (như 1664x768) trong Setting để khớp aspect.
- Có thể áp dụng pattern này cho `Options_Screen.cc` sau này nếu cần.