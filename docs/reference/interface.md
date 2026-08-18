---
title: Motor Interface
description: The public AKSeriesInterface entry point and the Motor binding classes.
---

`include/AKSeries.hpp`, `src/AKSeries.cpp`

`AKSeriesInterface` is the top-level public API. It owns the CAN socket for one physical bus and
hands out `Motor` objects — `MitModeMotor` or `ServoModeMotor` — bound to a specific CAN id. Each
`Motor` shares the interface's underlying `CanInterface` (see [CAN Layer](../can-layer/)), so all
motors created from the same `AKSeriesInterface` talk over the same socket.

## `AKSeriesInterface`

```cpp
class AKSeriesInterface {
public:
  explicit AKSeriesInterface(const char *canif);

  MitModeMotor createMitMotor(const AKSeriesMotor, uint32_t canId);
  MitModeMotor createMitMotor(const MotorRunLimits *, uint32_t canId);
  ServoModeMotor createServoMotor(const AKSeriesMotor, uint32_t canId);
  ServoModeMotor createServoMotor(const MotorRunLimits *, uint32_t canId);

  std::optional<ServoRecvFrame> readServoFrame();

  AKSeriesInterface(const AKSeriesInterface &);
  AKSeriesInterface &operator=(const AKSeriesInterface &);
  AKSeriesInterface(AKSeriesInterface &&) noexcept;
  AKSeriesInterface &operator=(AKSeriesInterface &&) noexcept;
  ~AKSeriesInterface();
};
```

### Construction

`AKSeriesInterface(canif)` opens and binds a CAN socket on `canif` (e.g. `"can0"`), the same way
`CanInterface` does. Create one `AKSeriesInterface` per physical CAN bus.

### Creating motors

Servo mode and MIT mode are exposed as distinct types since the protocol and available commands
are completely different — see [Servo Mode](../../guides/servo-mode/) and
[MIT Mode](../../guides/mit-mode/):

- `createMitMotor(...)` returns a `MitModeMotor`.
- `createServoMotor(...)` returns a `ServoModeMotor`.

Each has two overloads:

- Pass an `AKSeriesMotor` enum value to look up that model's limits from the built-in
  `motorRunLimits` table (see [Motor Limits](../motors/)).
- Pass a `const MotorRunLimits *` to supply custom limits instead (e.g. a derated motor, or a
  model that isn't in the table).

:::caution
The `MotorRunLimits *` overload stores the pointer — it does not copy the struct. The
`MotorRunLimits` object must outlive every `Motor` created from it.
:::

Passing `nullptr` as the limits pointer throws `std::invalid_argument`.

```cpp
AKSeriesInterface iface("can0");

// Table-driven limits
MitModeMotor knee = iface.createMitMotor(AKSeriesMotor::AK80_9, 1);

// Custom limits — hipLimits must stay alive as long as `hip` is used
MotorRunLimits hipLimits{.speed = 20.0f, .torque = 12.0f};
ServoModeMotor hip = iface.createServoMotor(&hipLimits, 2);
```

### Reading servo feedback

Servo mode motors reply continuously and aren't tied to a specific `send*()` call (see
[Servo Mode](../../guides/servo-mode/)), so feedback is read from the interface, not from a
`ServoModeMotor`:

```cpp
std::optional<ServoRecvFrame> feedback = iface.readServoFrame();
if (feedback) {
  float pos = feedback->getPosition();
}
```

Returns `std::nullopt` on a socket timeout or read error, the same convention as
`CanInterface::read()`.

### Lifetime

`AKSeriesInterface` holds the CAN socket behind a `std::shared_ptr`; every `Motor` created from it
holds a copy of that pointer, so the socket stays alive as long as any `Motor` (or another
`AKSeriesInterface` handle) still references it.

:::note
The destructor's bookkeeping — refusing to tear down while `Motor` objects are still alive — is
being reworked, so don't rely on it catching a dangling `Motor` today.
:::

Copy and move construction/assignment are declared so multiple `AKSeriesInterface` handles can
share one underlying bus, but the definitions aren't implemented yet.

## `Motor`

`Motor` is the common base of `MitModeMotor` and `ServoModeMotor`. It's move-only — movable, not
copyable — and can't be constructed directly; you always get one from
`AKSeriesInterface::createMitMotor()` / `createServoMotor()`.

## `MitModeMotor`

```cpp
class MitModeMotor : public Motor {
public:
  [[nodiscard]] std::optional<MitRecvFrame> sendAndRecieve(MitRunSettings &);
  void send(MitRunSettings &);
};
```

- `sendAndRecieve()` — the normal way to drive an MIT-mode motor: encodes a `MitSendFrame` from
  the settings and this motor's limits, sends it, then blocks for the motor's reply and decodes it
  into a `MitRecvFrame`. Returns `std::nullopt` on a socket error or timeout.
- `send()` — fire-and-forget: sends the frame and doesn't wait for or consume a reply. Use this
  when you don't need this cycle's feedback, e.g. commanding several motors before reading any of
  them back.

```cpp
MitRunSettings settings{1.0f, 0.0f, 0.0f, 50.0f, 1.0f}; // pos, speed, current, KP, KD
if (auto reply = knee.sendAndRecieve(settings)) {
  float pos = reply->getPosition();
}
```

## `ServoModeMotor`

```cpp
class ServoModeMotor : public Motor {
public:
  void sendDutyCycle(float dutyCycle);
  void sendCurrentLoop(float currentLoop);
  void sendCurrentBrake(float current);
  void sendRPM(float rpm);
  void sendPosition(float pos);
  void sendOrigin(uint8_t origin_mode);
  void sendPositionAndVelo(float position, float speed, float accel);
};
```

Each method builds the matching `ServoSendFrame` (see [CAN Layer](../can-layer/) /
[Servo Mode](../../guides/servo-mode/)) and sends it. On top of the protocol-level ranges
`ServoSendFrame` itself enforces, these bindings additionally clamp the argument to this motor's
`MotorRunLimits` before encoding, logging a warning to `stderr` when clamping happens:

| Method                | Clamped to                                                                                                                       |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| `sendDutyCycle`       | `±limits.torque`                                                                                                                 |
| `sendCurrentLoop`     | `±limits.torque`                                                                                                                 |
| `sendCurrentBrake`    | `±limits.torque` at the binding level, then floored to `0` if still negative (current brake is protocol-defined as non-negative) |
| `sendRPM`             | `±limits.speed`                                                                                                                  |
| `sendPosition`        | `±limits.pos`                                                                                                                    |
| `sendPositionAndVelo` | position → `±limits.pos`, speed → `±limits.speed`; acceleration is **not** clamped by this layer                                 |
| `sendOrigin`          | not clamped — only `0`/`1` are valid; anything else throws `std::invalid_argument` from `ServoSendFrame::setOrigin`              |

:::caution
`sendDutyCycle` clamps against `limits.torque` (an Amps limit), not a dedicated duty-cycle bound —
there isn't one in `MotorRunLimits` yet. Treat this as a placeholder.
:::

```cpp
hip.sendRPM(15.0f);      // clamped to ±hipLimits.speed
hip.sendPosition(90.0f); // clamped to ±hipLimits.pos
```

<!-- TODO: keep the clamp table in sync with src/AKSeries.cpp as the limit-checking logic evolves -->
