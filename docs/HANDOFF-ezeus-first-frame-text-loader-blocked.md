# Handoff — eZeus first frame có texture, text loader bị chặn

**Ngày:** 2026-08-18  
**Nhánh:** `feature/multi-edition-detector`  
**Trạng thái:** chưa commit/push

## Kết luận

Android Emulator đã chạy được eZeus tới first frame:

- background commercial render được;
- interface texture render được;
- SDL window sống;
- `window.exec()` chưa return;
- menu text chưa xuất hiện;
- screenshot hiện tại chỉ có background + khung xanh, không có chữ.

Đây **không còn là lỗi build, library load, SDL init, commercial data, hoặc thiếu texture**.

## Emulator

```text
AVD: Pronunciation_API_35
Device: emulator-5554
Model: sdk_gphone64_arm64
```

Root runtime:

```text
commercial:
/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial

support:
/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support

writable:
/data/user/0/io.github.ja2stracciatella/files/ezeus
```

Support files đã push:

```text
support/interface.e
support/i30.e
support/Zeus_Text.xml
support/Zeus_MM.xml
support/Text/language.txt
support/Fonts/Zeus.ttf
```

## Fresh runtime evidence

```text
I/eZeus: SDL_main full entrypoint argc=4
I/eZeus: after configureRoots
I/eZeus: after initializeSDL
I/eZeus: after settings.read
I/eZeus: after texture-pack filtering
I/eZeus: before audio construction
I/eZeus: after audio construction
I/eZeus: before window.initialize
I/eZeus: after window.initialize
I/eZeus: before eGameTextures::initialize
I/eZeus: after eGameTextures::initialize
I/eZeus: before window.exec
```

Không thấy:

```text
after window.exec result=0
Finished main function
FATAL EXCEPTION
Fatal signal
UnsatisfiedLinkError
dlopen failed
```

## Đường dẫn text đã xác minh

Upstream `elanguage.cpp` dùng:

```cpp
eGameDir::exeDir() + "../Zeus_Text.xml"
eGameDir::exeDir() + "../Zeus_MM.xml"
eGameDir::exeDir() + "../Text/language.txt"
```

Android overlay `eGameDir::exeDir()` trả về support root dạng `.../support/bin/`, nên các path trên resolve thành:

```text
.../support/Zeus_Text.xml
.../support/Zeus_MM.xml
.../support/Text/language.txt
```

Vì vậy support root hiện tại đúng. Không cần duplicate file sang commercial root.

## Điểm block chính xác

`eMainWindow::exec()` gọi:

```cpp
showMenuLoading();
```

`eMenuLoadingWidget` gọi loader:

```cpp
const bool r = eGameTextures::loadNextMenu(sett, text);
if(r) {
    text = "Loading music...";
    eMusic::loadMenu();
    eSounds::loadButtonSound();
    eLanguage::load();
    return true;
}
```

`eLoadingWidget::initialize()` tạo text/progress widget nhưng sau đó gọi:

```cpp
mPB->hide();
mLabelW->hide();
```

Trong trạng thái hiện tại, loader đang còn chạy hoặc mắc trong một bước của `loadNextMenu()` / `eMusic::loadMenu()` / `eSounds::loadButtonSound()` / `eLanguage::load()`. Vì label bị hide khi chờ, màn hình chỉ còn texture background.

## Việc tiếp theo

Thêm marker trong upstream source local `/tmp/eZeus` tại `widgets/emenuloadingwidget.cpp`:

```cpp
SDL_Log("eZeus menu loader: before loadNextMenu");
const bool r = eGameTextures::loadNextMenu(sett, text);
SDL_Log("eZeus menu loader: after loadNextMenu r=%d", r);
if(r) {
    text = "Loading music...";
    SDL_Log("eZeus menu loader: before eMusic::loadMenu");
    eMusic::loadMenu();
    SDL_Log("eZeus menu loader: after eMusic::loadMenu");
    eSounds::loadButtonSound();
    SDL_Log("eZeus menu loader: after eSounds::loadButtonSound");
    eLanguage::load();
    SDL_Log("eZeus menu loader: after eLanguage::load");
    return true;
}
```

Build lại với:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella"
PATH="$HOME/Library/Android/sdk/cmake/3.22.1/bin:$PATH" \
./android/gradlew -p android :app:assembleDebug \
  -PezeusSourceDir=/tmp/eZeus \
  -PdebugSigning
```

Cài/chạy:

```bash
ADB="$HOME/Library/Android/sdk/platform-tools/adb"
"$ADB" install -r -d android/app/build/outputs/apk/debug/app-debug.apk
"$ADB" logcat -c
"$ADB" shell am force-stop io.github.ja2stracciatella
"$ADB" shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Chọn `Zeus`, nhấn FAB, lấy marker:

```bash
sleep 10
"$ADB" logcat -d -v brief | grep -E \
  'eZeus|SDL|Missing|FATAL|Fatal signal|SIG|failed|Finished main' \
  | tail -200
```

## Cách đọc kết quả

- Có `before loadNextMenu`, thiếu `after loadNextMenu`: block trong menu texture loader.
- Có `after loadNextMenu r=1`, thiếu `after eMusic::loadMenu`: block trong menu music loading.
- Có `after eMusic::loadMenu`, thiếu `after eSounds::loadButtonSound`: block trong button sound loading.
- Có `after eSounds::loadButtonSound`, thiếu `after eLanguage::load`: block trong XML/language loading.
- Có đủ `after eLanguage::load` nhưng vẫn không có main menu: kiểm tra `eLoadingWidget::paintEvent()` và `mDoneAction`/render transition.

## Ràng buộc

- Không commit/push nếu chưa được yêu cầu.
- Không stage `android/Zeus + Poseidon/`.
- Không bundle commercial/support data vào APK.
- Không duplicate support files sang commercial root.
- Không sửa `src/game/Tactical/Interface_Items.cc` hoặc handoff inventory.
- Không đọc `android/build.log`, `*.bak`, `*.ogvm-bak`.
