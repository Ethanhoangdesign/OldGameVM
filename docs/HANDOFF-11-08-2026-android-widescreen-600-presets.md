# HANDOFF — 11/08/2026

**Android widescreen presets tại 600/480 chiều cao**

| | |
|---|---|
| Branch | `feature/multi-edition-detector` |
| Repo | `/Users/ethan/Documents/Ethan_repo/JA for all/ja2-stracciatella` |
| Scope | Android launcher resolution presets |
| File thay đổi | `android/app/src/main/java/io/github/ja2stracciatella/ConfigurationModel.kt` |

---

## 0. STATUS

| Work | Status | Note |
|---|---|---|
| Thêm preset `934x480`  | DONE | Widescreen hẹp tại chiều cao tối thiểu, đứng đầu danh sách |
| Thêm preset `1280x600` | DONE | Widescreen tại 600 chiều cao |
| Thêm preset `1280x480` | DONE | Widescreen tại chiều cao tối thiểu |
| Thêm preset `1024x600` | DONE | Medium-wide tại 600 chiều cao |
| Build APK verify | TODO | `cd android && ./gradlew assembleDebug` |
| Live emulator test | TODO | Chọn từng preset, xác nhận game khởi động và map rộng ra |

---

## 1. BỐI CẢNH

Người dùng đang chạy game ở `800x600` trên Android emulator. Ở kích thước này nhân vật dễ nhìn nhất (zoom level phù hợp). Tuy nhiên emulator rộng hơn 800px nên game bị pillarbox (2 cột đen hai bên).

Yêu cầu: giữ nguyên kích thước nhân vật (cùng chiều cao) nhưng mở rộng chiều ngang để thấy nhiều bản đồ tactical hơn.

---

## 2. CÁCH HOẠT ĐỘNG

Từ `UILayout.cc`:

```cpp
m_VIEWPORT_END_X = m_screenWidth;
m_VIEWPORT_END_Y = m_screenHeight - 120;   // 120px = UI bar dưới
```

Tactical map viewport chỉ phụ thuộc `screenWidth × (screenHeight - 120)`. Tăng chiều ngang mà giữ nguyên chiều cao → nhân vật **cùng kích thước**, thấy **thêm map** hai bên.

| Preset | Map viewport | Ghi chú |
|---|---|---|
| `800x600` | 800 × 480 | Gốc |
| `934x480` | 934 × 360 | +17% rộng hơn, chiều cao tối thiểu |
| `1024x600` | 1024 × 480 | +28% rộng hơn |
| `1280x600` | 1280 × 480 | +60% rộng hơn |
| `1280x480` | 1280 × 360 | +60% rộng, chiều cao tối thiểu (MIN_INTERFACE_HEIGHT = 480) |

`1280x480` là preset "rộng nhất / zoom nhân vật lớn nhất" — chiều cao 480 là sàn cứng của engine, thấp hơn sẽ throw exception.

---

## 3. THAY ĐỔI

### `android/app/src/main/java/io/github/ja2stracciatella/ConfigurationModel.kt`

Thêm 4 preset vào `Resolution.PRESETS` (`934x480` lên đầu danh sách, 3 preset còn lại sau `1024x768`, trước `800x600`):

```kotlin
Resolution(934u, 480u),  // widescreen at minimum height, narrower than 1280
Resolution(1280u, 600u), // widescreen at 600 height — same zoom as 800x600
Resolution(1280u, 480u), // widescreen at minimum height
Resolution(1024u, 600u), // medium-wide at 600 height — same zoom as 800x600
```

Danh sách đầy đủ sau khi thêm:

```
934x480    ← MỚI: đứng đầu — widescreen hẹp, min height
1366x768   ← HD Big Map
1280x768   ← HD Big Map
1024x768
1280x600   ← MỚI: wide + nhân vật 600px
1280x480   ← MỚI: wide + nhân vật to nhất (min height)
1024x600   ← MỚI: medium-wide + nhân vật 600px
800x600
640x480
1664x768   ← ultra-wide
```

---

## 4. VERIFY

```sh
cd android && ./gradlew assembleDebug
```

Sau đó install và test từng preset mới:

```sh
./gradlew installDebug && adb shell am start -n io.github.ja2stracciatella/.LauncherActivity
```

Checklist:
1. Chọn `934x480` → game mở được, map rộng hơn `800x600`, nhân vật lớn hơn (min height).
2. Chọn `1280x600` → game mở được, map rộng hơn, nhân vật cùng kích thước với `800x600`.
3. Chọn `1280x480` → game mở được, map rộng hơn, nhân vật lớn hơn `800x600`.
4. Chọn `1024x600` → game mở được.
5. Xác nhận UI bar dưới (team slots, radar, clock) hiển thị đúng ở cả 4 preset.

Nếu crash, lấy log:

```sh
adb logcat | grep -i "fatal\|backtrace\|libja2\|SIGSEGV"
```

---

## 5. LIÊN QUAN

- `docs/HANDOFF-11-08-2026-android-800x600-crash.md` — crash fix cho `800x600` (temp-surface clipping)
- `src/game/UILayout.cc` — viewport calculation (`m_VIEWPORT_END_X`, `m_VIEWPORT_END_Y`)
- `android/app/src/main/java/io/github/ja2stracciatella/ConfigurationModel.kt` — preset list
