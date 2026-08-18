# OldGame Launcher + eZeus — Handoff tổng hợp

**Ngày:** 2026-08-17  
**Nhánh khảo sát:** `feature/multi-edition-detector`  
**Nền tảng đích:** Android, trước mắt `arm64-v8a`  
**Trạng thái:** đã khảo sát; chưa triển khai  
**Nguồn eZeus khảo sát:** [`MaurycyLiebner/eZeus`](https://github.com/MaurycyLiebner/eZeus), checkout local tại commit ngắn `046f9d9` ngày 2026-08-13  
**Quyết định sản phẩm:** một ứng dụng OldGame; dropdown chọn game; JA2 và eZeus tách hoàn toàn từ cấu hình đến runtime

---

## 1. Mục tiêu

Biến launcher Android hiện tại thành **OldGame Launcher**, nơi người dùng:

1. Chọn game bằng dropdown:
   - **Jagged Alliance 2**
   - **Zeus: Master of Olympus + Poseidon**
2. Cấu hình dữ liệu, hiển thị, điều khiển và log riêng cho game đang chọn.
3. Nhấn một nút Play chung.
4. Chạy đúng engine native tương ứng.
5. Quay lại launcher mà không làm rò state, config, log hoặc thư viện giữa hai game.

Đây là launcher đa engine, không phải trộn source hai game thành một engine.

> **Tên gọi chính xác:** eZeus không phải emulator CPU/Windows. Đây là native open-source reimplementation/source port của Zeus, dùng dữ liệu game gốc.

---

## 2. Quyết định bắt buộc

### 2.1 Một launcher, hai runtime

```text
OldGame Launcher APK
│
├── LauncherActivity                  process mặc định
│   ├── dropdown: Jagged Alliance 2
│   └── dropdown: Zeus + Poseidon
│
├── StracciatellaActivity             process mặc định hoặc :ja2
│   ├── SDL2
│   └── libja2.so
│
└── EZeusActivity                     process :ezeus
    ├── SDL2
    ├── SDL2_image
    ├── SDL2_ttf
    ├── SDL2_mixer
    └── libezeus.so
```

### 2.2 Không link eZeus vào `libja2.so`

Không làm:

```text
libja2.so + toàn bộ source eZeus + hai main/game loop
```

Lý do:

- Hai entrypoint và hai game loop độc lập.
- Hai hệ audio/resource/config khác nhau.
- eZeus cần SDL_image, SDL_ttf, SDL_mixer; JA2 hiện không cần chúng.
- Cả hai dùng nhiều global/static state.
- Nâng upstream, debug crash, đo hiệu năng khó hơn.
- Làm mờ ranh giới giấy phép.

### 2.3 eZeus chạy process riêng

Manifest dùng:

```xml
<activity
    android:name=".EZeusActivity"
    android:process=":ezeus"
    android:screenOrientation="sensorLandscape"
    android:configChanges="orientation|screenSize|keyboardHidden" />
```

Lý do thực tế: `SDLActivity` hiện chứa nhiều field static cho singleton, surface, native thread, input và lifecycle tại `android/app/src/main/java/org/libsdl/app/SDLActivity.java`. Process riêng đảm bảo:

- SDL/native globals không còn từ lần chạy JA2.
- Audio device và renderer được khởi tạo sạch.
- Crash eZeus không kéo theo state native của JA2.
- Có thể kill/restart riêng engine.

Process riêng **không** phải sandbox dữ liệu. Hai process vẫn thuộc cùng application ID và đọc được app-private files.

### 2.4 Dropdown là game context cấp cao nhất

Dropdown không chỉ đổi nhãn. Khi đổi game, launcher phải đổi toàn bộ:

- tabs;
- form dữ liệu;
- validation;
- config model;
- nút Play;
- log hiển thị;
- help text;
- engine được mở.

Không để field JA2 xuất hiện trong Zeus hoặc ngược lại.

---

## 3. UX đích

### 3.1 Bố cục launcher

```text
┌─────────────────────────────────────────────┐
│ OldGame                                     │
│ Game  [ Jagged Alliance 2              ▾ ]  │
├─────────────────────────────────────────────┤
│ Data │ Settings │ Controller │ Logs         │
├─────────────────────────────────────────────┤
│ Nội dung chỉ thuộc game đang chọn           │
│                                             │
│                                      [ Play ]│
└─────────────────────────────────────────────┘
```

Khi chọn Zeus:

```text
┌─────────────────────────────────────────────┐
│ OldGame                                     │
│ Game  [ Zeus + Poseidon                ▾ ]  │
├─────────────────────────────────────────────┤
│ Data │ Display │ Audio │ Logs               │
├─────────────────────────────────────────────┤
│ Original game directory                     │
│ eZeus support files                         │
│ Texture pack                                │
│                                      [ Play ]│
└─────────────────────────────────────────────┘
```

### 3.2 Control được chọn

Dùng Material exposed dropdown hoặc `Spinner` có label **Game**. Ưu tiên Material exposed dropdown vì:

- trạng thái chọn rõ;
- label luôn hiện;
- hỗ trợ TalkBack tốt hơn custom popup;
- phù hợp dependency Material hiện có.

Không dùng hai nút game song song. Yêu cầu sản phẩm đã chốt là dropdown.

### 3.3 Hành vi dropdown

- Mặc định lần cài mới: `Jagged Alliance 2` để không đổi hành vi người dùng hiện tại.
- Lưu lựa chọn cuối trong launcher preferences, ví dụ `selected_game=ja2|ezeus`.
- Đổi game không tự chạy engine.
- Nếu form đang focus, đóng keyboard trước khi thay adapter.
- FAB đổi `contentDescription` và validation theo game.
- Orientation launcher vẫn `fullUser`; cả hai game chạy landscape.

### 3.4 Trạng thái dữ liệu

Dropdown hiển thị thêm trạng thái bằng text, không chỉ màu:

```text
Jagged Alliance 2        Ready
Zeus + Poseidon          Setup required
```

MVP có thể chưa đưa trạng thái vào từng row; tối thiểu phải hiện validation trong tab Data trước khi Play.

---

## 4. Kiến trúc launcher

### 4.1 Game ID duy nhất

Thêm enum nhỏ, không tạo framework plugin:

```kotlin
enum class GameId {
    JA2,
    EZEUS
}
```

Không cần interface/factory tổng quát trong phase đầu. Chỉ có hai game; `when (selectedGame)` rõ hơn abstraction sớm.

### 4.2 Tách model hoàn toàn

Giữ JA2 model hiện tại. Thêm model eZeus riêng:

```text
ConfigurationModel.kt          JA2 hiện tại; có thể đổi tên sau
EZeusConfigurationModel.kt     chỉ eZeus
LauncherSelectionModel.kt      chỉ selected GameId nếu cần survive rotation
```

Không nhét field Zeus vào `Ja2Json` hoặc `ConfigurationModel`.

### 4.3 Tách adapter/tab

`SectionsPagerAdapter` hiện hard-code bốn tab JA2. Đổi tối thiểu theo một trong hai cách:

**Khuyến nghị:** truyền `GameId` vào adapter, dùng hai danh sách fragment riêng.

```kotlin
when (gameId) {
    GameId.JA2 -> ...
    GameId.EZEUS -> ...
}
```

Khi dropdown đổi:

1. cập nhật selected game;
2. gắn adapter mới;
3. rebuild `TabLayoutMediator`;
4. cập nhật FAB action/content description.

Không reuse cùng instance fragment cho hai game.

### 4.4 Config files

```text
filesDir/.ja2/ja2.json
filesDir/.ja2/controller.ini
filesDir/.ezeus/ezeus.json
filesDir/.ezeus/settings.txt       nếu giữ format upstream
filesDir/.ezeus/logs/ezeus.log
filesDir/.launcher/selection.json  hoặc SharedPreferences
```

Schema eZeus tối thiểu:

```json
{
  "gameDir": "/storage/emulated/0/Games/Zeus and Poseidon",
  "supportDir": "/storage/emulated/0/Games/eZeus",
  "saveDir": "/storage/emulated/0/Games/eZeus/Save",
  "texturePack": "i30",
  "width": 1280,
  "height": 720,
  "music": true,
  "sound": true
}
```

Không nhất thiết expose hết field ngay. Phase boot chỉ cần `gameDir`, `supportDir`, texture pack và resolution mặc định.

---

## 5. eZeus đã được xác minh

### 5.1 Chức năng

README upstream xác nhận:

- open-source implementation của Zeus: Master of Olympus;
- hỗ trợ adventures gốc;
- cần dữ liệu gốc của Zeus base game và Poseidon expansion;
- hiện hỗ trợ English và Polish do giới hạn font glyph;
- upstream cung cấp Windows binaries; Linux/macOS build từ source.

### 5.2 Công nghệ

- C++17.
- SDL2 renderer/window/event loop.
- SDL2_image cho PNG/image decoding.
- SDL2_ttf cho font.
- SDL2_mixer cho music/sound.
- `std::filesystem`, `std::ifstream`, `std::ofstream`.
- `std::thread`; thread pool giới hạn 2–4 worker.
- CMake mới có `FetchContent`; qmake cũ vẫn tồn tại.

### 5.3 Runtime entrypoint

`main.cpp` hiện:

1. define `SDL_MAIN_HANDLED`;
2. gọi `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)`;
3. init PNG, mixer, TTF;
4. gọi `eGameDir::initialize()`;
5. kiểm tra texture binaries;
6. tạo `eMainWindow`;
7. tạo SDL window/renderer;
8. chạy `eMainWindow::exec()`.

Android không thể dùng nguyên trạng vì `SDLActivity` mặc định tìm symbol `SDL_main`, trong khi upstream expose `main()` và chủ động disable SDL main handling.

### 5.4 Renderer và lifecycle

`eMainWindow` dùng SDL accelerated renderer. Event loop đã xử lý:

- `SDL_WINDOWEVENT_MINIMIZED` / `RESTORED`;
- `SDL_RENDER_TARGETS_RESET`;
- `SDL_RENDER_DEVICE_RESET`;
- mouse motion/button/wheel;
- keyboard down/up.

Điểm tốt: code đã có đường phục hồi render target khi Android mất/recreate GPU context. Vẫn phải test thật khi Home/Resume, xoay màn hình và lock/unlock.

### 5.5 Data bắt buộc

Dữ liệu chia thành ba nhóm:

#### A. Dữ liệu game thương mại do người dùng cung cấp

Ví dụ:

```text
<gameDir>/DATA/
<gameDir>/Audio/Music/
<gameDir>/Audio/Wavs/
<gameDir>/Audio/Ambient/
<gameDir>/Audio/Voice/
<gameDir>/Model/Zeus eventmsg.txt
<gameDir>/Adventures/
```

Không đóng gói dữ liệu thương mại vào APK/repository.

#### B. eZeus support/generated files

Upstream hiện tìm cạnh executable:

```text
interface.e
một hoặc nhiều: i15.e, i30.e, i45.e, i60.e
Zeus_Text.xml
Zeus_MM.xml
Text/language.txt
settings.txt
numbers.txt
Save/
Textures/
Adventures/
```

`interface.e` và các `i*.e` không nằm đầy đủ trong source checkout; flow release/upstream tạo hoặc cung cấp chúng riêng. Đây là blocker dữ liệu cần giải quyết trước khi tuyên bố app chạy được.

#### C. File redistributable từ source eZeus

Repo có font, text, sanctuary definitions và một số adventure `.epak`. Trước khi đưa vào APK phải kiểm tra provenance/license từng asset, không suy ra GPL cho mọi dữ liệu chỉ vì source có GPL file.

---

## 6. Các blocker Android

### Blocker 1 — Native target sai dạng

CMake upstream dùng:

```cmake
add_executable(eZeus ...)
```

Android cần:

```cmake
add_library(ezeus SHARED ...)
```

và output `libezeus.so`.

### Blocker 2 — Entrypoint

Tách logic khỏi `main()`:

```cpp
int eZeusMain(int argc, char** argv);

#ifndef __ANDROID__
int main(int argc, char** argv) {
    return eZeusMain(argc, argv);
}
#else
extern "C" int SDL_main(int argc, char** argv) {
    return eZeusMain(argc, argv);
}
#endif
```

Không đổi game loop. Chỉ thêm wrapper nền tảng.

### Blocker 3 — Path cạnh executable không writable/không đúng

`eGameDir` hiện dùng `SDL_GetBasePath()` cộng `../` và `../../`. Trên Android:

- native library nằm trong native library directory;
- APK/assets không phải thư mục POSIX bình thường;
- không được ghi settings/save cạnh `.so`;
- đường dẫn `../` không đại diện thư mục game người dùng chọn.

Refactor tối thiểu thành ba root rõ ràng:

```cpp
eGameDir::initialize(gameDataRoot, supportRoot, writableRoot);
```

Ý nghĩa:

- `gameDataRoot`: thư mục Zeus + Poseidon do người dùng chọn;
- `supportRoot`: `interface.e`, `i*.e`, XML/text/font support;
- `writableRoot`: settings, logs, saves trong app-private hoặc save directory đã chọn.

Không giấu ba root sau current working directory.

### Blocker 4 — SDL companion libraries

Project JA hiện chỉ build/package SDL2. eZeus cần thêm:

- SDL2_image;
- SDL2_ttf;
- SDL2_mixer.

Build Android phải reproducible, không `FetchContent` network trong Gradle build. Khuyến nghị:

1. pin source versions;
2. vendor/submodule theo policy repo;
3. tắt codecs/formats không dùng;
4. build companion libraries static và link vào `libezeus.so`, hoặc package shared `.so` rõ ràng.

**MVP khuyến nghị:** SDL2 dùng chung; image/ttf/mixer link static vào `libezeus.so`. Khi đó `EZeusActivity.getLibraries()` chỉ cần:

```kotlin
arrayOf("SDL2", "ezeus")
```

Cách này giảm lỗi load-order Java và giảm số native library công khai. Phải xác minh license các codec được bật.

### Blocker 5 — Original/support data validation

Trước Play, Zeus validator phải kiểm tra case-insensitive tối thiểu:

```text
<gameDir>/DATA
<supportDir>/interface.e
<supportDir>/i30.e hoặc texture pack được chọn
<supportDir>/Zeus_Text.xml
<supportDir>/Zeus_MM.xml
```

Sau boot prototype, mở rộng check đúng những file engine thực sự cần. Error phải nói rõ file nào thiếu và đặt ở đâu.

### Blocker 6 — Touch/gamepad chưa đủ

Upstream chỉ xử lý mouse, wheel, keyboard. Android SDL có thể sinh mouse event từ touch, nhưng “boot được” chưa đồng nghĩa “chơi được”. Cần test và bổ sung tối thiểu:

- tap: left click;
- drag: cursor/drag;
- long press hoặc gesture riêng: right click;
- two-finger vertical: wheel;
- Android Back: Escape hoặc mở menu game;
- keyboard text khi cần;
- gamepad cursor/click nếu sản phẩm yêu cầu.

Không sửa global `SDLActivity` để phục vụ eZeus nếu thay đổi có thể làm JA2 regress. Ưu tiên override trong `EZeusActivity` hoặc event adapter phía eZeus.

### Blocker 7 — Giấy phép

- eZeus checkout chứa GNU GPLv3.
- JA2-Stracciatella có lịch sử license đặc thù: phần thay đổi mới public domain, source gốc theo SFI-SCLA, dependencies có license riêng.
- Tách `.so` hoặc process giúp kiến trúc, **không tự động giải quyết license khi phân phối chung một APK**.

Trước public release phải có review giấy phép về:

1. có được phân phối GPLv3 eZeus cùng binary JA2/SFI-SCLA trong một APK không;
2. source offer và Corresponding Source phải cung cấp thế nào;
3. notices/UI legal text;
4. license SDL_image/ttf/mixer và codec;
5. provenance của font, XML, generated texture packs, bundled adventures;
6. tuyệt đối không bundle original Zeus/JA2 commercial data.

Nếu review kết luận một APK không phù hợp, fallback sản phẩm vẫn giữ trải nghiệm gần như cũ:

- OldGame launcher APK;
- eZeus engine APK riêng được launcher mở qua explicit intent;
- dropdown vẫn là điểm vào duy nhất.

Fallback này chỉ dùng khi pháp lý bắt buộc; kỹ thuật ưu tiên vẫn là một APK.

---

## 7. Cấu trúc source đề xuất

Không refactor toàn repo trước khi boot được. Diff nhỏ nhất:

```text
android/app/src/main/java/io/github/ja2stracciatella/
├── GameId.kt
├── LauncherActivity.kt                 sửa
├── LauncherSelection.kt                mới, rất nhỏ
├── StracciatellaActivity.kt            giữ
├── EZeusActivity.kt                    mới
├── EZeusConfigurationModel.kt          mới
├── EZeusJson.kt                        mới
├── EZeusGameDir.kt                     mới validator
└── ui/main/
    ├── SectionsPagerAdapter.kt          sửa theo GameId
    ├── DataTabFragment.kt               JA2 giữ behavior
    ├── EZeusDataFragment.kt             mới
    ├── EZeusSettingsFragment.kt         thêm sau boot nếu cần
    └── EZeusLogsFragment.kt             thêm sau boot nếu cần

android/app/src/main/res/
├── layout/activity_launcher.xml         thêm dropdown
├── layout/fragment_ezeus_data.xml       mới
└── values/strings.xml                   thêm OldGame/eZeus strings

android/app/src/main/AndroidManifest.xml  thêm EZeusActivity/:ezeus

cmake/ hoặc dependencies/
└── ezeus integration CMake              vị trí chốt sau prototype

third_party/ezeus hoặc dependencies/ezeus
└── source/fork pinned                    chỉ sau khi chốt import strategy
```

Tên package hiện là `io.github.ja2stracciatella`. Không đổi application ID trong cùng phase; đổi package là migration riêng, dễ phá config/signing/update path.

Tên app hiển thị có thể đổi từ `JA2 Stracciatella` thành `OldGame` sau khi eZeus boot pass.

---

## 8. Build integration đề xuất

### 8.1 Không copy toàn bộ CMake upstream trực tiếp vào root

CMake upstream liệt kê hàng nghìn source và tự FetchContent SDL. Tạo lớp integration mỏng:

```text
cmake/ezeus.cmake
```

Trách nhiệm:

- khai báo `ezeus` target `SHARED` trên Android;
- reuse SDL2 target hiện tại;
- link image/ttf/mixer đã pin;
- đặt include paths;
- thêm compile definitions Android;
- không ảnh hưởng target `ja2`.

### 8.2 Import source

Khuyến nghị theo hai bước:

**Prototype:** build từ sibling checkout `/tmp/eZeus` hoặc configurable `EZEUS_SOURCE_DIR`; không commit source lớn khi chưa boot.

```cmake
option(BUILD_EZEUS "Build eZeus Android engine" OFF)
set(EZEUS_SOURCE_DIR "" CACHE PATH "Path to eZeus source")
```

**Sau boot:** chọn một trong:

- fork eZeus có Android patches, pin submodule commit;
- vendor snapshot có upstream commit + license + patch history.

Ưu tiên fork/submodule nếu CI/release luôn clone recursive. Ưu tiên vendor snapshot nếu Android build phải hoạt động từ source tarball không có Git metadata. Không quyết định trước khi kiểm tra release workflow hiện tại.

### 8.3 Gradle

Thêm CMake argument có feature flag trong lúc prototype:

```text
-DBUILD_EZEUS=ON
-DEZEUS_SOURCE_DIR=/absolute/path/for/local-prototype
```

Không hard-code `/tmp/eZeus` vào committed Gradle.

APK phải chứa cho `arm64-v8a`:

```text
lib/arm64-v8a/libSDL2.so
lib/arm64-v8a/libja2.so
lib/arm64-v8a/libezeus.so
```

Nếu companion SDL libraries là shared, thêm chúng vào list và kiểm tra load order. Nếu static, kiểm tra `libezeus.so` không còn `DT_NEEDED` thiếu.

---

## 9. Trình tự triển khai

Không làm UI hoàn chỉnh trước khi chứng minh native boot. Trình tự giảm rủi ro:

### Phase 0 — License/data gate

**Mục tiêu:** biết chính xác thứ được phép ship và bộ file cần để boot.

- Ghi inventory license source/dependencies/assets.
- Xác định cách người dùng lấy `interface.e`, `i30.e`, XML support hợp pháp.
- Chuẩn bị một thư mục test Zeus + Poseidon hợp pháp trên thiết bị.
- Chọn một texture pack cho Android prototype; khuyến nghị `i30.e` trước.

**Exit:** có data test; có quyết định “prototype nội bộ được phép”; chưa cần quyết định public distribution cuối.

### Phase 1 — Native compile smoke test

**Mục tiêu:** tạo được `libezeus.so`, chưa cần launcher đẹp.

- Chuyển target executable thành Android shared target qua integration CMake.
- Thêm `SDL_main` wrapper.
- Build SDL_image/ttf/mixer.
- Stub/path injection cho ba root.
- Thêm temporary `EZeusActivity` trực tiếp trong manifest.
- Launch bằng `adb am start`.

**Exit:** activity mở; native log vào entrypoint; không có `UnsatisfiedLinkError`.

### Phase 2 — First frame

**Mục tiêu:** render menu eZeus.

- Hoàn tất `eGameDir` roots.
- Validate/open `interface.e`, `i30.e`, XML/text/font.
- Tạo SDL window/renderer.
- Load texture support.
- Render first usable menu frame.
- Home/Resume một lần.

**Exit:** menu nhìn thấy; audio init không crash; resume không black screen.

### Phase 3 — Playability input

**Mục tiêu:** điều khiển menu và vào gameplay.

- Xác minh touch-to-mouse.
- Bổ sung right click, wheel, Back/Escape.
- Kiểm tra keyboard/gamepad sau; không chặn first playable nếu touch đủ.
- Test save/load vào writable root.

**Exit:** bắt đầu adventure, đặt công trình, mở/đóng menu, save, load.

### Phase 4 — OldGame dropdown launcher

**Mục tiêu:** một launcher chính thức cho hai game.

- Thêm dropdown.
- Tách adapter/model/config.
- Thêm Zeus Data UI/validation.
- FAB dispatch sang Activity tương ứng.
- Lưu selected game.
- Đổi app display name thành OldGame.

**Exit:** đổi qua lại hai game không lẫn field/config; cả hai launch được.

### Phase 5 — Hardening/release

- Process isolation verification.
- Crash/log reporting eZeus qua file thay vì static singleton.
- RAM/startup profiling.
- Audio focus, phone call interruption, Bluetooth/headphones.
- Renderer context loss.
- Low-storage and missing/corrupt file messages.
- License notices/source publication.
- CI clean clone build; no network FetchContent.

---

## 10. File-level implementation map

### `android/app/src/main/res/layout/activity_launcher.xml`

Hiện chỉ có `TabLayout`, `ViewPager2`, FAB. Thêm game dropdown trong `AppBarLayout`, phía trên tabs hoặc cùng toolbar row. Giữ body/FAB.

### `android/app/src/main/java/io/github/ja2stracciatella/LauncherActivity.kt`

Hiện FAB gọi `startGame()` và luôn mở `StracciatellaActivity`. Tách:

```text
startSelectedGame()
├── startJa2()
└── startEZeus()
```

Mỗi nhánh dùng validator/config riêng.

### `android/app/src/main/java/io/github/ja2stracciatella/ui/main/SectionsPagerAdapter.kt`

Hiện bốn tab đều là JA2. Làm adapter nhận `GameId`; không để Zeus dùng `SettingsFragment`/`ControllerFragment` JA2.

### `android/app/src/main/AndroidManifest.xml`

Giữ launcher `fullUser`. Giữ JA2 landscape. Thêm eZeus landscape + `android:process=":ezeus"`.

### `android/app/src/main/java/io/github/ja2stracciatella/EZeusActivity.kt`

Tối thiểu:

- extends `SDLActivity`;
- trả native libraries;
- landscape/immersive giống game Activity;
- truyền args/path config nếu chọn args bridge;
- không đọc JA2 config.

Không copy toàn bộ `StracciatellaActivity` nếu chỉ khác library name; chỉ trích helper chung sau khi duplication thật sự xuất hiện.

### eZeus `main.cpp`

Tách platform wrapper; giữ `eZeusMain` dùng chung desktop/Android. Không fork hai game loops.

### eZeus `egamedir.cpp/.h`

Đây là refactor portability quan trọng nhất. Loại giả định mọi thứ nằm quanh executable. Tách read-only commercial root, read-only support root, writable root.

### eZeus `emainwindow.cpp`

Không sửa trước first frame trừ lỗi Android cụ thể. Sau boot mới xử lý:

- fullscreen/window semantics;
- touch/right-click gesture;
- pause/resume edge cases;
- FPS clamp/performance.

---

## 11. Validation thư mục Zeus

Validator launcher phải trả lỗi hành động được:

| Kiểm tra | Lỗi hiển thị |
|---|---|
| Chưa chọn game dir | Chọn thư mục cài Zeus + Poseidon gốc. |
| Không có `DATA` | Thư mục này không chứa `DATA`; hãy chọn thư mục gốc của game. |
| Không có `interface.e` | Thiếu eZeus support file `interface.e`. |
| Không có texture pack | Thiếu `i30.e` hoặc texture pack đã chọn. |
| Không có XML | Thiếu `Zeus_Text.xml`/`Zeus_MM.xml`. |
| Save dir không writable | Chọn thư mục save khác hoặc dùng app-private storage. |

Case-insensitive lookup giống JA2 validator hiện tại, vì dữ liệu từ Windows có casing không ổn định.

Không cho “Continue anyway” với support file bắt buộc; engine chắc chắn fail thì launcher phải chặn.

---

## 12. Log và crash

Process `:ezeus` không chia sẻ `NativeExceptionContainer` static với launcher process. Không dựa vào singleton hiện tại để báo lỗi eZeus.

MVP:

```text
filesDir/.ezeus/logs/ezeus.log
filesDir/.ezeus/last_run.json
```

`last_run.json`:

```json
{
  "status": "crashed",
  "message": "Could not open interface.e",
  "timestamp": "..."
}
```

Launcher đọc file khi resume và hiển thị lỗi. Ghi file atomically: temp + rename, tránh file hỏng khi process chết.

JA2 giữ flow hiện tại; không refactor chung trong phase boot.

---

## 13. Hiệu năng và dung lượng

Các điểm phải đo, không đoán:

- kích thước `libezeus.so` stripped;
- kích thước SDL companion/codecs;
- kích thước support texture packs;
- peak RSS khi load menu/game;
- số texture/GPU memory;
- first-frame time;
- save/load time;
- thermal behavior sau 20 phút.

Android prototype chỉ bật một texture pack phù hợp thay vì mặc định bật `i15`, `i30`, `i45`, `i60`. Khuyến nghị bắt đầu `i30`; đổi sau số đo DPI/RAM thật.

Thread pool 2–4 worker tương thích NDK về nguyên tắc. Không tối ưu trước profile.

---

## 14. Test matrix bắt buộc

### Launcher

- Cài mới mặc định JA2.
- Dropdown đổi JA2 → Zeus → JA2.
- Rotation launcher portrait/landscape giữ lựa chọn.
- Mỗi game giữ config riêng sau app restart.
- FAB luôn mở đúng Activity.
- Thiếu data báo đúng game, đúng file.

### Isolation

- Chạy JA2, thoát về launcher, chạy Zeus.
- Chạy Zeus, thoát về launcher, chạy JA2.
- Lặp 5 lần; không `UnsatisfiedLinkError`, không stale SDL surface/audio.
- Force-stop process `:ezeus`; launcher còn hoạt động.

### eZeus runtime

- First frame.
- Tap/drag/right-click/wheel.
- Start adventure.
- Build/select/delete.
- Music + SFX.
- Save/load.
- Home 10 giây, resume.
- Lock/unlock.
- Bluetooth audio reconnect.
- Renderer reset không black screen.

### JA2 regression

- Launcher fields/tabs hiện như trước khi chọn JA2.
- Start game.
- Touch/controller hiện tại.
- Save/load.
- Return launcher.
- Không tăng startup đáng kể do eZeus initialization; eZeus libraries không load khi mở JA2.

### Build

```bash
git diff --check
./gradlew :app:assembleDebug
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

CMake output phải lọc theo CLAUDE.md; không dump toàn log.

Kiểm tra APK:

```bash
unzip -l android/app/build/outputs/apk/debug/app-debug.apk \
  | grep -E 'lib/(arm64-v8a)/(libSDL2|libja2|libezeus).*\.so'
```

Kiểm tra dependencies native bằng NDK `llvm-readelf -d libezeus.so`; không để `DT_NEEDED` trỏ library không nằm trong APK.

---

## 15. Definition of Done

Feature chỉ được gọi là hoàn thành khi tất cả đúng:

- [ ] Launcher hiển thị dropdown hai game.
- [ ] Chọn game thay toàn bộ UI/config context.
- [ ] JA2 và eZeus có config files riêng.
- [ ] JA2 và eZeus có Activity/native library riêng.
- [ ] eZeus chạy process `:ezeus`.
- [ ] Không bundle dữ liệu thương mại.
- [ ] Zeus data/support validation rõ ràng.
- [ ] eZeus render menu, nhận touch thiết yếu, vào gameplay.
- [ ] Save/load eZeus hoạt động trên writable storage.
- [ ] Chuyển qua lại hai engine không crash/state leak.
- [ ] JA2 regression test pass.
- [ ] Clean Android build không cần FetchContent network.
- [ ] License/source/notices được review trước phát hành.

---

## 16. Không làm trong vòng đầu

- Không xây plugin framework cho game thứ ba.
- Không đổi application ID/package namespace.
- Không redesign toàn launcher.
- Không hợp nhất JA2/eZeus settings schema.
- Không chuyển JA2 sang process mới nếu chưa có lỗi thực.
- Không viết file browser mới; reuse chooser hiện tại trong MVP.
- Không port renderer sang OpenGL/Vulkan riêng; SDL renderer đã đủ để chứng minh.
- Không hỗ trợ mọi ABI; giữ `arm64-v8a` theo build hiện tại.
- Không tối ưu eZeus trước khi first-frame/profile.
- Không ship original game data.

**Ceiling chủ ý:** file chooser hiện tại dựa trên đường dẫn external storage và target SDK cũ. Nâng sang Storage Access Framework/copy-import chỉ khi target SDK/scoped storage bắt buộc hoặc thiết bị mục tiêu không cấp path trực tiếp.

---

## 17. Rủi ro xếp hạng

| Mức | Rủi ro | Giảm thiểu |
|---|---|---|
| Critical | GPLv3/SFI-SCLA phân phối cùng APK | legal review; giữ fallback engine APK riêng |
| High | Thiếu/provenance không rõ của `.e` support packs | xác định pipeline và quyền phân phối trước release |
| High | Path model desktop không hoạt động Android | ba explicit roots; không dùng `../` |
| High | SDL_image/ttf/mixer + codecs build/package | pin, vendor, static-link vào `libezeus.so`, inspect ELF |
| Medium | Touch không đủ chơi | gesture adapter riêng eZeus; test gameplay thật |
| Medium | GPU context loss/black screen | reuse reset handling; lifecycle stress test |
| Medium | RAM do texture packs | một pack mặc định; profile peak RSS/GPU |
| Medium | Static SDLActivity state | process `:ezeus` |
| Low | libnoise stale dependency | compile chứng minh; bỏ chỉ khi không có symbol/use |

---

## 18. Bước tiếp theo duy nhất

Không bắt đầu từ dropdown. Bước tiếp theo:

> **Tạo native Android smoke target `libezeus.so` từ checkout eZeus, expose `SDL_main`, thêm `EZeusActivity`, launch được tới log entrypoint.**

Lý do: đây là rủi ro kỹ thuật lớn nhất. Dropdown chỉ là Kotlin/XML nhỏ; làm trước sẽ tạo UI không chạy game.

Sau smoke target:

1. refactor path roots;
2. đạt first frame;
3. đạt input/playability;
4. mới nối dropdown launcher.

---

## 19. Prompt tiếp quản cho session mới

```text
Đọc docs/OLDGAME-LAUNCHER-EZEUS-HANDOFF.md.
Thực hiện riêng Phase 1: native Android compile smoke test cho eZeus.
Giữ JA2 nguyên trạng. Không làm dropdown/UI đầy đủ.
Dùng /tmp/eZeus làm source local prototype nhưng không hard-code path vào file commit.
Tạo libezeus.so, SDL_main wrapper, EZeusActivity process :ezeus.
Build arm64, launch bằng adb, báo chính xác compile/load/runtime blockers.
Tuân CLAUDE.md: không đọc android/build.log; cmake output chỉ grep error/warning tail.
Không commit/push.
```

---

## 20. Bằng chứng source chính

### Project hiện tại

- `android/app/src/main/java/io/github/ja2stracciatella/LauncherActivity.kt`: FAB, JA2 validation, mở `StracciatellaActivity`, config path.
- `android/app/src/main/java/io/github/ja2stracciatella/StracciatellaActivity.kt`: SDL libraries và landscape behavior.
- `android/app/src/main/java/org/libsdl/app/SDLActivity.java`: `SDL_main`, native libraries, static lifecycle/native thread state.
- `android/app/src/main/res/layout/activity_launcher.xml`: TabLayout, ViewPager2, FAB hiện tại.
- `android/app/src/main/java/io/github/ja2stracciatella/ui/main/SectionsPagerAdapter.kt`: bốn tab JA2 hard-coded.
- `android/app/src/main/java/io/github/ja2stracciatella/GameDir.kt`: validation case-insensitive có thể làm mẫu.
- `android/app/build.gradle`: arm64, CMake, assets, Android SDK/NDK config.
- `CMakeLists.txt`: Android build `libja2.so`, SDL2 và native dependencies.

### eZeus checkout

- `README.md`: yêu cầu original Zeus + Poseidon data, platform/language support.
- `LICENSE.md`: GNU GPL version 3.
- `main.cpp`: init SDL companion libraries, data checks, main loop entry.
- `egamedir.cpp/.h`: desktop-relative path assumptions.
- `emainwindow.cpp/.h`: SDL window/renderer/event/lifecycle loop.
- `ebinaryimageloader.cpp`: đọc `interface.e`/`i*.e` bằng filesystem.
- `textures/egametextures.cpp`: kiểm tra original `DATA` directory.
- `elanguage.cpp`: `Zeus_Text.xml`, `Zeus_MM.xml`, language text paths.
- `CMakeLists.txt`: C++17, executable target, SDL FetchContent/link targets.

Handoff này là nguồn quyết định chính. Nếu implementation phát hiện source upstream đã đổi, cập nhật bằng bằng chứng compile/runtime; không âm thầm đổi kiến trúc một launcher/hai process/hai engine.
