#include "can/Servo_frame.hpp"
#include <charconv>
#include <functional>
#include <linux/can.h>
#include <stdexcept>
#include <string.h>

static std::string invalidCallStr(uint8_t id) {
  char buffer[10];
  std::to_chars(&buffer[0], &buffer[9], id);
  std::string desc{"Invalid call called get_current on a frame with id 0x29 this has id of "};
  desc += buffer;
  return desc;
}

static bool check_is_0x29(uint32_t frame_header) {
  return (static_cast<uint8_t>(frame_header >> 16) == 0x29);
}

ServoFrame::operator can_frame() {
  can_frame f{};
  f.can_id = static_cast<canid_t>(m_frame_header);
  f.can_id = f.can_id | CAN_EFF_FLAG;
  f.len8_dlc = 8;
  std::copy(&f.data[0], &f.data[8], m_data.begin());
  return f;
}

ServoMsgFrame::operator can_frame() {
  can_frame f{};
  f.can_id = static_cast<canid_t>(m_frame_header);
  f.can_id = f.can_id | CAN_EFF_FLAG;
  f.len8_dlc = 8;
  std::copy(&f.data[0], &f.data[8], m_data.begin());
  return f;
}

ServoFrame::ServoFrame(canid_t id, ServoFrameID frame_id) : Frame(id), m_servo_id{frame_id} {
  m_frame_header = static_cast<uint8_t>(m_servo_id) << 8;
  m_frame_header += static_cast<int32_t>(id);
}

ServoMsgFrame::ServoMsgFrame(canid_t id, ServoFrameID frame_id) : Frame(id), m_servo_id{frame_id} {
  m_frame_header = static_cast<uint8_t>(m_servo_id) << 8;
  m_frame_header += static_cast<int32_t>(id);
  if (!check_is_0x29(m_frame_header)) {
    throw std::invalid_argument("Invalid frame header for ServoMsgFrame");
  }
}

ServoFrame ServoFrame::setDutyCycle(canid_t can_id, float dutyCycle) {
  // Look in the PDF to see why this is done
  int32_t duty = static_cast<int32_t>(dutyCycle * 100000);
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
  if (current < -60000 || current > 60000) {
    current = current < -60000 ? -60000 : 60000;
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are -60000, 60000",
               current);

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

  if (current < 0 || current > 60000) {

    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are 0, 60000", current);

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

  if (rpm < -100000 || rpm > 100000) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are -10000, 10000", rpm);

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

  if (pos < -360000000 || pos > 360000000) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are -360000000, 360000000",
               pos);

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
  if (position < -360000000 || position > 360000000) {
    // TODO implement throw
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are -360000000, 360000000",
               position);

    throw std::invalid_argument(buf);
  }
  // Speed just uses the int limit, no check needed

  // Accel uses int limit as upper bound
  if (accel < 0) {
    char buf[500]{};
    ::snprintf(buf, sizeof(buf) - 1,
               "Invalid argument, you sent a value of %d but the limits are 0, 32767", position);

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

uint8_t ServoFrame::get_id() { return static_cast<uint8_t>(m_frame_header >> 16); }

uint8_t ServoMsgFrame::get_id() { return static_cast<uint8_t>(m_frame_header >> 16); }

bool ServoFrame::is0x29() { return check_is_0x29(m_frame_header); }

float ServoMsgFrame::getPosition() {
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

int32_t ServoMsgFrame::getSpeed() {
  int16_t speed{};
  std::copy(m_data.begin() + 2, m_data.begin() + 4, &speed);
  int32_t speedreal{speed};
  speedreal *= 10;
  return speedreal;
}

float ServoMsgFrame::getCurrent() {
  int16_t current{};
  std::copy(m_data.begin() + 4, m_data.begin() + 6, &current);
  float currentreal{static_cast<float>(current)};
  currentreal /= 100;
  return currentreal;
}

int8_t ServoMsgFrame::getTemperature() {
  int8_t temp{static_cast<int8_t>(m_data[6])};
  return temp;
}

ErrorCode ServoMsgFrame::getErrorCode() {
  ErrorCode r = static_cast<ErrorCode>(m_data[7]);
  return r;
}
