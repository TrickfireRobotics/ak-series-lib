#ifndef __SERVO_AK_SERIES
#define __SERVO_AK_SERIES
#pragma once

#include "Exceptions.hpp"
#include "frame.hpp"
#include <stdexcept>
#include <stdint.h>

enum class ServoFrameID : uint8_t {
  // int32_t(Set duty cycle voltage)
  DutyCycleMode = 0,
  // int32_t(Set current)
  CurrentLoopMode,
  // int32_t(How much current to brake)
  CurrentBrakeMode,
  // int32_t (Velocity value)
  RPMmode,
  // int32_t (Position value)
  PositionMode,
  // uint8_t (Send to origin value)
  //  0 for setting current position to temp value
  //  1 for setting the permanent 0 point (only on dual encoder devices)
  SetOriginMode,
  // int32_t(Position value), int16_t (speed value), int16_t (acceleration value)
  PositionVelocityMode,
  // This is the message servo mode screams back at you
  // int16_t (position), int16_t (speed), int16)_t (current), uint8_t (motor temperature),
  // uint8_t(error code)
  ServoModeFeedback = 0x29
};

class ServoFrame : public Frame {
private:
  ServoFrameID m_servo_id;
  ServoFrame(canid_t id, ServoFrameID frame_id);
  uint32_t m_frame_header{};
  uint8_t get_id();
  bool is0x29();

public:
  // when we read, we implicitly assume that it will be a canframe we are pulling this from
  ServoFrame(can_frame frame);
  [[nodiscard]] explicit operator can_frame();
  [[nodiscard]] static ServoFrame setDutyCycle(canid_t can_id, float dutyCycle);
  [[nodiscard]] static ServoFrame setCurrentLoop(canid_t can_id, int32_t currentLoop);
  [[nodiscard]] static ServoFrame setCurrentBrake(canid_t can_id, int32_t current);
  [[nodiscard]] static ServoFrame setRPM(canid_t can_id, int32_t rpm);
  [[nodiscard]] static ServoFrame setPosition(canid_t can_id, int32_t pos);
  [[nodiscard]] static ServoFrame setOrigin(canid_t can_id, uint8_t origin_mode);
  [[nodiscard]] static ServoFrame setPositionAndVelo(canid_t, int32_t position, int16_t speed,
                                                     int16_t accel);
  // If the frame was constructed with the wrong can_frame struct
  // This will throw, the command id needs to be 0x29
  float getPosition();
  int32_t getSpeed();
  float getCurrent();
  int8_t getTemperature();
  ErrorCode getErrorCode();
};

class ServoMsgFrame : public Frame {
private:
  ServoFrameID m_servo_id;
  ServoMsgFrame(canid_t id, ServoFrameID frame_id);
  uint32_t m_frame_header{};
  uint8_t get_id();

public:
  ServoMsgFrame(can_frame frame);
  [[nodiscard]] explicit operator can_frame();
  float getPosition();
  int32_t getSpeed();
  float getCurrent();
  int8_t getTemperature();
  ErrorCode getErrorCode();
};

#endif
