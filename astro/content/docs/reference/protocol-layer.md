---
title: Protocol Layer
description: Servo and MIT command encoding/decoding and protocol exceptions.
---

The protocol layer translates high-level intents (set position, set velocity, …) into the
byte layouts described for each protocol in their respective files [MIT Frames](../guides/mit-mode/) and [Servo Frames](../guides/servo-mode/) guide, and decodes
responses back into typed values.

## Servo — `include/protocol/servo.hpp`

<!-- TODO: document the Servo command IDs and encode/decode functions -->

## MIT — `include/protocol/MIT.hpp`

<!-- TODO: document MIT field packing (int16 position, int12 fields) and scaling -->

## Exceptions — `include/protocol/Exceptions.hpp`

<!-- TODO: document protocol-level error types and the motor error byte mapping -->
