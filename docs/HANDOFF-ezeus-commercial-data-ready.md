# Handoff — eZeus: commercial data đã có, tiếp tục first frame

**Ngày:** 2026-08-17  
**Nhánh:** `feature/multi-edition-detector`  
**Trạng thái:** Support/commercial data đã chuẩn bị và push đúng emulator; native eZeus khởi tạo SDL thành công nhưng process thoát sạch trước khi giữ menu frame. Chi tiết mới: `docs/HANDOFF-ezeus-first-frame-support-loaded.md`.

## 1. Kết luận quan trọng

`android/Zeus + Poseidon/` **là commercial game data thật**, không phải thư mục rỗng hay source giả. Đã kiểm tra có:

```text
DATA/
Adventures/
Audio/
Model/
Save/
zeus.exe
Zeus_Text.eng
Zeus_MM.eng
```

Kích thước đã đo:

```text
Toàn bộ:   khoảng 769 MB
DATA:      khoảng 370 MB
Audio:     khoảng 266 MB
Adventures: khoảng 62 MB
```

Phát hiện này sửa kết luận cũ trong `HANDOFF-oldgame-launcher-ezeus-dropdown-smoke.md`: blocker hiện tại **không còn là thiếu commercial data**. Blocker là eZeus support/generated data.

## 2. File còn thiếu

Chưa tìm thấy trong commercial install hoặc `/tmp/eZeus`:

```text
interface.e
một texture pack: i15.e hoặc i30.e hoặc i45.e hoặc i60.e
Zeus_Text.xml
Zeus_MM.xml
```

Hai file `.eng` gốc đã có:

```text
android/Zeus + Poseidon/Zeus_Text.eng
android/Zeus + Poseidon/Zeus_MM.eng
```

## 3. Nguồn support files

### eZeus release chính

Release chính thức:

<https://github.com/MaurycyLiebner/eZeus/releases/tag/0.8.2-beta>

Package:

<https://github.com/MaurycyLiebner/eZeus/releases/download/0.8.2-beta/eZeus-0.8.2-beta.zip>

Tải package, giải nén ở thư mục tạm, kiểm tra và lấy các file support cần thiết. Không đưa package hoặc commercial data vào Git repository/APK.

### Texture pack riêng

Release `0.8.2-beta` có asset trực tiếp:

- `i45.e`: <https://github.com/MaurycyLiebner/eZeus/releases/download/0.8.2-beta/i45.e>
- `i60.e`: <https://github.com/MaurycyLiebner/eZeus/releases/download/0.8.2-beta/i60.e>

Prototype nên bắt đầu với **một** pack. Ưu tiên `i45.e` nếu package không có `i30.e`; không tải cả hai khi chưa đo RAM/GPU.

### XML

Có thể dùng XML upstream:

- `Zeus_Text.xml`: <https://github.com/MacThings/eZeus/releases/download/Zeus/Zeus_Text.xml>
- `Zeus_MM.xml`: <https://github.com/MacThings/eZeus/releases/download/Zeus/Zeus_MM.xml>

Hoặc dùng `engconverter-0.5.exe` trong eZeus Windows package để convert:

```text
Zeus_Text.eng → Zeus_Text.xml
Zeus_MM.eng   → Zeus_MM.xml
```

Release hướng dẫn đặt hai XML cạnh eZeus executable/support root.

## 4. Root Android hiện tại

`EZeusActivity` đang truyền ba root cho native `SDL_main`:

```text
commercial root:
/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial

support root:
/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support

writable root:
/data/user/0/io.github.ja2stracciatella/files/ezeus
```

Ý nghĩa:

- `commercial`: copy toàn bộ nội dung `android/Zeus + Poseidon/`; không ghi.
- `support`: `interface.e`, `i*.e`, XML, `Text/`, `Fonts/`, và support directories cần thiết.
- `writable`: `settings`, `numbers`, `Save`, log.

Cấu trúc support tối thiểu:

```text
support/
├── interface.e
├── i45.e                         # hoặc i30/i15/i60
├── Zeus_Text.xml
├── Zeus_MM.xml
├── Text/language.txt
└── Fonts/Zeus.ttf
```

`Text/language.txt` và `Fonts/Zeus.ttf` có trong `/tmp/eZeus`; copy vào support root khi chuẩn bị emulator. Nếu runtime yêu cầu, copy thêm `Textures/`, `Sanctuaries/`, `Adventures/` từ source support.

## 5. Việc cần làm tiếp theo

### Bước A — Chuẩn bị local data, không commit

1. Tải `eZeus-0.8.2-beta.zip` vào thư mục ngoài repo.
2. Extract vào thư mục tạm.
3. Xác định `interface.e` và texture pack có trong package.
4. Nếu package không có pack phù hợp, tải `i45.e` riêng.
5. Lấy hai XML upstream hoặc convert từ `.eng`.
6. Tạo thư mục local, ví dụ:

```text
/tmp/ezeus-support/
/tmp/ezeus-commercial/
```

7. Copy commercial install vào `/tmp/ezeus-commercial/`.
8. Copy support files vào `/tmp/ezeus-support/`.

Không stage:

```text
android/Zeus + Poseidon/
/tmp/ezeus-support/
/tmp/ezeus-commercial/
```

### Bước B — Nạp lên emulator

Dùng `adb push` vào đúng roots:

```bash
ADB="$HOME/Library/Android/sdk/platform-tools/adb"

"$ADB" shell mkdir -p \
  /sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/commercial \
  /sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support

"$ADB" push "/tmp/ezeus-commercial/." \
  /sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/commercial/

"$ADB" push "/tmp/ezeus-support/." \
  /sdcard/Android/data/io.github.ja2stracciatella/files/ezeus/support/
```

Nếu Android không cho ghi trực tiếp vào `/sdcard/Android/data`, dùng app-private staging hoặc implement Storage Access Framework/import flow. Không sửa path native để né sandbox.

### Bước C — Build và launch

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella" || exit 1

PATH="$HOME/Library/Android/sdk/cmake/3.22.1/bin:$PATH" \
./android/gradlew -p android :app:assembleDebug \
-PezeusSourceDir=/tmp/eZeus
```

Cài APK:

```bash
"$HOME/Library/Android/sdk/platform-tools/adb" install -r \
  android/app/build/outputs/apk/debug/app-debug.apk
```

Launch qua `LauncherActivity`, chọn `Zeus`, nhấn FAB. Không launch trực tiếp `EZeusActivity`; activity không exported theo thiết kế.

Kiểm tra log:

```bash
adb logcat -c
adb logcat -v brief | grep -E 'eZeus|FATAL EXCEPTION|UnsatisfiedLinkError|dlopen failed'
```

Expected: không còn:

```text
Missing support file: .../support/interface.e
```

Expected next milestone: SDL window tạo được và first visible menu frame.

## 6. Bằng chứng hiện tại

Đã pass:

- full eZeus native target build;
- build không eZeus;
- `libezeus.so` trong APK arm64-v8a;
- launcher process và `:ezeus` process cùng tồn tại;
- `SDL_main argc=4`;
- ba root truyền đúng;
- fail-fast khi thiếu `interface.e`;
- không `UnsatisfiedLinkError`;
- không `dlopen failed`;
- không `FATAL EXCEPTION`.

Log cũ:

```text
I eZeus: SDL_main full entrypoint argc=4
I eZeus: EZEUS_COMMERCIAL_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/commercial
I eZeus: EZEUS_SUPPORT_ROOT=/storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support
I eZeus: EZEUS_WRITABLE_ROOT=/data/user/0/io.github.ja2stracciatella/files/ezeus
E eZeus: Missing support file: /storage/emulated/0/Android/data/io.github.ja2stracciatella/files/ezeus/support/interface.e
```

## 7. Ràng buộc

- Không bundle original Zeus/Poseidon data vào APK.
- Không commit `android/Zeus + Poseidon/`.
- Không commit downloaded `.e`/XML support files nếu provenance/license chưa được review.
- Không commit/push nếu chưa được yêu cầu.
- Giữ nguyên thay đổi không thuộc task, đặc biệt `src/game/Tactical/Interface_Items.cc` và `docs/HANDOFF-android-inventory-pants-magazine.md`.
- Tuân `CLAUDE.md`: không đọc `android/build.log`; không dump toàn bộ CMake output.

## 8. Sau first frame

Chỉ làm sau khi menu đã render:

1. xác nhận settings/numbers/Save ghi đúng writable root;
2. xác nhận audio init;
3. touch/right-click/wheel;
4. Home/resume và renderer reset;
5. start adventure và save/load;
6. importer UI/Storage Access Framework nếu adb không đủ cho người dùng cuối;
7. license/provenance review trước release.

**Mục tiêu session tiếp theo:** lấy support files từ release hợp pháp, nạp ba roots, đạt first visible eZeus menu frame. Không viết lại engine/game data.
