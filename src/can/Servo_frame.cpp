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
} // namespace

static bool check_is_0x29(uint32_t frameHeader) {
  return static_cast<uint8_t>(frameHeader >> 8) == 0x29;
}

ServoSendFrame::operator can_frame() const {
  can_frame f{};
  f.can_id = static_cast<canid_t>(mFrameHeader);
  f.can_id = f.can_id | CAN_EFF_FLAG;
  f.len8_dlc = 8;
  std::copy(mData.begin(), mData.end(), f.data);
  return f;
}

ServoRecvFrame::operator can_frame() const {
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

ServoRecvFrame::ServoRecvFrame(canid_t id, ServoFrameID frame_id) : Frame(id), mServoID{frame_id} {
  mFrameHeader = (static_cast<uint32_t>(mServoID) << 8) | (static_cast<uint32_t>(id) & 0xFFu);
  if (!check_is_0x29(mFrameHeader)) {
    throw std::invalid_argument("Invalid frame header for ServoRecvFrame");
  }
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

ServoSendFrame ServoSendFrame::setCurrentLoop(canid_t can_id, int32_t current) {
  ServoSendFrame f(can_id, ServoFrameID::CurrentLoopMode);

  [[unlikely]]
  if (current < -60000 || current > 60000) {
    std::fprintf(stderr,
                 "Passed invalid current value, you passed %d, limits are -60,000 -> 60,000",
                 current);
    current = current < -60000 ? -60000 : 60000;
  }

  f.mData[0] = static_cast<uint8_t>(current >> 24);
  f.mData[1] = static_cast<uint8_t>(current >> 16);
  f.mData[2] = static_cast<uint8_t>(current >> 8);
  f.mData[3] = static_cast<uint8_t>(current);

  return f;
}

ServoSendFrame ServoSendFrame::setCurrentBrake(canid_t can_id, int32_t current) {
  ServoSendFrame f(can_id, ServoFrameID::CurrentBrakeMode);

  if (current < 0 || current > 60000) {
    std::fprintf(stderr, "Passed invalid brake value, you passed %d, limits are %d -> %d\n",
                 current, 0, kCurrentBrakeMax);
    current = current < 0 ? 0 : 60000;
  }

  f.mData[0] = static_cast<uint8_t>(current >> 24);
  f.mData[1] = static_cast<uint8_t>(current >> 16);
  f.mData[2] = static_cast<uint8_t>(current >> 8);
  f.mData[3] = static_cast<uint8_t>(current);

  return f;
}

ServoSendFrame ServoSendFrame::setRPM(canid_t can_id, int32_t rpm) {
  ServoSendFrame f(can_id, ServoFrameID::RPMmode);

  if (rpm < -100000 || rpm > 100000) {
    std::fprintf(stderr, "Passed an invalid RPM value, you passed %d limits were %d -> %d \n", rpm,
                 -kRPMMax, kRPMMax);
    rpm = rpm < -10000 ? -100000 : 100000;
  }

  f.mData[0] = static_cast<uint8_t>(rpm >> 24);
  f.mData[1] = static_cast<uint8_t>(rpm >> 16);
  f.mData[2] = static_cast<uint8_t>(rpm >> 8);
  f.mData[3] = static_cast<uint8_t>(rpm);
  return f;
}

ServoSendFrame ServoSendFrame::setPosition(canid_t can_id, int32_t pos) {
  ServoSendFrame f(can_id, ServoFrameID::PositionMode);

  if (pos < -360000000 || pos > 360000000) {
    pos = pos < -360000000 ? -360000000 : 360000000;
    std::fprintf(stderr,
                 "Passed an invalid position value, you passed %d -360,000,000 -> 360,000,000\n",
                 pos);
  }

  f.mData[0] = static_cast<uint8_t>(pos >> 24);
  f.mData[1] = static_cast<uint8_t>(pos >> 16);
  f.mData[2] = static_cast<uint8_t>(pos >> 8);
  f.mData[3] = static_cast<uint8_t>(pos);

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

ServoSendFrame ServoSendFrame::setPositionAndVelo(canid_t can_id, int32_t position, int16_t speed,
                                                  int16_t accel) {
  ServoSendFrame f(can_id, ServoFrameID::PositionVelocityMode);
  if (position < kPositionMin || position > kPositionMax) {

    std::fprintf(stderr, "Invalid argument passed to position you passed %d, expected %d -> %d\n",
                 position, kPositionMin, kPositionMax);
    position = position < kPositionMin ? kPositionMin : kPositionMax;
  }
  // Speed just uses the int limit, no check needed

  // Accel uses int limit as upper bound
  if (accel < kAccelMin) {

    std::fprintf(stderr,
                 "Invalid argument passed to acceleration, value was %d, limits were %d->%d\n",
                 accel, kAccelMin, kAccelMax);
    accel = kAccelMin;
  }

  f.mData[0] = static_cast<uint8_t>(position >> 24);
  f.mData[1] = static_cast<uint8_t>(position >> 16);
  f.mData[2] = static_cast<uint8_t>(position >> 8);
  f.mData[3] = static_cast<uint8_t>(position);

  f.mData[4] = static_cast<uint8_t>(speed >> 8);
  f.mData[5] = static_cast<uint8_t>(speed);

  f.mData[6] = static_cast<uint8_t>(accel >> 8);
  f.mData[7] = static_cast<uint8_t>(accel);

  return f;
}

uint8_t ServoSendFrame::getId() { return static_cast<uint8_t>(mFrameHeader & 0xFFu); }

uint8_t ServoRecvFrame::getId() { return static_cast<uint8_t>(mFrameHeader & 0xFFu); }

float ServoRecvFrame::getPosition() {
  int16_t pos{};
  std::copy(mData.begin(), mData.begin() + 2, &pos);
  /*
   * Check ak-series PDF, protocol calls for this
   * I want values to reflect what they look like on the motors
   * not the explicit return values on the protocol,
   * it introduces some confusion and makes life harder for anyone
   * that doesnt have an intimate knowledge of the protocol
   */
  float posReal{static_cast<float>(pos)};
  posReal /= 100;
  return posReal;
}

int32_t ServoRecvFrame::getSpeed() {
  int16_t speed{};
  std::copy(mData.begin() + 2, mData.begin() + 4, &speed);
  int32_t speedReal{speed};
  speedReal *= 10;
  return speedReal;
}

float ServoRecvFrame::getCurrent() {
  int16_t current{};
  std::copy(mData.begin() + 4, mData.begin() + 6, &current);
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
