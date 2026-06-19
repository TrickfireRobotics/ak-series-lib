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

class ServoSendFrame : public Frame {
private:
  ServoFrameID mServoID;
  ServoSendFrame(canid_t id, ServoFrameID frame_id);
  uint32_t mFrameHeader{};
  uint8_t getId();

public:
  // when we read, we implicitly assume that it will be a canframe we are pulling this from
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

#endif
