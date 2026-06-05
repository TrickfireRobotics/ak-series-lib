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

static std::string invalidCallStr(uint8_t id) {
  char buffer[10];
  auto result = std::to_chars(buffer, buffer + sizeof(buffer), id);
  if (result.ec != std::errc{}) {
    throw std::runtime_error("Failed to format servo id");
  }
  std::string desc{"Invalid call called get_current on a frame with id 0x29 this has id of "};
  desc.append(buffer, result.ptr);
  return desc;
}

static bool check_is_0x29(uint32_t frame_header) {
  // Command ID occupies bits [15:8] per protocol (stored as << 8)
  return (static_cast<uint8_t>((frame_header >> 8) & 0xFF) == kServoModeFeedbackId);
}

ServoFrame::operator can_frame() const {
  can_frame f{};
  f.can_id = static_cast<canid_t>(m_frame_header);
  f.can_id = f.can_id | CAN_EFF_FLAG;
  f.len8_dlc = 8;
  std::copy(m_data.begin(), m_data.end(), f.data);
  return f;
}

ServoRecvFrame::operator can_frame() const {
  can_frame f{};
  f.can_id = static_cast<canid_t>(m_frame_header);
  f.can_id = f.can_id | CAN_EFF_FLAG;
  f.len8_dlc = 8;
  std::copy(m_data.begin(), m_data.end(), f.data);
  return f;
}

ServoFrame::ServoFrame(canid_t id, ServoFrameID frame_id) : Frame(id), m_servo_id{frame_id} {
  // Pack command (frame id) into bits [15:8] and motor id into [7:0]
  m_frame_header = (static_cast<uint32_t>(m_servo_id) << 8) | (static_cast<uint32_t>(id) & 0xFFu);
}

ServoRecvFrame::ServoRecvFrame(canid_t id, ServoFrameID frame_id)
    : Frame(id), m_servo_id{frame_id} {
  m_frame_header = (static_cast<uint32_t>(m_servo_id) << 8) | (static_cast<uint32_t>(id) & 0xFFu);
  if (!check_is_0x29(m_frame_header)) {
    throw std::invalid_argument("Invalid frame header for ServoRecvFrame");
  }
}

ServoFrame ServoFrame::setDutyCycle(canid_t can_id, float dutyCycle) {
  // Look in the PDF to see why this is done
  int32_t duty = static_cast<int32_t>(dutyCycle * kDutyScale);
  ServoFrame f(can_id, ServoFrameID::DutyCycleMode);

  f.m_data[0] = static_cast<uint8_t>(duty >> 24);
  f.m_data[1] = static_cast<uint8_t>(duty >> 16);
  f.m_data[2] = static_cast<uint8_t>(duty >> 8);
  f.m_data[3] = static_cast<uint8_t>(duty);

  return f;
}

ServoFrame ServoFrame::setCurrentLoop(canid_t can_id, int32_t current) {
  ServoFrame f(can_id, ServoFrameID::CurrentLoopMode);

  [[unlikely]]
  if (current < kCurrentLoopMin || current > kCurrentLoopMax) {
    current = current < kCurrentLoopMin ? kCurrentLoopMin : kCurrentLoopMax;
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are %d, %d", current,
               kCurrentLoopMin, kCurrentLoopMax);

    throw std::invalid_argument(const_cast<const char *>(buf));
  }

  f.m_data[0] = static_cast<uint8_t>(current >> 24);
  f.m_data[1] = static_cast<uint8_t>(current >> 16);
  f.m_data[2] = static_cast<uint8_t>(current >> 8);
  f.m_data[3] = static_cast<uint8_t>(current);

  return f;
}

ServoFrame ServoFrame::setCurrentBrake(canid_t can_id, int32_t current) {
  ServoFrame f(can_id, ServoFrameID::CurrentBrakeMode);

  if (current < 0 || current > kCurrentBrakeMax) {

    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are 0, %d", current,
               kCurrentBrakeMax);

    throw std::invalid_argument(buf);
  }

  f.m_data[0] = static_cast<uint8_t>(current >> 24);
  f.m_data[1] = static_cast<uint8_t>(current >> 16);
  f.m_data[2] = static_cast<uint8_t>(current >> 8);
  f.m_data[3] = static_cast<uint8_t>(current);

  return f;
}

ServoFrame ServoFrame::setRPM(canid_t can_id, int32_t rpm) {
  ServoFrame f(can_id, ServoFrameID::RPMmode);

  if (rpm < -kRPMMax || rpm > kRPMMax) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are -%d, %d", rpm, kRPMMax,
               kRPMMax);

    throw std::invalid_argument(buf);
  }

  f.m_data[0] = static_cast<uint8_t>(rpm >> 24);
  f.m_data[1] = static_cast<uint8_t>(rpm >> 16);
  f.m_data[2] = static_cast<uint8_t>(rpm >> 8);
  f.m_data[3] = static_cast<uint8_t>(rpm);
  return f;
}

ServoFrame ServoFrame::setPosition(canid_t can_id, int32_t pos) {
  ServoFrame f(can_id, ServoFrameID::PositionMode);

  if (pos < kPositionMin || pos > kPositionMax) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are %d, %d", pos,
               kPositionMin, kPositionMax);

    throw std::invalid_argument(buf);
  }

  f.m_data[0] = static_cast<uint8_t>(pos >> 24);
  f.m_data[1] = static_cast<uint8_t>(pos >> 16);
  f.m_data[2] = static_cast<uint8_t>(pos >> 8);
  f.m_data[3] = static_cast<uint8_t>(pos);

  return f;
}

ServoFrame ServoFrame::setOrigin(canid_t can_id, uint8_t origin_mode) {
  ServoFrame f(can_id, ServoFrameID::SetOriginMode);
  if (origin_mode > 1) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are 0 or 1", origin_mode);

    throw std::invalid_argument(buf);
  }
  f.m_data[0] = origin_mode;
  return f;
}

ServoFrame ServoFrame::setPositionAndVelo(canid_t can_id, int32_t position, int16_t speed,
                                          int16_t accel) {
  ServoFrame f(can_id, ServoFrameID::PositionVelocityMode);
  if (position < kPositionMin || position > kPositionMax) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are %d, %d", position,
               kPositionMin, kPositionMax);

    throw std::invalid_argument(buf);
  }
  // Speed just uses the int limit, no check needed

  // Accel uses int limit as upper bound
  if (accel < kAccelMin) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are %d, %d", accel,
               kAccelMin, kAccelMax);

    throw std::invalid_argument(buf);
  }

  f.m_data[0] = static_cast<uint8_t>(position >> 24);
  f.m_data[1] = static_cast<uint8_t>(position >> 16);
  f.m_data[2] = static_cast<uint8_t>(position >> 8);
  f.m_data[3] = static_cast<uint8_t>(position);

  f.m_data[4] = static_cast<uint8_t>(speed >> 8);
  f.m_data[5] = static_cast<uint8_t>(speed);

  f.m_data[6] = static_cast<uint8_t>(accel >> 8);
  f.m_data[7] = static_cast<uint8_t>(accel);

  return f;
}

uint8_t ServoFrame::getId() { return static_cast<uint8_t>(m_frame_header & 0xFFu); }

uint8_t ServoRecvFrame::getId() { return static_cast<uint8_t>(m_frame_header & 0xFFu); }

bool ServoFrame::is0x29() { return check_is_0x29(m_frame_header); }

float ServoRecvFrame::getPosition() {
  int16_t pos{};
  std::copy(m_data.begin(), m_data.begin() + 2, &pos);
  /*
   * Check ak-series PDF, protocol calls for this
   * I want values to reflect what they look like on the motors
   * not the explicit return values on the protocol,
   * it introduces some confusion and makes life harder for anyone
   * that doesnt have an intimate knowledge of the protocol
   */
  float posreal{static_cast<float>(pos)};
  posreal /= 100;
  return posreal;
}

int32_t ServoRecvFrame::getSpeed() {
  int16_t speed{};
  std::copy(m_data.begin() + 2, m_data.begin() + 4, &speed);
  int32_t speedreal{speed};
  speedreal *= 10;
  return speedreal;
}

float ServoRecvFrame::getCurrent() {
  int16_t current{};
  std::copy(m_data.begin() + 4, m_data.begin() + 6, &current);
  float currentreal{static_cast<float>(current)};
  currentreal /= 100;
  return currentreal;
}

int8_t ServoRecvFrame::getTemperature() {
  int8_t temp{static_cast<int8_t>(m_data[6])};
  return temp;
}

ErrorCode ServoRecvFrame::getErrorCode() {
  ErrorCode r = static_cast<ErrorCode>(m_data[7]);
  return r;
}
