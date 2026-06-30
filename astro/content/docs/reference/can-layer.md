---
title: CAN Layer
description: The raw CAN frame types and socket I/O.
---

The CAN layer wraps Linux SocketCAN. It owns the raw `can_frame` representation and the
conversion from typed frame objects into bytes on the wire.

## `Frame` — `include/can/frame.hpp`

The base `Frame` holds the CAN ID, a fixed 8-byte data buffer, and a length. Every frame
type is convertible to a `can_frame` via `explicit operator can_frame()`.

```cpp
class Frame {
protected:
  canid_t mCanId;
  const uint8_t len{8};
  std::array<uint8_t, 8> mData{};
public:
  Frame(canid_t id);
  virtual explicit operator can_frame();
};
```

## `Servo_frame`

##### `include/can/Servo_frame.hpp`, `src/can/Servo_frame.cpp`

The Servo frame class is split into two different classes, `ServoSendFrame` and `ServoRecvFrame`.

`ServoSendFrame` works similarly to `Frame` where it is directly convertible to a `can_frame` object specificly for sending down the canline.

```cpp
class ServoSendFrame : public Frame {
private:
  ServoFrameID mServoID;
  ServoSendFrame(canid_t id, ServoFrameID frame_id);
  uint32_t mFrameHeader{};
  uint8_t getId();

public:
  ServoSendFrame(can_frame frame);
  [[nodiscard]] explicit operator can_frame();
  [[nodiscard]] static ServoSendFrame setDutyCycle(canid_t can_id, float dutyCycle);
  [[nodiscard]] static ServoSendFrame setCurrentLoop(canid_t can_id, float currentLoop);
  [[nodiscard]] static ServoSendFrame setCurrentBrake(canid_t can_id, float current);
  [[nodiscard]] static ServoSendFrame setRPM(canid_t can_id, float rpm);
  [[nodiscard]] static ServoSendFrame setPosition(canid_t can_id, float pos);
  [[nodiscard]] static ServoSendFrame setOrigin(canid_t can_id, uint8_t origin_mode);
  [[nodiscard]] static ServoSendFrame setPositionAndVelo(canid_t, float position, float speed,
                                                         float accel);
};
```

We treat `ServoSendFrame` as a factory for our can frames, we pass it the constructors and it orders the memory as specifically needed for the protocol.
Example code

```cpp
//Create our can frame
auto result = ServoSendFrame::setRPM(11, 1000);
//Example send API
sendCanFrame(static_cast<can_frame>(result));
```

`ServoRecvFrame` works similarly to `ServoSendFrame` but instead of creating it with our parameters we pass a `can_frame` struct to it to extract the information from it.
:::note
ServoRecvFrame will not construct if the can_id field doesnt contain `0x29` in bits 29->21
:::

```cpp
class ServoRecvFrame : public Frame {
private:
  ServoFrameID mServoID;
  ServoRecvFrame(canid_t id, ServoFrameID frame_id);
  uint32_t mFrameHeader{};

public:
  uint8_t getId();
  ServoRecvFrame(const can_frame &frame);
  [[nodiscard]] explicit operator can_frame();
  float getPosition();
  int32_t getSpeed();
  float getCurrent();
  int8_t getTemperature();
  ErrorCode getErrorCode();
};
```

Each of the function headers describe what value we are grabbing from the received can frame, it grabs each value from the specific spots in the laid out memory.

```cpp

```

## `MIT_frame`

##### `include/can/MIT_frame.hpp`, `src/can/MIT_frame.cpp`

Like the Servo frame, the MIT frame is split into `MitSendFrame` and `MitRecvFrame`. MIT mode
packs floating point control values (position, speed, current, and the `KP`/`KD` gains) into the
8-byte CAN payload by quantizing each value into a fixed number of bits. Because the valid range of
each field depends on the motor, both classes take an `AkSeriesMotors` enum and look up the
matching `MotorRunLimits` (see `include/motors/MotorLimits.hpp`).

### `MitRunSettings`

The control values for a single MIT command are grouped into a `MitRunSettings` struct:

```cpp
typedef struct MitRunSettings {
  float position{}; // rads
  float speed{};    // rads/s
  float current{};  // amps
  float KP{};       // position gain
  float KD{};       // velocity gain
  MitRunSettings(float pos, float sp, float curr, float P, float D);
  MitRunSettings(); // all fields zeroed
} MitRunSettings;
```

### `MitSendFrame`

```cpp
class MitSendFrame : public Frame {
private:
  AkSeriesMotors mMotor{};
  const MotorRunLimits &mLims;
  void padData();

public:
  MitSendFrame() = delete;
  MitRunSettings *mSettings{nullptr};
  explicit MitSendFrame(canid_t can_id, AkSeriesMotors motor,
                        MitRunSettings *settings = nullptr);
  explicit operator can_frame();
};
```

`MitSendFrame` is constructed with the target CAN ID, the motor model, and (optionally) a pointer to
a `MitRunSettings`. If no settings are provided, a zeroed `MitRunSettings` is allocated. When the
frame is cast to a `can_frame`, `padData()` quantizes each field and bit-packs it into the 8 data
bytes:

```cpp
void MitSendFrame::padData() {
  MotorRunLimits mLims = motorRunLimits[static_cast<uint8_t>(mMotor)];

  uint16_t pos     = float_to_uint(mSettings->position, -mLims.pos, mLims.pos, 16);
  uint16_t speed   = float_to_uint(mSettings->speed, -mLims.speed, mLims.speed, 12);
  uint16_t current = float_to_uint(mSettings->current, -mLims.torque, mLims.torque, 12);
  uint16_t KP      = float_to_uint(mSettings->KP, 0, mLims.KPMax, 12);
  uint16_t KD      = float_to_uint(mSettings->KD, 0, mLims.KDMax, 12);

  mData[0] = pos >> 8;                                   // position high 8 bits
  mData[1] = pos;                                        // position low 8 bits
  mData[2] = speed >> 4;                                 // speed high 8 bits
  mData[3] = ((speed & LOW4BITS) << 4) | ((KP >> 8) & LOW4BITS); // speed low 4 | KP high 4
  mData[4] = KP;                                         // KP low 8 bits
  mData[5] = KD >> 4;                                    // KD high 8 bits
  mData[6] = ((KD & LOW4BITS) << 4) | ((current >> 8) & LOW4BITS); // KD low 4 | current high 4
  mData[7] = current;                                    // current low 8 bits
}
```

The packing follows the AK-series MIT protocol: position uses a full 16 bits, while speed, current,
`KP`, and `KD` each use 12 bits, sharing the boundary bytes (`mData[3]` and `mData[6]`) between two
fields. `float_to_uint` clamps each value to the motor's range before scaling it across the
available bits.

Example code

```cpp
// Position 1.0 rad, speed 0, current 0, KP 50, KD 1
MitRunSettings settings{1.0f, 0.0f, 0.0f, 50.0f, 1.0f};
MitSendFrame frame{11, AkSeriesMotors::AK80_9, &settings};
// Example send API
sendCanFrame(static_cast<can_frame>(frame));
```

Before sending control frames the motor must be put into MIT mode. `beginMitMode(canid_t can_id)`
sends the special enter-control-mode frame (`0xFF … 0xFC`); the protocol also defines exit
(`0xFD`) and zero-position (`0xFE`) commands.

### `MitRecvFrame`

```cpp
class MitRecvFrame : public Frame {
private:
  AkSeriesMotors mMotor;
  const MotorRunLimits &mLims;

public:
  MitRecvFrame() = delete;
  MitRecvFrame(const can_frame &frame, AkSeriesMotors motor);
  uint8_t getId();
  float getPosition();
  float getCurrent();
  int8_t getTemperature();
  ErrorCode getErrorCode();
  explicit operator can_frame();
};
```

`MitRecvFrame` is built from a received `can_frame` plus the motor model so it can apply the correct
limits when unpacking. Each accessor pulls its value out of the laid-out payload and, for the
analog fields, reverses the quantization with `uint_to_float`:

- `getId()` — controller/motor id from `mData[0]`.
- `getPosition()` — 28-bit position rebuilt from `mData[1..4]` (shifted right by 4), scaled back to
  `[-pos, pos]` radians.
- `getCurrent()` — 12-bit current from the low nibble of `mData[4]` and `mData[5]`, scaled to
  `[-torque, torque]`.
- `getTemperature()` — signed temperature in `mData[6]`.
- `getErrorCode()` — `ErrorCode` enum from `mData[7]`.

## `send` — `include/can/send.hpp`

Incomplete file, to be included when it will be finished
