#ifndef __MOTOR_LIMITS_AK_SERIES
#define __MOTOR_LIMITS_AK_SERIES
#pragma once

#include <stdint.h>
struct MitRunSettings {
  float position{};
  float speed{};
  float current{};
  float KP{};
  float KD{};
  MitRunSettings(float pos, float sp, float curr, float P, float D)
      : position{pos}, speed{sp}, current{curr}, KP{P}, KD{D} {}
  MitRunSettings() : position{0.0f}, speed{0.0f}, current{0.0f}, KP{0.0f}, KD{0.0f} {}
};

// Each motor has different max run settings
enum class AKSeriesMotor : uint8_t {
  AK10_9 = 0,
  AK60_6,
  AK70_10,
  AK80_6,
  AK80_9,
  AK80_64,
  AK80_8,
  AK45_36,
  AK45_10,
  AK40_10,
  COUNT // Use in loops for the amount of motors
};

// Values represent both positive and negative bounds, since they arent different according to
// hardware
typedef struct MotorRunLimits {
  float pos{12.5f};
  float speed{};
  float torque{};
  float KPMax{500.0f};
  float KDMax{5.0f};
} MotorRunLimits;

static const MotorRunLimits motorRunLimits[] = {
    {.speed = 50.0f, .torque = 65.0f}, {.speed = 45.0f, .torque = 15.0f},
    {.speed = 50.0f, .torque = 25.0f}, {.speed = 76.0f, .torque = 12.0f},
    {.speed = 50.0f, .torque = 18.0f}, {.speed = 8.0f, .torque = 144.0f},
    {.speed = 37.5f, .torque = 32.0f}, {.speed = 6.0f, .torque = 34.0f},
    {.speed = 20.0f, .torque = 8.0f},  {.speed = 45.5f, .torque = 5.0f}};

#endif
