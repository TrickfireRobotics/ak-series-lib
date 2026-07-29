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

The memory ordering for these is fairly straightforward.
`setDutyCycle()` Uses the float as a 32 bit integer and uses the first 4 bytes to pad the data.

```cpp
  f.mData[0] = static_cast<uint8_t>(duty >> 24);
  f.mData[1] = static_cast<uint8_t>(duty >> 16);
  f.mData[2] = static_cast<uint8_t>(duty >> 8);
  f.mData[3] = static_cast<uint8_t>(duty);
```

All of the can frames with one line follow this same pattern all of the values are padded to the front.
`setCurrentLoop()` uses an int32_t
`setCurrentBrake` uses an int32_t
`setRPM()` uses an int32_t
`setPosition()` uses an int32_t

`setOrigin()` uses an uint8_t, but `setOrigin()` is a special case command which sets the current position of the motor to the 0 position. This is the only can frame that will throw if passed an invalid argument since this specifically needs the value to be 1 or 0

`setPositionAndVelo()` Has a few values, but the padding is still pretty straightforward

```cpp
  f.mData[0] = static_cast<uint8_t>(posReal >> 24);
  f.mData[1] = static_cast<uint8_t>(posReal >> 16);
  f.mData[2] = static_cast<uint8_t>(posReal >> 8);
  f.mData[3] = static_cast<uint8_t>(posReal);

  f.mData[4] = static_cast<uint8_t>(speedReal >> 8);
  f.mData[5] = static_cast<uint8_t>(speedReal);

  f.mData[6] = static_cast<uint8_t>(accelReal >> 8);
  f.mData[7] = static_cast<uint8_t>(accelReal);
```

Similar pattern, just pads with more values.

Example Code

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
each field depends on the motor, both classes take an `AKSeriesMotor` enum and look up the
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
  AKSeriesMotor mMotor{};
  const MotorRunLimits &mLims;
  void padData();

public:
  MitSendFrame() = delete;
  MitRunSettings *mSettings{nullptr};
  explicit MitSendFrame(canid_t can_id, AKSeriesMotor motor,
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
MitSendFrame frame{11, AKSeriesMotor::AK80_9, &settings};
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
  AKSeriesMotor mMotor;
  const MotorRunLimits &mLims;

public:
  MitRecvFrame() = delete;
  MitRecvFrame(const can_frame &frame, AKSeriesMotor motor);
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

## `comms` — `include/can/comms.hpp`

The comms file contains one class. The `CanInterface` class which manages interfacing with the actual canline.

#### Member functions

```cpp
[[nodiscard]] Expected<can_frame, CanIOError> sendAndRead(can_frame &frame);
CanIOError send(can_frame &frame);
Expected<can_frame, CanIOError> read();
void setPollTimeout(uint32_t pTime) { pollTime = pTime; };
```

`sendAndRead` and `read` both use a small struct inside our `Errors.hpp` file.

```cpp
template <typename T, typename U> struct Expected {
  union {
    T success;
    U error;
  };
  bool successful;
  Expected(T val) : success{val}, successful{true} {};
  Expected(U val) : error{val}, successful{false} {};
};
```

If the object successfully reads it returns the successful can frame we read from the socket. If the object fails to read from the canline then it wilreturn the appropriate value from the `CanIOError` enum.

```cpp
enum class CanIOError {
  TIMEOUT = 0,
  SOCKET_ERROR,
  BIND_ERROR,
  FAILED_WRITE,
  FAILED_READ,
  NO_DATA,
  BUSY = 16,
  RESOURCE_UNAVAILABLE = 35,
  NO_BUFFER_SPACE = 55,
  NONE,
};
```

All of these values are descriptive of what is happening on the canline.

If a write fails you will typically see these 3 errors `BUSY` `RESOURCE_UNAVAILABLE` or `NO_BUFFER_SPACE`, these are all pulled from the socketcan errors.

#### Example usage of a writer object

```cpp
auto interface = CanWriter("can0");
auto recv =  interface.sendAndRead(f);
//Fake functions
if (recv.succesful) {
    doSomething(recv.success);
} else {
    logError(recv.error);
}

//Fire and forget the canframe
interface.send(f);

//Read the can frame
auto frame = interface.read();
if (recv.succesful) {
    doSomething(recv.success);
} else {
    logError(recv.error);
}
//Delete the interface at the end of the control block
delete interface;
```
