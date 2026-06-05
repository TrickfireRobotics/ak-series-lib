#include "can/MIT_frame.hpp"
#include "motors/MotorLimits.hpp"

enum class EnterMitMode { EnterMotorControlMode = 0, ExitMotorControlMode, SetCurrentMotorPositon };

static const uint8_t mitMotorFrames[][8]{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc},
                                         {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd},
                                         {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe},
                                         NULL};

// Following the guide from the PDF
static int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
  float span = x_max - x_min;

  if (x < x_min)
    x = x_min;
  else if (x > x_max)
    x = x_max;

  return (int)((x - x_min) * ((float)((1 << bits) / span)));
}

static float uint_to_float(int x_int, float x_min, float x_max, int bits) {
  float span = x_max - x_min;
  float offset = x_min;

  return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
}

MitSendFrame::operator can_frame() {
  padData();
  can_frame frame{};
  frame.can_id = m_can_id;
  frame.len = 8;
  std::copy(m_data.begin(), m_data.end(), &frame.data[0]);
  return frame;
}

void MitSendFrame::padData() {
  MotorRunLimits lims = motorRunLimits[static_cast<uint8_t>(m_motor)];

  uint16_t pos = float_to_uint(m_settings->position, -lims.pos, lims.pos, 16);
  uint16_t speed = float_to_uint(m_settings->speed, -lims.speed, lims.speed, 12);
  uint16_t current = float_to_uint(m_settings->current, 0, 60, 12);
  uint16_t KP = float_to_uint(m_settings->KP, 0, lims.KPMax, 12);
  uint16_t KD = float_to_uint(m_settings->KD, 0, lims.KDMax, 12);
  m_data[0] = static_cast<uint8_t>(pos >> 8);
  // Position low 8 bytes
  m_data[1] = static_cast<uint8_t>(pos);
  // Motor speed high 8 bits
  m_data[2] = static_cast<uint8_t>(speed >> 4);
  m_data[3] = (speed >> 4) | (KP >> 8);
  m_data[4] = static_cast<uint8_t>(KP);
  m_data[5] = static_cast<uint8_t>(KD >> 4);
  m_data[6] = (KD >> 4) | (current >> 8);
  m_data[7] = static_cast<uint8_t>(current >> 4);
}

MitRecvFrame::MitRecvFrame(const can_frame &frame) : Frame(frame.can_id) {
  std::copy(&frame.data[0], &frame.data[7], m_data);
}

// TODO implement
uint8_t MitRecvFrame::getId() {}
int32_t MitRecvFrame::getPosition() {}
float MitRecvFrame::getCurrent() {}
uint8_t MitRecvFrame::getTemperature() {}
ErrorCode MitRecvFrame::getErrorCode() {}
