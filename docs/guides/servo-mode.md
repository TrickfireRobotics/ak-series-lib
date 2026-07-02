---
title: Servo Mode
description: The extended CAN ID encoding and payload structure used in Servo mode.
---

Servo mode uses the **extended (29-bit) CAN ID** to encode both the command and the target
motor. It is based on the VESC open motor architecture.

## Extended ID layout

- Bits `[28:8]` — command ID (e.g. `4` = position control, sent as `4 << 8`)
- Bits `[7:0]` — motor CAN ID (e.g. motor `1` = `1`)

:::note
ID bits can be stored in one continuous `int32`. You do not need to shift the entire CAN
frame; the hardware strips the empty bits.
:::

The remaining payload (up to 7 bytes) carries data or control bits depending on the command.

## Enabling Servo mode

Servo mode requires the CubeMars Servo firmware. Using the CubeMars tool:

1. Connect the motor to your computer with the R-LINK and open the CubeMars software.
2. Open the **Mode Switch** tab.
3. Click **"Servo App"** (bottom right) to command the board into Servo mode.

:::tip
Servo mode can be configured in firmware to report its current position at a set rate
(between 0 and 500).
:::

<!-- TODO: link to the relevant manual section and add screenshots -->

:::caution
In Servo mode the motor replies continuously, not strictly one reply per send. Account for
this when designing the controller.
:::

# Servo Mode frames

---

## Command ID 0 — Duty Cycle Mode

Sets a duty cycle voltage. `int32_t`, scale float duty by `100000.0` (range 0 to 100,000).
Uses 4 bytes (`Data[0]` high → `Data[3]` low), pad `Data[4:7]` with `0x00`.

## Command ID 1 — Current Loop Mode

Sets the Iq current (torque control). `int32_t`, scale Amps by `1000.0`
(range -60,000 to 60,000). 4 bytes, pad the rest.

## Command ID 2 — Current Brake Mode

Applies a holding brake current. `int32_t`, scale Amps by `1000.0` (range 0 to 60,000).
4 bytes, pad the rest.

## Command ID 3 — Velocity Loop Mode

Sets a target speed in ERPM. `int32_t`, send ERPM directly (range -100,000 to 100,000).
4 bytes, pad the rest.

## Command ID 4 — Position Loop Mode

Moves to a target angle at max speed. `int32_t`, scale degrees by `10000.0`
(range -360,000,000 to 360,000,000). 4 bytes, pad the rest.

## Command ID 5 — Set Origin Mode

Sets the current position as the zero origin. `uint8_t` in `Data[0]`:
`0x00` = temporary, `0x01` = permanent (dual-encoder models only). Pad `Data[1:7]`.

## Command ID 6 — Position-Velocity Loop Mode

Moves to a target position within speed/acceleration limits. Fully packed (8 bytes):

| Bytes       | Field              | Type / Scaling                 |
| ----------- | ------------------ | ------------------------------ |
| `Data[0:3]` | Position           | `int32_t`, degrees × `10000.0` |
| `Data[4:5]` | Speed limit        | `int16_t`, ERPM ÷ `10.0`       |
| `Data[6:7]` | Acceleration limit | `int16_t`, ERPM/s² ÷ `10.0`    |

## Command ID 0x29 - Servo Mode Response Frame

:::caution
This can frame is RECEIVE ONLY, do not try to send this can frame as it will just be rejected this is how Servo mode replies back with its position.
:::

| Bytes       | Field    | Type / Scaling              |
| ----------- | -------- | --------------------------- |
| `Data[0:1]` | Position | `int16_t`, degrees / `10.0` |
| `Data[2:3]` | Speed    | `int16_t`, ERPM * `10`      |
| `Data[4:5]` | Current  | `int16_t`, Amps / `100.0`   |

<!-- TODO: keep this in sync with docs/Motor-can-frames.md and the manufacturer manual -->
