---
title: Code Overview
description: A deep dive into the AK Series driver library layout and APIs.
---

This section documents the library's source layout and the APIs that interface with each
other. The goal is to keep each layer as separate as possible — CAN framing, protocol
encoding, and motor limits each live in their own headers.

## Source layout

```
include/
├── AK_Series.hpp        # top-level public header
├── Exceptions.hpp       # shared exception types
├── IO.hpp               # CAN I/O (socket send/receive)
├── can/                 # raw CAN frame types
│   ├── frame.hpp        # base Frame
│   ├── MIT_frame.hpp
│   ├── Servo_frame.hpp
│   └── send.hpp
├── motors/
│   └── MotorLimits.hpp  # per-motor limit definitions
└── protocol/            # protocol encoding/decoding
    ├── Exceptions.hpp
    ├── MIT.hpp
    └── servo.hpp
src/
└── can/
    ├── MIT_frame.cpp
    └── Servo_frame.cpp
```

## Layers

| Layer | Where | What it does |
| --- | --- | --- |
| [CAN frames](../can-layer/) | `include/can`, `src/can` | Builds and converts raw `can_frame`s |
| [Protocol](../protocol-layer/) | `include/protocol` | Encodes/decodes Servo and MIT commands |
| [Motors](../motors/) | `include/motors` | Per-motor limits and constraints |

<!-- TODO: expand once the public API in AK_Series.hpp is implemented -->
