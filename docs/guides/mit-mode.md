---
title: MIT Mode
description: The tightly-packed single-frame CAN protocol used in MIT mode.
---

MIT mode is designed to be simple and compact: it uses a single CAN frame with tightly
packed fields. The motor address is sent as the first 11 bits.

:::note
MIT mode is send and receive. When we fire off a command MIT mode fires back with its information.
:::

## Enabling MIT mode

Send the following payloads (prefixed with `{ADDRESS_BIT_HIGH, ADDRESS_BIT_LOW}`):

| Action                    | Payload                                   |
| ------------------------- | ----------------------------------------- |
| Enter motor control mode  | `0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFC` |
| Exit motor control mode   | `0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFD` |
| Set current position to 0 | `0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFE` |

## Send frame

| Byte      | Field                                    |
| --------- | ---------------------------------------- |
| `Data[0]` | Motor position high                      |
| `Data[1]` | Motor position low                       |
| `Data[2]` | Motor speed high                         |
| `Data[3]` | Motor speed low `[7:4]`, KP high `[3:0]` |
| `Data[4]` | KP low                                   |
| `Data[5]` | KD high                                  |
| `Data[6]` | KD low `[7:4]`, current high `[3:0]`     |
| `Data[7]` | Current low                              |

Motor position is an `int16`; every other field is an `int12`.

## Response frame

| Byte      | Field                                         |
| --------- | --------------------------------------------- |
| `Data[0]` | Drive (motor CAN) ID                          |
| `Data[1]` | Motor position high                           |
| `Data[2]` | Motor position low                            |
| `Data[3]` | Motor speed high                              |
| `Data[4]` | Motor speed low `[7:4]`, current high `[3:0]` |
| `Data[5]` | Current low                                   |
| `Data[6]` | Motor temperature                             |
| `Data[7]` | Motor error byte                              |

<!-- TODO: document scaling factors and value ranges for each field -->
