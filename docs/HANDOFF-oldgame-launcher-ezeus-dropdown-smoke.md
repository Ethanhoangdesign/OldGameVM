# Handoff — OldGame launcher + full eZeus Android compile-first

**Ngày:** 2026-08-17  
**Nhánh:** `feature/multi-edition-detector`  
**Trạng thái:** dropdown hoàn thành; full eZeus native target build và load trên emulator; chưa có first menu frame vì thiếu support files  
**Commit/push:** chưa thực hiện

## 1. Kết quả hiện tại

Android launcher có hai lựa chọn:

- `JA`
- `Zeus`

Hành vi đã giữ nguyên:

- Cài mới mặc định JA.
- Lựa chọn lưu qua restart/rotation.
- JA có bốn tab Data, Settings, Controller / Gamepad, Logs.
- Zeus chỉ có Status.
- JA Play giữ flow cũ.
- Zeus Play guard `libezeus.so`, mở `EZeusActivity` trong process `:ezeus`.
- APK không build eZeus vẫn chạy; Zeus Play báo thiếu library thay vì crash.

## 2. Nâng cấp từ smoke lên full source

Target cũ chỉ compile:

- `android/ezeus/ezeus_android_smoke.cpp`
- `/tmp/eZeus/erand.cpp`

Target mới compile full source manifest từ `/tmp/eZeus/CMakeLists.txt`:

- khoảng 695 file `.cpp` upstream;
- loại `main.cpp` để tránh duplicate entrypoint;
- dùng `android/ezeus/ezeus_android_main.cpp` làm `SDL_main` Android;
- dùng `android/ezeus/egamedir.cpp` làm Android path overlay;
- link SDL2, SDL2_image, SDL2_ttf, SDL2_mixer.

Build vẫn opt-in:

```bash
-PezeusSourceDir=/tmp/eZeus
```

Gradle map property thành:

```text
BUILD_EZEUS=ON
EZEUS_SOURCE_DIR=/tmp/eZeus
```

`EZEUS_SOURCE_DIR` là source checkout. Không trỏ tới `android/Zeus + Poseidon/`.

## 3. Source/dependency integration

### `cmake/ezeus.cmake`

- Validate `EZEUS_SOURCE_DIR`.
- Parse source list tường minh từ upstream `add_executable(eZeus ...)`.
- Không glob source.
- Download release archives có SHA-256:
  - SDL_ttf 2.24.0;
  - SDL_image 2.8.12;
  - SDL_mixer 2.8.2.
- Tắt codec/format không cần thiết.
- Build companion SDL libraries shared trên Android.
- Tạo header overlay `SDL2/SDL_*.h` vì eZeus dùng include layout này.
- Force include `<vector>` và `<cmath>` để bù include chuẩn upstream còn thiếu.

### SDL2 target visibility

`dependencies/lib-sdl2/CMakeLists.txt` đánh dấu imported targets sau là global:

- `SDL2::SDL2`
- `SDL2::SDL2-static`
- `SDL2::SDL2main`

Việc này cho phép các FetchContent subdirectory của SDL_image/ttf/mixer reuse SDL2 đã build, không tạo malformed linker dependency `sdl2-internal`.

### libnoise / map generator

Android hiện không build phần map generator phụ thuộc libnoise:

- `engine/emapgenerator.cpp`
- `engine/emapgenerator.h`
- `widgets/eboardsettingsmenu.cpp`

CMake tạo overlay, đổi guard thành:

```cpp
#if defined(__unix__) && !defined(__ANDROID__)
```

Đây là giới hạn có chủ ý cho compile-first. Startup, campaign có sẵn, save/load không cần map generator. Khi cần New Map trên Android, thêm libnoise native target rồi bỏ overlay guard.

## 4. Android entrypoint và ba root

`EZeusActivity.getArguments()` truyền ba argument cho `SDL_main`:

1. commercial root;
2. support root;
3. writable root.

Paths hiện tại trên emulator:

```text
EZEUS_COMMERCIAL_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial
EZEUS_SUPPORT_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support
EZEUS_WRITABLE_ROOT=/data/user/0/io.github.ja2stracciatella/files/ezeus
```

Phân quyền:

- commercial: dữ liệu game read-only về mặt thiết kế;
- support: file eZeus riêng;
- writable: settings, numbers, Save, log tương lai.

`android/ezeus/egamedir.cpp` map API upstream:

- `path(...)` → commercial root;
- `iBinaryPath()` và `i15/i30/i45/i60` → support root;
- font/XML/Text/Textures/Sanctuaries → support root qua `exeDir()` compatibility path;
- `settingsPath()`, `numbersPath()`, `saveDir()` → writable root;
- unpacked support Adventures → support root;
- commercial `.pak` Adventures → commercial root.

## 5. Fail-fast validation

`SDL_main` validate trước khi khởi tạo game loop:

- commercial root tồn tại;
- writable `Save/` tạo được;
- `interface.e` tồn tại;
- ít nhất một texture pack tồn tại:
  - `i15.e`, hoặc
  - `i30.e`, hoặc
  - `i45.e`, hoặc
  - `i60.e`;
- `Zeus_Text.xml`;
- `Zeus_MM.xml`;
- `Text/language.txt`;
- `Fonts/Zeus.ttf`.

Thiếu file sẽ ghi `SDL_Log` rõ path rồi return sạch. Không crash process Java.

## 6. Dữ liệu hiện có

### Commercial data

`android/Zeus + Poseidon/` là bản cài game thương mại Windows:

- khoảng 2.050 files;
- khoảng 802 MB;
- `DATA`, `Adventures`, `Audio`, `Model`, `Save`;
- `zeus.exe`, DLL/ASI/M3D, BINK/MP3, manuals.

Nó:

- không phải eZeus source;
- không phải JA2 Data;
- không được bundle vào APK;
- không được Gradle copy;
- không được commit/stage;
- hiện vẫn untracked.

### eZeus source

Source checkout hiện tại:

```text
/tmp/eZeus
```

Source này có:

- full C++ engine;
- `Text/language.txt`;
- `Fonts/Zeus.ttf`;
- source-side Adventures/Textures.

### Support files còn thiếu

Không tìm thấy trong commercial data hoặc `/tmp/eZeus`:

```text
interface.e
i30.e              # hoặc i15/i45/i60
Zeus_Text.xml
Zeus_MM.xml
```

Vì vậy first menu frame chưa thể chạy.

## 7. Build đã xác nhận

Full eZeus:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella" || exit 1

PATH="$HOME/Library/Android/sdk/cmake/3.22.1/bin:$PATH" \
./android/gradlew -p android :app:assembleDebug \
-PezeusSourceDir=/tmp/eZeus
```

Kết quả:

```text
BUILD SUCCESSFUL
```

Build không eZeus:

```bash
PATH="$HOME/Library/Android/sdk/cmake/3.22.1/bin:$PATH" \
./android/gradlew -p android :app:assembleDebug
```

Kết quả:

```text
BUILD SUCCESSFUL
```

Sau verification cuối, APK được build lại với eZeus.

## 8. APK native libraries

APK full hiện chứa arm64-v8a:

```text
libSDL2.so
libSDL2_image.so
libSDL2_mixer.so
libSDL2_ttf.so
libezeus.so
libja2.so
liblua.so
```

`libezeus.so` debug khoảng 48 MB.

APK:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

## 9. Emulator smoke đã xác nhận

AVD:

```text
Pronunciation_API_35
```

Cài APK:

```bash
ADB="$HOME/Library/Android/sdk/platform-tools/adb"
"$ADB" install -r android/app/build/outputs/apk/debug/app-debug.apk
```

`EZeusActivity` không exported, nên command sau bị Android từ chối đúng thiết kế:

```bash
adb shell am start -n io.github.ja2stracciatella/.EZeusActivity
```

Phải mở `LauncherActivity`, chọn Zeus, nhấn FAB.

Log thực tế:

```text
I eZeus: SDL_main full entrypoint argc=4
I eZeus: EZEUS_COMMERCIAL_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial
I eZeus: EZEUS_SUPPORT_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support
I eZeus: EZEUS_WRITABLE_ROOT=/data/user/0/io.github.ja2stracciatella/files/ezeus
E eZeus: Missing support file: /storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support/interface.e
```

Đã xác nhận:

- launcher process và `:ezeus` process cùng tồn tại;
- `SDL_main` nhận đúng 4 args;
- ba root đúng;
- fail-fast đúng file thiếu đầu tiên;
- không có `UnsatisfiedLinkError`;
- không có `dlopen failed`;
- không có `FATAL EXCEPTION`.

## 10. Files thêm/sửa trong phase này

### Native/build

- `CMakeLists.txt`
  - đổi mô tả option từ smoke thành Android target.
- `cmake/ezeus.cmake`
  - full source manifest, companion SDL dependencies, overlays.
- `dependencies/lib-sdl2/CMakeLists.txt`
  - imported SDL targets global.
- `android/ezeus/ezeus_android_main.cpp`
  - full Android `SDL_main`, root validation, game startup.
- `android/ezeus/egamedir.cpp`
  - three-root path overlay.
- `android/ezeus/ezeus_android_smoke.cpp`
  - file cũ còn trong working tree nhưng không còn được target compile; có thể xóa ở cleanup sau khi xác nhận không cần lịch sử local.

### Android

- `android/app/src/main/java/io/github/ja2stracciatella/EZeusActivity.kt`
  - tạo/truyền commercial, support, writable roots.

Launcher/dropdown files từ phase trước vẫn chưa commit.

## 11. Bước tiếp theo

1. Cung cấp hợp pháp support files:
   - `interface.e`;
   - một `i*.e` texture pack;
   - `Zeus_Text.xml`;
   - `Zeus_MM.xml`.
2. Copy vào support root trên emulator.
3. Copy commercial install vào commercial root, ngoài APK.
4. Copy source support directories cần thiết từ `/tmp/eZeus`:
   - `Text/`;
   - `Fonts/`;
   - `Textures/` nếu runtime dùng unpacked textures;
   - `Sanctuaries/`;
   - support Adventures nếu cần.
5. Launch qua launcher tới first visible menu frame.
6. Xác nhận settings/numbers/Save chỉ ghi vào writable root.
7. Sau first-frame mới làm:
   - import UI/Storage Access Framework;
   - touch/right-click/wheel;
   - audio completeness;
   - Home/Resume;
   - save/load;
   - Android libnoise map generation.

## 12. Working-tree safety

Không commit/push nếu chưa được yêu cầu.

Giữ nguyên thay đổi không thuộc task:

- `src/game/Tactical/Interface_Items.cc`;
- `docs/HANDOFF-android-inventory-pants-magazine.md`.

Trước commit tương lai:

- stage từng file;
- tuyệt đối loại `android/Zeus + Poseidon/`;
- kiểm tra `git diff --check`;
- build cả có và không có `-PezeusSourceDir`.
