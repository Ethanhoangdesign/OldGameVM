# Handoff — Strategic map time-compression slowdown

## Trạng thái ngày 2026-08-27

Đã sửa và xác nhận trên Android: khi bật `5 min`, strategic map không còn chậm nghiêm trọng; nút mũi tên phải tiếp tục đổi sang `30 min` và `60 min` bình thường.

## Nguyên nhân

Logic nút đã đúng:

```text
Paused → 5 min → 30 min → 60 min
```

`5 min` tương ứng 300 giây game mỗi giây thực. Trong time compression, `HandleTacticalEndTurn()` được gọi thường xuyên. Kiểm tra elapsed time trong `src/game/Tactical/Tactical_Turns.cc` bị đảo chiều:

```cpp
uiTimeSinceLastStrategicUpdate - now > 1200
```

Hai toán hạng là `UINT32`; khi `now` lớn hơn timestamp cũ, phép trừ underflow. `HandleRottingCorpses()` vì thế chạy gần như mỗi tactical maintenance tick, làm nghẽn main thread và khiến `POINTER_UP` của nút khó được xử lý.

## Thay đổi

`src/game/Tactical/Tactical_Turns.cc`:

```cpp
if (now - uiTimeSinceLastStrategicUpdate > 1200)
```

Không đổi:

- callback hoặc hitbox nút time compression;
- compression rates;
- strategic event ordering;
- SDL main loop;
- tactical AI behavior/instrumentation.

## Verification

Passed:

```text
git diff --check
cmake --build build --target ja2 -j$(sysctl -n hw.logicalcpu)
./tools/build-android-relwithdebinfo.sh
```

Android APK được tạo tại:

```text
android/app/build/outputs/apk/release/app-release.apk
```

Device verification do người dùng xác nhận:

- `5 min` chạy mượt;
- có thể bấm tiếp sang mức compression cao hơn;
- chưa cần tăng hitbox nút.

## Follow-up

Không còn việc bắt buộc. Chỉ điều tra hitbox Android riêng nếu một thiết bị khác vẫn khó bấm khi game đã chạy mượt.
