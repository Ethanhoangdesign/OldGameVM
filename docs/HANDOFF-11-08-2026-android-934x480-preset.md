# HANDOFF — 11/08/2026

**Thêm preset `934x480` lên đầu danh sách resolution**

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
| Thêm preset `934x480` lên đầu PRESETS | DONE | Đứng trên `1366x768` |
| Build APK verify | DONE | `./gradlew installDebug` |
| Live emulator test | TODO | Chọn preset, xác nhận game khởi động và map rộng ra |

---

## 1. BỐI CẢNH

Sau khi đã có 3 preset widescreen (`1280x600`, `1280x480`, `1024x600`), người dùng muốn thêm `934x480` — widescreen hẹp hơn `1280` nhưng vẫn rộng hơn `800x600`, và để preset này **lên đầu danh sách** để dễ chọn nhất.

`934x480` phù hợp với màn hình emulator có chiều ngang ~934dp — tận dụng toàn bộ chiều ngang mà không bị letterbox, đồng thời chiều cao 480 là sàn cứng của engine.

---

## 2. THAY ĐỔI

### `android/app/src/main/java/io/github/ja2stracciatella/ConfigurationModel.kt`

```kotlin
val PRESETS = listOf(
    Resolution(934u, 480u),  // widescreen at minimum height, narrower than 1280  ← MỚI, đầu danh sách
    Resolution(1366u, 768u), // mobile-recommended 16:9 HD (Big Map)
    Resolution(1280u, 768u), // mobile-recommended 5:3 HD (Big Map)
    Resolution(1024u, 768u),
    Resolution(1280u, 600u), // widescreen at 600 height — same zoom as 800x600
    Resolution(1280u, 480u), // widescreen at minimum height
    Resolution(1024u, 600u), // medium-wide at 600 height — same zoom as 800x600
    Resolution(800u, 600u),
    Resolution(640u, 480u),  // original 4:3
    Resolution(1664u, 768u)  // mobile-only ultra-wide
)
```

Map viewport của `934x480`:

| Preset | Map viewport | So sánh với 800x600 |
|---|---|---|
| `800x600` | 800 × 480 | Gốc |
| `934x480` | 934 × 360 | +17% rộng hơn, chiều cao tối thiểu |

---

## 3. BUILD & INSTALL

```sh
cd android && ./gradlew installDebug
```

Mở app, vào **Settings → Internal Resolution**, xác nhận `934x480` hiện ở đầu danh sách.

---

## 4. VERIFY

Checklist:
1. Chọn `934x480` → game mở được, không crash.
2. Map rộng hơn `800x600`, nhân vật cùng kích thước hoặc lớn hơn.
3. UI bar dưới (team slots, radar, clock) hiển thị đúng.

Nếu crash, lấy log:

```sh
adb logcat | grep -i "fatal\|backtrace\|libja2\|SIGSEGV"
```

---

## 5. LIÊN QUAN

- `docs/HANDOFF-11-08-2026-android-widescreen-600-presets.md` — 3 preset widescreen trước đó (`1280x600`, `1280x480`, `1024x600`)
- `docs/HANDOFF-11-08-2026-android-800x600-crash.md` — crash fix cho `800x600`
- `src/game/UILayout.cc` — viewport calculation (`m_VIEWPORT_END_X`, `m_VIEWPORT_END_Y`)
- `android/app/src/main/java/io/github/ja2stracciatella/ConfigurationModel.kt` — preset list
