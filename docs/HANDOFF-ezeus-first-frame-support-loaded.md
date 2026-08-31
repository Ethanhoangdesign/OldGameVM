# Handoff — eZeus support loaded, startup exits after SDL init

**Ngày:** 2026-08-17  
**Nhánh:** `feature/multi-edition-detector`  
**Trạng thái:** Commercial/support data đã nạp đúng emulator; native eZeus khởi tạo SDL thành công; chưa giữ được menu frame. Không commit/push.

## 1. Đã hoàn tất

### Local data ngoài repo

Đã tải:

```text
/tmp/eZeus-0.8.2-beta.zip       # khoảng 323 MB
/tmp/ezeus-release/
```

Đã tạo:

```text
/tmp/ezeus-commercial/
/tmp/ezeus-support/
```

Commercial copy:

```text
android/Zeus + Poseidon/ → /tmp/ezeus-commercial/
```

Support files:

```text
/tmp/ezeus-support/interface.e       # khoảng 81 MB
/tmp/ezeus-support/i30.e              # khoảng 143 MB
/tmp/ezeus-support/Zeus_Text.xml      # khoảng 526 KB
/tmp/ezeus-support/Zeus_MM.xml        # khoảng 669 KB
/tmp/ezeus-support/Text/language.txt
/tmp/ezeus-support/Fonts/Zeus.ttf
```

XML lấy từ upstream:

```text
https://github.com/MacThings/eZeus/releases/download/Zeus/Zeus_Text.xml
https://github.com/MacThings/eZeus/releases/download/Zeus/Zeus_MM.xml
```

Không đưa commercial data/support files vào Git/APK.

### Emulator

AVD:

```text
Pronunciation_API_35
```

Device:

```text
emulator-5554    device
```

Commercial push đã pass:

```text
2050 files
801958197 bytes
```

Support push từng file đã pass. Remote listing xác nhận:

```text
/sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support/interface.e
/sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support/i30.e
/sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support/Zeus_Text.xml
/sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support/Zeus_MM.xml
/sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support/Text/language.txt
/sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support/Fonts/Zeus.ttf
```

## 2. Build

Build eZeus đã pass nhiều lần:

```text
BUILD SUCCESSFUL in 2m 12s
```

Lệnh dùng:

```bash
PATH="$HOME/Library/Android/sdk/cmake/3.22.1/bin:$PATH" \
./android/gradlew -p android clean :app:assembleDebug \
  -PezeusSourceDir=/tmp/eZeus
```

APK install pass:

```text
Performing Streamed Install
Success
```

Cảnh báo C++ chỉ là warning upstream, không chặn build.

Lưu ý zsh: không dùng biến tên `status`; zsh báo:

```text
zsh: read-only variable: status
```

Dùng `rc` hoặc `build_rc`.

## 3. Launcher/SDL evidence

Launcher mở được. Chọn `Zeus`, nhấn FAB, `EZeusActivity` chạy trong process SDL.

Log thành công:

```text
V/SDL: Running main function SDL_main from library .../libezeus.so
V/SDL: nativeRunMain()
I/eZeus: SDL_main full entrypoint argc=4
I/eZeus: EZEUS_COMMERCIAL_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial
I/eZeus: EZEUS_SUPPORT_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support
I/eZeus: EZEUS_WRITABLE_ROOT=/data/user/0/io.github.ja2stracciatella/files/ezeus
V/SDL: eZeus locked landscape w=1280 h=720 resizable=false hint=
I/SDL/APP: pixel format wanted SDL_PIXELFORMAT_RGBX8888 (2), got SDL_PIXELFORMAT_RGBX8888 (2)
V/SDL: surfaceChanged()
V/SDL: Window size: 2264x1080
```

Sau khi push đúng support, lỗi cũ đã biến mất:

```text
Missing support file: .../support/interface.e
```

Không có:

```text
UnsatisfiedLinkError
dlopen failed
FATAL EXCEPTION
Fatal signal
```

## 4. Hiện tượng còn lại

Sau SDL init, process kết thúc sạch:

```text
V/SDL: Finished main function
V/SDL: onPause()
V/SDL: surfaceDestroyed()
V/SDL: onStop()
V/SDL: onDestroy()
V/SDL: SDLActivity thread ends
```

Launcher quay lại. `dumpsys`:

```text
ResumedActivity: .../.LauncherActivity
mLastPausedActivity: .../.EZeusActivity
```

Không còn missing support file. Vì vậy blocker hiện tại nằm sau `configureRoots()` và SDL setup, gần một trong các bước:

```cpp
settings.read();
eMusic/eSounds construction;
eMainWindow::initialize(settings);
eGameTextures::initialize(window.renderer());
window.exec();
```

## 5. Display/scaling work đã thử

First frame trước đó đã render được nhưng nhỏ giữa nền đen:

```text
SDL surface: 2264x1080
SDL logical/game canvas: 1280x720
```

Đã thử:

```cpp
settings.fFullscreen = true;
SDL_SetWindowFullscreen(window.window(), SDL_WINDOW_FULLSCREEN_DESKTOP);
SDL_RenderSetLogicalSize(window.renderer(), settings.fRes.width(), settings.fRes.height());
```

`SDL_SetWindowFullscreen(...DESKTOP)` gây lifecycle/surface transition không ổn định nên đã bỏ. Hiện code giữ lại:

```cpp
#if defined(__ANDROID__)
    SDL_RenderSetLogicalSize(
        window.renderer(),
        settings.fRes.width(),
        settings.fRes.height()
    );
#endif
```

Vị trí: `android/ezeus/ezeus_android_main.cpp`, ngay sau `eMainWindow::initialize(settings)`.

Không nên tiếp tục đổi fullscreen trước khi xác định exit point.

## 6. Phân tích tiếp theo nên làm

Thêm `SDL_Log`/Android log marker quanh từng bước sau, build lại, rồi xác định marker cuối cùng:

```cpp
log("after settings.read");
log("before eMusic/eSounds");
log("before eMainWindow::initialize");
log("after eMainWindow::initialize");
log("before eGameTextures::initialize");
log("after eGameTextures::initialize");
log("before window.exec");
```

Đặc biệt chú ý:

- `eSettings::read()` có thể đọc `settings.txt` từ writable root chưa tồn tại;
- `eMusic`/`eSounds` có thể fail khi audio asset path chưa map đúng;
- `eMainWindow::initialize()` tạo window theo `settings.fRes`;
- `eGameTextures::initialize()` hiện chỉ kiểm tra `commercial/DATA` và tạo loaders;
- `window.exec()` bắt đầu bằng `showMenuLoading()`, không render main menu ngay lập tức;
- `eMenuLoadingWidget` load interface textures + music + sounds + language trước menu.

Có thể cần thêm logging trong `eMainWindow::exec()` và `eMenuLoadingWidget` để biết menu loader có ném exception/return sớm hay không. Không nuốt lỗi.

## 7. Lệnh reproduce hiện tại

```bash
ADB="$HOME/Library/Android/sdk/platform-tools/adb"

"$ADB" logcat -c
"$ADB" shell am force-stop io.github.ja2stracciatella
"$ADB" shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Chọn `Zeus`, nhấn FAB, rồi:

```bash
sleep 5
"$ADB" logcat -d -v brief | grep -E \
  'eZeus|SDL|Missing|FATAL|Fatal signal|SIG|failed' | tail -150
```

## 8. Ràng buộc

- Không commit/push nếu chưa được yêu cầu.
- Không stage `android/Zeus + Poseidon/`.
- Không bundle commercial data vào APK.
- Không commit `/tmp/ezeus-support` hoặc `/tmp/ezeus-commercial`.
- Không đọc `android/build.log`.
- Giữ nguyên thay đổi không thuộc task, đặc biệt:
  - `src/game/Tactical/Interface_Items.cc`
  - `docs/HANDOFF-android-inventory-pants-magazine.md`

**Mốc hiện tại:** roots đúng, support files đúng, native SDL entrypoint đúng, process thoát sạch sau init; cần instrument native startup để tìm điểm return trước khi tiếp tục fullscreen/scaling.
