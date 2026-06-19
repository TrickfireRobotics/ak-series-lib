#include "can/Servo_frame.hpp"
#include <charconv>
#include <cstdio>
#include <limits>
#include <linux/can.h>
#include <stdexcept>

namespace {
constexpr int32_t kCurrentLoopMin = -60000;
constexpr int32_t kCurrentLoopMax = 60000;
constexpr int32_t kCurrentBrakeMax = 60000;
constexpr int32_t kRPMMax = 100000;
constexpr int32_t kPositionMin = -360000000;
constexpr int32_t kPositionMax = 360000000;
constexpr int16_t kAccelMin = 0;
constexpr int16_t kAccelMax = std::numeric_limits<int16_t>::max();
constexpr uint8_t kServoModeFeedbackId = 0x29;
constexpr float kDutyScale = 100000.0f;
constexpr float kPositionDecodeScale = 100.0f;
constexpr float kCurrentDecodeScale = 100.0f;
constexpr int32_t kSpeedDecodeScale = 10;

constexpr float fCurrentLoopMin = static_cast<float>(kCurrentLoopMin) / 1000.0f;
constexpr float fCurrentLoopMax = static_cast<float>(kCurrentLoopMax) / 1000.0f;
constexpr float fCurrentBrakeMax = static_cast<float>(kCurrentBrakeMax) / 1000.0f;
constexpr float fRPMMax = static_cast<float>(kRPMMax);
constexpr float fPositionMin = static_cast<float>(kPositionMin) / 10000.0f;
constexpr float fPositionMax = static_cast<float>(kPositionMax) / 10000.0f;
constexpr float fAccelMin = 0.0f;
constexpr float fAccelMax = static_cast<float>(kAccelMax) * 10.0f;

bool checkIs0x29(uint32_t frameHeader) { return static_cast<uint8_t>(frameHeader >> 8) == 0x29; }

void warnRounding(const char *val, float min, float max, float n) {
  std::fprintf(stderr,
               "Warning: the value you passed for %s is in between invalid bounds, the bounds were "
               "%f -> %f, you passed %f rounding\n",
               val, min, max, n);
}

} // namespace

ServoSendFrame::operator can_frame() {
  can_frame f{};
  f.can_id = static_cast<canid_t>(mFrameHeader);
  f.can_id = f.can_id | CAN_EFF_FLAG;
  f.len8_dlc = 8;
  std::copy(mData.begin(), mData.end(), f.data);
  return f;
}

ServoSendFrame::ServoSendFrame(canid_t id, ServoFrameID frame_id) : Frame(id), mServoID{frame_id} {
  mFrameHeader = (static_cast<uint32_t>(mServoID) << 8) | (static_cast<uint32_t>(id) & 0xFFu);
}

ServoSendFrame ServoSendFrame::setDutyCycle(canid_t can_id, float dutyCycle) {
  // Look in the PDF to see why this is done
  int32_t duty = static_cast<int32_t>(dutyCycle * kDutyScale);
  ServoSendFrame f(can_id, ServoFrameID::DutyCycleMode);

  f.mData[0] = static_cast<uint8_t>(duty >> 24);
  f.mData[1] = static_cast<uint8_t>(duty >> 16);
  f.mData[2] = static_cast<uint8_t>(duty >> 8);
  f.mData[3] = static_cast<uint8_t>(duty);

  return f;
}

ServoSendFrame ServoSendFrame::setCurrentLoop(canid_t can_id, float current) {
  ServoSendFrame f(can_id, ServoFrameID::CurrentLoopMode);

  int32_t currentInt = static_cast<int32_t>(current * 1000.0f);
  [[unlikely]]
  if (currentInt < -60000 || current > 60000) {
    warnRounding("current", fCurrentLoopMin, fCurrentLoopMax, current);
    currentInt = currentInt < -60000 ? -60000 : 60000;
  }

  f.mData[0] = static_cast<uint8_t>(currentInt >> 24);
  f.mData[1] = static_cast<uint8_t>(currentInt >> 16);
  f.mData[2] = static_cast<uint8_t>(currentInt >> 8);
  f.mData[3] = static_cast<uint8_t>(currentInt);

  return f;
}

ServoSendFrame ServoSendFrame::setCurrentBrake(canid_t can_id, float current) {
  ServoSendFrame f(can_id, ServoFrameID::CurrentBrakeMode);
  int32_t currentReal = static_cast<int32_t>(current * 1000.0f);

  if (currentReal < 0 || currentReal > 60000) {
    warnRounding("Current brake", 0.0f, fCurrentBrakeMax, current);
    currentReal = currentReal < 0 ? 0 : 60000;
  }

  f.mData[0] = static_cast<uint8_t>(currentReal >> 24);
  f.mData[1] = static_cast<uint8_t>(currentReal >> 16);
  f.mData[2] = static_cast<uint8_t>(currentReal >> 8);
  f.mData[3] = static_cast<uint8_t>(currentReal);

  return f;
}

ServoSendFrame ServoSendFrame::setRPM(canid_t can_id, float rpm) {
  ServoSendFrame f(can_id, ServoFrameID::RPMmode);

  int32_t rpmReal{static_cast<int32_t>(rpm)};
  if (rpmReal < -100000 || rpm > 100000) {
    warnRounding("Velocity", -fRPMMax, fRPMMax, rpm);
    rpmReal = rpmReal < -10000 ? -100000 : 100000;
  }

  f.mData[0] = static_cast<uint8_t>(rpmReal >> 24);
  f.mData[1] = static_cast<uint8_t>(rpmReal >> 16);
  f.mData[2] = static_cast<uint8_t>(rpmReal >> 8);
  f.mData[3] = static_cast<uint8_t>(rpmReal);

  return f;
}

ServoSendFrame ServoSendFrame::setPosition(canid_t can_id, float pos) {
  ServoSendFrame f(can_id, ServoFrameID::PositionMode);
  int32_t posReal = static_cast<int32_t>(pos * 10000.0f);

  if (posReal < -360000000 || posReal > 360000000) {
    warnRounding("Position", fPositionMin, fPositionMax, pos);
    posReal = posReal < -360000000 ? -360000000 : 360000000;
  }

  f.mData[0] = static_cast<uint8_t>(posReal >> 24);
  f.mData[1] = static_cast<uint8_t>(posReal >> 16);
  f.mData[2] = static_cast<uint8_t>(posReal >> 8);
  f.mData[3] = static_cast<uint8_t>(posReal);

  return f;
}

ServoSendFrame ServoSendFrame::setOrigin(canid_t can_id, uint8_t origin_mode) {
  ServoSendFrame f(can_id, ServoFrameID::SetOriginMode);
  if (origin_mode > 1) {
    throw std::invalid_argument("Invalid origin position value, please pass either 1 or 0");
  }
  f.mData[0] = origin_mode;
  return f;
}

ServoSendFrame ServoSendFrame::setPositionAndVelo(canid_t can_id, float position, float speed,
                                                  float accel) {
  ServoSendFrame f(can_id, ServoFrameID::PositionVelocityMode);

  int32_t posReal = static_cast<int32_t>(position * 10000.0f);
  int16_t speedReal = static_cast<int16_t>(speed / 10.0f);
  int16_t accelReal = static_cast<int16_t>(accel / 10.0f);

  if (position < kPositionMin || position > kPositionMax) {
    posReal = posReal < kPositionMin ? kPositionMin : kPositionMax;
    warnRounding("Position", fPositionMin, fPositionMax, position);
  }

  if (speed < -327670.0f || speed > 327670.0f) {
    warnRounding("Velocity", -fRPMMax, fRPMMax, speed);
    speedReal = speedReal < -32767 ? -32767 : 32767;
  }

  if (accel < 0.0f || accel > 327680.0f) {
    warnRounding("Acceleration", 0.0f, fAccelMax, accel);
    accelReal = accelReal < 0 ? 0 : 32767;
  }

  f.mData[0] = static_cast<uint8_t>(posReal >> 24);
  f.mData[1] = static_cast<uint8_t>(posReal >> 16);
  f.mData[2] = static_cast<uint8_t>(posReal >> 8);
  f.mData[3] = static_cast<uint8_t>(posReal);

  f.mData[4] = static_cast<uint8_t>(speedReal >> 8);
  f.mData[5] = static_cast<uint8_t>(speedReal);

  f.mData[6] = static_cast<uint8_t>(accelReal >> 8);
  f.mData[7] = static_cast<uint8_t>(accelReal);

  return f;
}

uint8_t ServoSendFrame::getId() { return static_cast<uint8_t>(mFrameHeader); }

ServoRecvFrame::ServoRecvFrame(canid_t id, ServoFrameID frame_id) : Frame(id), mServoID{frame_id} {
  mFrameHeader = (static_cast<uint32_t>(mServoID) << 8) | (static_cast<uint32_t>(id));
  if (!checkIs0x29(mFrameHeader)) {
    throw std::invalid_argument("Invalid frame header for ServoRecvFrame");
  }
}

ServoRecvFrame::ServoRecvFrame(const can_frame &frame) : Frame(frame.can_id) {
  // looks like OOB error, but if you put frame.data[7] it chops the last byte off
  std::copy(&frame.data[0], &frame.data[8], mData.begin());
}

ServoRecvFrame::operator can_frame() {
  can_frame f{};
  f.can_id = static_cast<canid_t>(mFrameHeader);
  f.can_id = f.can_id | CAN_EFF_FLAG;
  f.len8_dlc = 8;
  std::copy(mData.begin(), mData.end(), f.data);
  return f;
}

uint8_t ServoRecvFrame::getId() { return static_cast<uint8_t>(mCanId); }

float ServoRecvFrame::getPosition() {
  int16_t pos{};
  pos += static_cast<int16_t>(mData[0]) << 8;
  pos += static_cast<int16_t>(mData[1]);
  float posReal{static_cast<float>(pos)};
  posReal /= 10.0f;
  return posReal;
}

int32_t ServoRecvFrame::getSpeed() {
  int16_t speed{};
  speed += static_cast<int16_t>(mData[2]) << 8;
  speed += static_cast<int16_t>(mData[3]);

  int32_t speedReal{speed};
  speedReal *= 10;
  return speedReal;
}

float ServoRecvFrame::getCurrent() {
  int16_t current{};
  current += static_cast<int16_t>(mData[4]) << 8;
  current += static_cast<int16_t>(mData[5]);

  float currentReal{static_cast<float>(current)};
  currentReal /= 100;
  return currentReal;
}

int8_t ServoRecvFrame::getTemperature() {
  int8_t temp{static_cast<int8_t>(mData[6])};
  return temp;
}

ErrorCode ServoRecvFrame::getErrorCode() {
  ErrorCode r = static_cast<ErrorCode>(mData[7]);
  return r;
}
