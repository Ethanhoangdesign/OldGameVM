# Android Save Shortcuts — Handoff

**Ngày:** 2026-08-17  
**Branch:** `feature/multi-edition-detector`  
**Trạng thái:** Đã sửa và xác minh trên thiết bị thật; tổ hợp controller `Ctrl+S` hoạt động

## Mục tiêu

Sửa bàn phím Android nhận từng phím `Ctrl`, `Alt`, `S`, nhưng không kích hoạt tổ hợp:

- `Ctrl+S`: mở Save screen
- `Alt+S`: quick-save

## Root cause

Game dispatch hotkey đúng trong `src/game/Tactical/Turn_Based_Input.cc`:

- `Ctrl+S`: `HandleModCtrl()` → Save screen
- `Alt+S`: `HandleModAlt()` → `DoQuickSave()`

`InputAtom.usKeyState` lấy modifier từ SDL `KMOD_CTRL` / `KMOD_ALT`. Android `KeyEvent` có modifier trong `getMetaState()`, nhưng `SDLActivity.java` chỉ chuyển `keyCode` xuống native SDL:

```java
SDLActivity.onNativeKeyDown(keyCode);
```

Một số bàn phím Android/IME không gửi sự kiện modifier riêng. Chúng chỉ gửi `S` kèm `META_CTRL_ON` hoặc `META_ALT_ON`. SDL vì vậy nhận `s` với modifier bằng 0; game không gọi save handler.

Ngoài ra, `isTextInputEvent()` chỉ loại `Ctrl`, không loại `Alt`; `Alt+S` có thể bị đường text input xử lý thay vì SDL key event.

## Thay đổi

### `android/app/src/main/java/org/libsdl/app/SDLActivity.java`

Thêm trạng thái:

```java
mCtrlKeyDown
mAltKeyDown
mSyntheticCtrlDown
mSyntheticAltDown
```

Thêm helper:

- `isCtrlKey()` / `isAltKey()`
- `updateModifierKeyState()`
- `pressMissingModifiers()`
- `releaseSyntheticModifiers()`

Cơ chế:

1. Nếu Android gửi modifier riêng, forward bình thường; ghi nhận trạng thái.
2. Nếu phím thường có `META_CTRL_ON` / `META_ALT_ON` nhưng chưa có modifier event, phát modifier `KEY_DOWN` tổng hợp trước phím thường.
3. Sau `KEY_UP` của phím thường, phát modifier `KEY_UP` tổng hợp.
4. Không gửi phím có `Ctrl` hoặc `Alt` qua SDL text input.
5. Áp dụng cho cả `SDLSurface.onKey()` và `DummyEdit.onKey()`.

Vị trí chính hiện tại:

- State: khoảng dòng 194–197
- Modifier helpers: khoảng dòng 1295–1348
- `SDLSurface.onKey()`: khoảng dòng 2037–2090
- `DummyEdit.onKey()`: khoảng dòng 2347–2363

## Lỗi build CTGT đã xử lý

Build trước đó fail:

```text
static declaration of 'guiCTGT_*' follows non-static declaration
```

Nguyên nhân: `src/game/Tactical/LOS.h` khai báo `extern`, trong khi `src/game/Tactical/LOS.cc` định nghĩa `static`.

Đã xóa các khai báo `extern guiCTGT_*` khỏi `LOS.h`. `LOS.h` hiện trở về trạng thái không có diff; counters vẫn private trong `LOS.cc`.

## Phát hiện bổ sung: controller giả lập bàn phím

Controller Android không đi qua `KeyEvent` như bàn phím. `src/sgp/GameController.cc::SendKey()` gọi thẳng `KeyDown()` / `KeyUp()` bằng `SDL_Keysym`, nhưng trước đây luôn để `keysym.mod = 0`. Vì vậy mọi controller SDL (DualSense, Xbox, GameSir) chỉ tạo từng phím riêng lẻ; game không thấy tổ hợp.

Đã sửa `SendKey()` lấy trạng thái `SHIFT`, `CTRL`, `ALT` hiện tại và điền `KMOD_SHIFT`, `KMOD_CTRL`, `KMOD_ALT` vào `keysym.mod` trước khi chuyển phím tới game.

Log thiết bị xác nhận:

- Nút gán Ctrl tạo `ctrlState=1`.
- Phím tiếp theo nhận `mod=192` (`KMOD_CTRL`).
- Alt tương tự với `mod=768` (`KMOD_ALT`).
- Lỗi test cuối cùng do nút mục tiêu đang gán `PageUp`, không phải `S`; đổi mapping thành `Keyboard key → S` thì `Ctrl+S` hoạt động.

Cấu hình đúng có dạng:

```ini
dpup=key:s
```

## Xác minh đã làm

```text
git diff --check: PASS
Android Release build: PASS
adb install -r: PASS
APK chạy trên thiết bị thật: PASS
Controller Ctrl+S: PASS
```

Phạm vi sửa controller nằm trong đường `SendKey()` chung, không phụ thuộc layout/tên thiết bị; áp dụng cho DualSense, Xbox và GameSir được SDL nhận diện.

Chưa xác minh trực tiếp:

- Bàn phím vật lý Android gửi `Ctrl+S` / `Alt+S`
- `Alt+S` controller tạo quick-save
- Xbox và GameSir trên phần cứng thật

## Build, cài, chạy

Chạy nguyên khối; không copy phần shell prompt như `ethan@... %`:

```bash
cd "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella" && \
./tools/build-android-relwithdebinfo.sh > /tmp/build.log 2>&1 && \
adb -d install -r "/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android/app/build/outputs/apk/release/app-release.apk" && \
adb -d shell am force-stop io.github.ja2stracciatella && \
adb -d shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Nếu fail:

```bash
grep -E "error:|ERROR|FAILED" /tmp/build.log | tail -30
```

APK:

```text
/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella/android/app/build/outputs/apk/release/app-release.apk
```

Không chạy đường dẫn APK như command. Luôn dùng `adb install -r "...apk"` vì đường dẫn chứa khoảng trắng.

## Test thiết bị

Trong tactical screen, ngoài conversation/pre-battle lock:

1. Nhấn `Ctrl+S`; Save screen phải mở.
2. Thoát Save screen.
3. Nhấn `Alt+S`; quick-save phải chạy, không mở Save screen.
4. Nhả modifier; nhấn `S` đơn; game phải đổi stance như cũ.
5. Lặp bằng Ctrl/Alt trái và phải nếu bàn phím có đủ phím.
6. Kiểm tra Ctrl/Alt + click vẫn tạo right-click trên Android.

## Working tree

Các file liên quan trực tiếp:

```text
M android/app/src/main/java/org/libsdl/app/SDLActivity.java
M src/game/Tactical/LOS.cc                         # CTGT instrumentation từ task trước
?? tools/build-android-relwithdebinfo.sh
?? docs/HANDOFF-android-save-shortcuts.md
```

`SDLActivity.java` còn chứa thay đổi task trước: xóa nút overlay `RMB`. Không revert toàn file khi xử lý shortcut.

## Bước tiếp theo

1. Test bổ sung `Alt+S`, bàn phím vật lý, Xbox và GameSir khi có thiết bị.
2. Nếu một mapping không kích hoạt hotkey, kiểm tra `controller.ini`; xác nhận output là `key:s`, không phải `key:pageup` hay phím khác.
3. Không giữ log `JA2Input` tạm trong bản commit.
