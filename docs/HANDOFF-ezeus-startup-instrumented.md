# Handoff — eZeus startup đã instrument, chờ marker runtime

## Trạng thái

- Nhánh: `feature/multi-edition-detector`
- Commercial data thật đã có ngoài APK/Git:
  - local: `/tmp/ezeus-commercial/`
  - emulator: `/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial`
- Support data đã push ngoài APK/Git:
  - `/tmp/ezeus-support/`
  - emulator: `/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support`
- APK build/install đã pass.
- `libezeus.so` load thành công.
- SDL init, Android window creation đã pass.
- First visible menu frame chưa giữ được; process từng thoát sạch sau SDL setup.
- Chưa commit, push, stage, hoặc bundle data.

## Roots runtime

```text
commercial:
/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial

support:
/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support

writable:
/data/user/0/io.github.ja2stracciatella/files/ezeus
```

Support tối thiểu đã có:

```text
interface.e
i30.e
Zeus_Text.xml
Zeus_MM.xml
Text/language.txt
Fonts/Zeus.ttf
```

## Native instrumentation

File:

```text
android/ezeus/ezeus_android_main.cpp
```

Marker hiện có:

```text
SDL_main full entrypoint argc=4
after configureRoots
after initializeSDL
after settings.read
after texture-pack filtering
before audio construction
after audio construction
before window.initialize
after window.initialize
before eGameTextures::initialize
after eGameTextures::initialize
before window.exec
after window.exec result=%d
```

Texture initialization failure đã return nonzero rõ ràng. Không thêm `showMenuLoading()` thủ công.

## Kết luận source

`eMainWindow::exec()` tự gọi:

```cpp
showMenuLoading();
```

First-frame loading bắt đầu trong event loop, gồm:

- interface texture loading từ `.e`;
- menu music/button sound setup;
- XML/language loading;
- renderer presentation.

`eGameTextures::initialize()` chỉ kiểm tra commercial `DATA` và tạo loader; chưa load ảnh first frame.

## Việc tiếp theo

1. Build APK hiện tại nếu cần.
2. Install APK.
3. Launch qua `LauncherActivity`, chọn `Zeus`, nhấn FAB.
4. Lấy log:

```bash
ADB="$HOME/Library/Android/sdk/platform-tools/adb"
"$ADB" logcat -c
"$ADB" shell am force-stop io.github.ja2stracciatella
"$ADB" shell am start -n io.github.ja2stracciatella/.LauncherActivity
sleep 5
"$ADB" logcat -d -v brief | grep -E \
  'eZeus|SDL|Missing|FATAL|Fatal signal|SIG|failed' | tail -150
```

## Cách đọc marker

- Thiếu `before window.exec`: lỗi ở bước trước; chỉ sửa bước đó.
- Có `before window.exec` và `after window.exec`: event loop return; điều tra `SDL_QUIT`/lifecycle.
- Có `before window.exec`, thiếu marker return: lỗi/hang trong first-frame loader, renderer, hoặc native crash.
- `Finished main function` nghĩa là `SDL_main()` đã return; không chỉ là SDL init xong.

## Không làm

- Không thêm duplicate `showMenuLoading()`.
- Không tiếp tục thử fullscreen trước khi event loop sống ổn định.
- Không rewrite path `support/bin/..`; native eZeus dùng filesystem bình thường.
- Không bundle commercial/support data vào APK.
- Không commit `android/Zeus + Poseidon/`.
- Không commit `.e`/XML downloaded khi chưa review provenance/license.
- Không commit/push nếu chưa được yêu cầu.
- Không sửa thay đổi ngoài task, đặc biệt:
  - `src/game/Tactical/Interface_Items.cc`
  - `docs/HANDOFF-android-inventory-pants-magazine.md`
- Không đọc `android/build.log`, `*.bak`, hoặc `*.ogvm-bak`.

## Mốc bàn giao

Chưa chọn runtime fix vì chưa có fresh marker sequence từ emulator. Cần xác định marker cuối trước khi sửa source.
