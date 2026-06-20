#include "can/MIT_frame.hpp"
#include "motors/MotorLimits.hpp"

enum class EnterMitMode { EnterMotorControlMode = 0, ExitMotorControlMode, SetCurrentMotorPositon };

namespace {
const uint8_t mitMotorFrames[][8]{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc},
                                  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd},
                                  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe}};

// Following the guide from the PDF
int float_to_uint(float x, float x_min, float x_max, unsigned int bits) {
  float span = x_max - x_min;
  if (x < x_min)
    x = x_min;
  else if (x > x_max)
    x = x_max;

  return static_cast<int>((x - x_min) * (static_cast<float>(1 << bits) / span));
}

float uint_to_float(int x_int, float x_min, float x_max, int bits) {
  float span = x_max - x_min;
  float offset = x_min;
  return (static_cast<float>(x_int) * span / (static_cast<float>(1 << bits) - 1)) + offset;
}
}; // namespace

MitSendFrame::operator can_frame() {
  padData();
  can_frame frame{};
  frame.can_id = mCanId;
  frame.len = 8;
  std::copy(mData.begin(), mData.end(), &frame.data[0]);
  return frame;
}

void MitSendFrame::padData() {
  MotorRunLimits mLims = motorRunLimits[static_cast<uint8_t>(mMotor)];

  uint16_t pos = float_to_uint(mSettings->position, -mLims.pos, mLims.pos, 16);
  uint16_t speed = float_to_uint(mSettings->speed, -mLims.speed, mLims.speed, 12);
  uint16_t current = float_to_uint(mSettings->current, -mLims.torque, mLims.torque, 12);
  uint16_t KP = float_to_uint(mSettings->KP, 0, mLims.KPMax, 12);
  uint16_t KD = float_to_uint(mSettings->KD, 0, mLims.KDMax, 12);
  mData[0] = static_cast<uint8_t>(pos >> 8);
  // Position low 8 bytes
  mData[1] = static_cast<uint8_t>(pos);
  // Motor speed high 8 bits
  mData[2] = static_cast<uint8_t>(speed >> 4);
  mData[3] = static_cast<uint8_t>(((speed & LOW4BITS) << 4) | ((KP >> 8) & LOW4BITS));
  mData[4] = static_cast<uint8_t>(KP);
  mData[5] = static_cast<uint8_t>(KD >> 4);
  mData[6] = static_cast<uint8_t>(((KD & LOW4BITS) << 4) | ((current >> 8) & LOW4BITS));
  mData[7] = static_cast<uint8_t>(current);
}

MitRecvFrame::MitRecvFrame(const can_frame &frame, AkSeriesMotors motor)
    : Frame(frame.can_id), mMotor{motor}, mLims{motorRunLimits[static_cast<uint8_t>(mMotor)]} {
  std::copy(&frame.data[0], &frame.data[8], mData.begin());
}

uint8_t MitRecvFrame::getId() { return mData[0]; }

float MitRecvFrame::getPosition() {
  int32_t pos = (static_cast<int32_t>(mData[1]) << 24) | (static_cast<int32_t>(mData[2]) << 16) |
                (static_cast<int32_t>(mData[3]) << 8) | static_cast<int32_t>(mData[4]);
  pos >>= 4;
  return uint_to_float(pos, -mLims.pos, mLims.pos, 28);
}

float MitRecvFrame::getCurrent() {
  uint16_t curr = static_cast<uint16_t>((mData[4] & LOW4BITS) << 8) | mData[5];
  return uint_to_float(curr, -mLims.torque, mLims.torque, 12);
}
int8_t MitRecvFrame::getTemperature() { return static_cast<int8_t>(mData[6]); }
ErrorCode MitRecvFrame::getErrorCode() { return static_cast<ErrorCode>(mData[7]); }

MitRecvFrame::operator can_frame() {
  can_frame frame{};
  frame.can_id = mCanId;
  std::copy(mData.begin(), mData.end(), &frame.data[0]);
  frame.len = 8;
  return frame;
}
