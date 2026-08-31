# HANDOFF — 06/08/2026, Main Menu stretch & GIO save buffer fix

Branch: `feature/multi-edition-detector`  
Repo: `Ethanhoangdesign/OldGameVM`  
Simulator: **Pronunciation_API_35**

## 1. Code Status (DONE)

### File: `src/game/GameInitOptionsScreen.cc`
- **Vấn đề cũ:** Khi xuất hiện popup thông báo (như xác nhận độ khó), hai bên lề màn hình wide bị đen xì do `guiSAVEBUFFER` chỉ chụp vùng `640x439` ở giữa.
- **Sửa đổi:** Thay đổi `BlitBufferToBuffer(FRAME_BUFFER, guiSAVEBUFFER, STD_SCREEN_X, STD_SCREEN_Y, 640, 439);` thành `BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);` để chụp lưu trữ trọn vẹn toàn bộ màn hình thực tế (kèm lề stretch).

### File: `src/game/MainMenuScreen.cc`
- **Vấn đề cũ:** Nền rừng màn hình Main Menu bị hai khoảng đen (letterbox) ở hai bên lề khi chạy ở độ phân giải rộng (1664x768).
- **Sửa đổi:** Áp dụng công thức stretch ảnh nền giống GIO Screen.
  - Sử dụng `BltStretchVideoSurface` thông qua một 16bpp temp surface (`SGPVSurface tmp`) để phóng to ảnh nền `mainmenubackground*.sti` tràn ngập toàn bộ màn hình thực tế (`SCREEN_WIDTH` x `SCREEN_HEIGHT`).
  - Giữ nguyên tọa độ logo Jagged Alliance Wildfire vẽ đè lên trên ở vị trí chính giữa màn hình (`STD_SCREEN_X + 188`).

## 2. Build & Smoke Test Status (SUCCESS)
- Đã chạy `clean` cache CMake/Gradle.
- Đã build và cập nhật thành công lên máy ảo Android `Pronunciation_API_35`.
- **Kết quả:**
  - Màn hình Main Menu kéo dãn đầy đủ 100% không còn viền đen. Logo Wildfire hiển thị rõ nét, cân đối ở giữa.
  - Chuyển sang New Game -> GIO screen, bấm chỉnh các tùy chọn có popup thông báo đè lên, nền rừng cây hai bên lề vẫn giữ nguyên, không bị sập đen.

## 3. Cách chạy lại Máy ảo & Build nhanh sau này
Mở cửa sổ Terminal thứ 1 (Chạy máy ảo):
```bash
export ANDROID_HOME="$HOME/Library/Android/sdk"
~/Library/Android/sdk/emulator/emulator -avd Pronunciation_API_35 -netdelay none -netspeed full
```

Mở cửa sổ Terminal thứ 2 (Build & Cài đặt):
```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"

# (Chỉ cần chạy lệnh dưới nếu sửa code C++ mà Gradle không nhận)
./android/gradlew -p android clean 

./tools/build-android-debug.sh
~/Library/Android/sdk/platform-tools/adb install -r android/app/build/outputs/apk/debug/app-debug.apk
~/Library/Android/sdk/platform-tools/adb shell am force-stop io.github.ja2stracciatella
~/Library/Android/sdk/platform-tools/adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```
