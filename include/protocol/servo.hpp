#ifndef __MIT_AK_SERIES
#pragma once
#include <stdint.h>

enum class ServoHeader : uint16_t {
  DutyCycleMode = 0,
  CurrentLoopMode = 1,
  CurrentBrakeMode = 2,
  RPMmode = 3,
  PositionMode = 4,
  SetOriginMode = 5,
  PositionVelocityLoopMode = 6, // int32_t(Position),int16_t(Speed),int16_t(Acceleration)
  ServoModeReply = 0x29 // Never send this value, this is specifically the position configuration
};

#endif
