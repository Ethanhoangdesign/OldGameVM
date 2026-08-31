# Controller Remapper — Spec giao DeepSeek (checklist)

> Làm tới đâu tick `- [x]` tới đó. Mỗi session build sạch + test được rồi mới sang session sau.

## Bối cảnh

Repo JA2 Stracciatella fork "OldGameVM". Gamepad native đã có trong engine
(`SDL_GameController`), map nút → chuột/phím **hardcode** trong
`src/sgp/GameController.cc`. Cần:

1. Engine đọc binding từ `controller.ini` thay hardcode.
2. Tab Controller trong launcher: **vẽ hình tay cầm bằng FLTK primitives** (không
   asset ngoài) + **bảng gán nút tương tác** (chọn hành động → bấm nút thật → gán).

## Ràng buộc cứng

- [ ] KHÔNG thêm dependency. SDL2 + FLTK 1.3.11 đã có sẵn.
- FLTK local **không có** `Fl_SVG_Image.H` → vẽ tay cầm bằng `fl_*` primitives.
- Launcher `.cc`/`.h` là nguồn THẬT (máy build không có fluid). `.fl` bỏ qua.
- macOS arm64. Build: `cmake --build build -j8`. Chạy: `./build/ja2-launcher`, `./build/ja2`.
- Đường dẫn game có dấu `( )` → luôn bọc nháy kép trong shell.

## API SDL tái dùng (KHÔNG tự chế bảng tên nút)

```c
SDL_GameControllerGetStringForButton(SDL_GameControllerButton) // -> "a","b","leftshoulder","dpup"...
SDL_GameControllerGetButtonFromString(const char*)             // -> SDL_GameControllerButton (INVALID nếu sai)
SDL_GameControllerUpdate(); SDL_PollEvent(&e);                 // detect nút bấm live
SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER); SDL_GameControllerOpen(i);
```

## Bảng hành động (dùng chung engine + launcher)

| key ini | nhãn UI | nút default | hiệu ứng engine |
|---|---|---|---|
| `mouse_left`  | Left click  | `a`             | `PadInjectMouseButton(SDL_BUTTON_LEFT, down)` |
| `mouse_right` | Right click | `b`             | `PadInjectMouseButton(SDL_BUTTON_RIGHT, down)` |
| `scroll_up`   | Scroll up   | `leftshoulder`  | `if(down) PadInjectWheel(true)` |
| `scroll_down` | Scroll down | `rightshoulder` | `if(down) PadInjectWheel(false)` |
| `arrow_up`    | Arrow Up    | `dpup`          | `SendKey(SDLK_UP, down)` |
| `arrow_down`  | Arrow Down  | `dpdown`        | `SendKey(SDLK_DOWN, down)` |
| `arrow_left`  | Arrow Left  | `dpleft`        | `SendKey(SDLK_LEFT, down)` |
| `arrow_right` | Arrow Right | `dpright`       | `SendKey(SDLK_RIGHT, down)` |
| `confirm`     | Enter       | `start`         | `SendKey(SDLK_RETURN, down)` |
| `cancel`      | Esc         | `back`          | `SendKey(SDLK_ESCAPE, down)` |

Analog trái → con trỏ: **giữ nguyên, không remap**. X/Y không default → user gán tùy ý.

Format `~/.ja2/controller.ini` mới (tương thích ngược — code cũ chỉ đọc `enabled`):

```ini
enabled=1
mouse_left=a
mouse_right=b
scroll_up=leftshoulder
scroll_down=rightshoulder
arrow_up=dpup
arrow_down=dpdown
arrow_left=dpleft
arrow_right=dpright
confirm=start
cancel=back
```

---

## SESSION A — Engine đọc binding, bỏ switch hardcode

File duy nhất: `src/sgp/GameController.cc`

Hiện tại `HandleButton()` (~dòng 89) switch cứng btn → hành động.
`ReadEnabledFlag()` (~dòng 28) đã parse `key=val` từ ini, chỉ trả `enabled`.
`SendKey(SDL_Keycode, bool)` đã có trong namespace ẩn danh.

- [x] A1. Thêm `enum PadAction { ACT_MOUSE_LEFT, ACT_MOUSE_RIGHT, ACT_SCROLL_UP, ACT_SCROLL_DOWN, ACT_ARROW_UP, ACT_ARROW_DOWN, ACT_ARROW_LEFT, ACT_ARROW_RIGHT, ACT_CONFIRM, ACT_CANCEL, ACT_COUNT };`
- [x] A2. Bảng tĩnh `{ const char* key; const char* defBtn; }` theo bảng hành động ở trên.
- [x] A3. `SDL_GameControllerButton g_buttonAction[SDL_CONTROLLER_BUTTON_MAX];` init tất cả = `(SDL_GameControllerButton)-1`.
- [x] A4. Đọc ini mỗi action: `val=SDL_GameControllerGetButtonFromString(...)`, thiếu/INVALID → dùng `defBtn`. Gán `g_buttonAction[btn]=action`. (Mở rộng `ReadEnabledFlag` → `ReadConfig()` trả cả `enabled` lẫn map.)
- [x] A5. `void DoAction(PadAction a, bool down)` = thân switch cũ, dispatch theo enum (giữ nguyên hiệu ứng bảng trên).
- [x] A6. `HandleButton(btn, down)`: `auto a=g_buttonAction[btn]; if(a>=0) DoAction((PadAction)a, down);` — bỏ switch hardcode.
- [x] A7. Giữ nguyên `enabled` + analog. Ghi chú `ponytail:` analog cứng, chưa remap.
- [x] A8. **Self-check** (không cần tay cầm): block `#ifdef OGVM_TEST_CTRL` hoặc file test nhỏ — feed ini mẫu → `assert` map đúng + default khi thiếu key. Assert-based, no framework.
- [x] A9. `cmake --build build -j8` sạch (`ja2`).

## SESSION B — Vẽ tay cầm + bảng (layout tĩnh, chưa detect)

Files: `src/launcher/StracciatellaLauncher.{h,cc}`, `src/launcher/Launcher.{h,cc}`, + file mới `src/launcher/ControllerView.h`

Group Controller ở `StracciatellaLauncher.cc` (~dòng 225), coord `(0,55,580,465)`.
Hiện có: `controllerEnableToggle`, `controllerStatusLabel`, `controllerImageBox`
(Fl_Box trống `(24,132,530,200)`), `controllerHelp`.

- [x] B1. Tạo `src/launcher/ControllerView.h` (header-only): `class ControllerView : public Fl_Box` override `draw()`. Vẽ bằng `fl_color`/`fl_rectf`/`fl_pie`/`fl_arc`/`fl_draw`: thân xám bo tròn, 2 analog dưới, dpad trái, 4 nút mặt phải (nhãn A/B/X/Y), 2 bumper trên. Bố cục giống dialog antimicrox.
- [x] B2. Thêm `int highlightBtn = -1;` trong ControllerView; nút trùng vẽ màu nhấn (Session C dùng).
- [x] B3. Trong `StracciatellaLauncher.h`: đổi `Fl_Box* controllerImageBox` → `ControllerView* controllerView`. Thêm `Fl_Button* bindBtn[10];`.
- [x] B4. Trong `StracciatellaLauncher.cc`: `#include "ControllerView.h"`. Thay imageBox bằng `controllerView` đặt góc phải-trên `(320,70,240,150)`.
- [x] B5. Thêm bảng 10 hàng bên trái (bắt đầu ~`(24,70)`, cao ~28px/hàng): mỗi hàng = `Fl_Box` nhãn hành động + `Fl_Button bindBtn[i]` hiện tên nút hiện tại.
- [x] B6. Rút gọn/giữ `controllerHelp` bên dưới.
- [x] B7. `Launcher.cc` `loadControllerConfig()` (~dòng 596): đọc thêm 10 binding, set label cho `bindBtn[i]`. Giữ đọc `enabled`.
- [x] B8. KHÔNG cần sửa `src/launcher/CMakeLists.txt` — header mới bắt qua `file(GLOB *.h)`; giữ ControllerView **header-only** để khỏi thêm `.cc` vào `add_executable`.
- [ ] B9. `cmake --build build -j8` sạch → `./build/ja2-launcher` → tab Controller thấy hình tay cầm + bảng nút, không crash.

## SESSION C — Live detect + lưu binding

Files: `src/launcher/Launcher.{h,cc}`

Launcher đã link `SDL2_LIBRARY` (xem `src/launcher/CMakeLists.txt`). Event loop =
`Fl::run()`; poll SDL qua `Fl::add_timeout` (đã dùng nhiều chỗ trong `Launcher.cc`).

- [x] C1. Khi mở launcher/tab: `SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)`, mở tay cầm đầu (loop `SDL_NumJoysticks` + `SDL_IsGameController` + `SDL_GameControllerOpen`).
- [x] C2. Callback click `bindBtn[i]` → vào "listen mode" cho action i; `controllerStatusLabel` = "Press a button on your gamepad…".
- [x] C3. `Fl::add_timeout(0.03, pollCb, this)` lặp: `SDL_GameControllerUpdate()` + `while(SDL_PollEvent(&e))`; bắt `e.type==SDL_CONTROLLERBUTTONDOWN` đầu tiên.
- [x] C4. Khi bắt được: lưu `binding[i]=e.cbutton.button`, set label `bindBtn[i]` = `SDL_GameControllerGetStringForButton(...)`, set `controllerView->highlightBtn` + `redraw()`, thoát listen, `Fl::remove_timeout`.
- [x] C5. `saveControllerConfig()` (~dòng 614): ghi thêm 10 dòng action (giữ `enabled=`). Gọi khi đổi binding + khi toggle.
- [x] C6. Rút/cắm tay khi listen: `if(g_pad==nullptr)` bỏ qua, không crash.
- [x] C7. Ghi chú `ponytail:` chỉ tay cầm đầu tiên; đa tay cầm nâng sau.
- [ ] C8. `cmake --build build -j8` sạch → click nút → bấm tay cầm thật → label đổi + highlight; Save → đọc lại đúng.

## SESSION D — Test tay cầm thật + commit

- [ ] D1. Bật toggle, gán vài nút, Save → mở `~/.ja2/controller.ini` kiểm đúng format 11 dòng.
- [ ] D2. `./build/ja2` với `enabled=1`: verify binding mới ăn trong game (đổi A/B thử).
- [ ] D3. Chỉnh `AXIS_DEADZONE` (8000) / `MAX_SPEED_PX` (600) trong `GameController.cc` nếu con trỏ nhanh/chậm/trôi.
- [ ] D4. Rút/cắm tay khi đang chơi (hotplug) không crash.
- [ ] D5. Commit — gồm cả phần controller chưa commit trong `OGVM_HANDOFF.md` mục 4 (9 sửa + 3 mới) + session A–C.
- [ ] D6. **BẮT BUỘC** trước push: `git diff --cached --name-only | grep -E '^(build|build-win)/'` phải **RỖNG**.

---

## Số liệu tham chiếu nhanh

- Input API public (`src/sgp/Input.h`): `PadInjectMouseButton(UINT8 sdlButton, bool down)`, `PadInjectWheel(bool up)`, `KeyDown/KeyUp(const SDL_Keysym*)`, `SetSafeMousePosition(int,int)`, `SimulateMouseMovement(UINT32,UINT32)`.
- Home dir: `EngineOptions_getStracciatellaHome()` → `~/.ja2/`. Launcher đã dùng `findPathFromStracciatellaHome(eo, nullptr, false, false)` trong `Launcher.cc` để lấy path `controller.ini`.
- `GameController.cc` đã include: `Input.h Timer.h Logger.h UILayout.h RustInterface.h`, `<fstream> <string> <cmath>`.
- Con trỏ engine: `gusMouseXPos/gusMouseYPos` (toạ độ LOGIC, không phải pixel).
- Launcher parse ini hiện tại: `Launcher.cc` ~dòng 596 (`loadControllerConfig`), ~dòng 614 (`saveControllerConfig`), ~dòng 650 (`controllerToggleCb`).
- Widget controller khai trong `StracciatellaLauncher.h` ~dòng 62–66.
- CMake launcher `file(GLOB ...*.h)` → header mới tự nhận; `.cc` mới **phải** thêm tay vào `add_executable` → vì vậy giữ ControllerView header-only.

