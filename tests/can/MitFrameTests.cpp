#include "../TestInclude.hpp"
#include "Errors.hpp"
#include "can/MIT_frame.hpp"
#include "motors/Motors.hpp"
#include <gtest/gtest.h>

namespace {
float uintToFloat(int32_t num, float nLims, float pLims, int bits) {
  //
  float span = pLims - nLims;
  float offset = nLims;
  return static_cast<float>(num) * span / (static_cast<float>(1 << bits) - 1) + offset;
}

uint32_t floatToUint(float num, float nLims, float pLims, int bits) {
  float span = pLims - nLims;
  if (num < nLims)
    num = nLims;
  else if (num > pLims)
    num = pLims;
  return static_cast<int32_t>((num - nLims) * (float)(1 << bits) / span);
}
}; // namespace

void MitFrameTests::SetUp() {
  uint8_t ind = static_cast<uint8_t>(AKSeriesMotor::AK10_9);
  runLimits = motorRunLimits[ind];

  const float position = runLimits.pos - 5.0f;
  const float speed = 45.0f;
  const float current = runLimits.torque - 10.0f;
  const float kp = 2.0f;
  const float kd = 2.0f;

  MitRunSettings *runSettings = new MitRunSettings{position, speed, current, kp, kd};

  mitRunSettings = std::make_unique<MitRunSettings>(position, speed, current, kp, kd);

  sendFrame = std::make_unique<MitSendFrame>(11, AKSeriesMotor::AK10_9, runSettings);

  id = 11;
  int32_t fPos{1000};
  fPos = fPos >> 4;
  fakePos = uintToFloat(fPos, -runLimits.pos, runLimits.pos, 28);
  uint16_t fCurr = {2000};
  fakeCurrent = uintToFloat(fCurr, -runLimits.torque, runLimits.torque, 12);
  fakeTemp = -10;
  fakeError = ErrorCode::OverTemperature;
  // Set this up to compare outputs later
  const int32_t posWire = fPos << 4;
  can_frame f{};
  f.data[0] = id;
  f.data[1] = static_cast<uint8_t>(posWire >> 24);
  f.data[2] = static_cast<uint8_t>(posWire >> 16);
  f.data[3] = static_cast<uint8_t>(posWire >> 8);
  f.data[4] = static_cast<uint8_t>((((posWire >> 4) & LOW4BITS) << 4) | ((fCurr >> 8) & LOW4BITS));
  f.data[5] = static_cast<uint8_t>(fCurr);
  f.data[6] = static_cast<uint8_t>(fakeTemp);
  f.data[7] = static_cast<uint8_t>(fakeError);
  recvFrame = std::make_unique<MitRecvFrame>(f, AKSeriesMotor::AK10_9);
}

TEST_F(MitFrameTests, SendFrameTests) {
  can_frame f = static_cast<can_frame>(*sendFrame);
  uint16_t pos = floatToUint(mitRunSettings->position, -runLimits.pos, runLimits.pos, 16);
  uint16_t posFF{0};
  posFF += static_cast<uint16_t>(f.data[0]) << 8;
  posFF += static_cast<uint16_t>(f.data[1]);
  ASSERT_EQ(pos, posFF) << "Position converted unsuccesfully";

  uint16_t speed = floatToUint(mitRunSettings->speed, -runLimits.speed, runLimits.speed, 12);
  uint16_t speedFF{0};
  speedFF += static_cast<uint16_t>(f.data[2]) << 4;
  speedFF += (static_cast<uint16_t>(f.data[3]) & HIGH4BITS) >> 4;
  ASSERT_EQ(speed, speedFF) << "speed converted unsuccesfully";

  uint16_t KP = floatToUint(mitRunSettings->KP, 0, runLimits.KPMax, 12);
  uint16_t KPFF{0};
  KPFF += static_cast<uint16_t>(f.data[3] & LOW4BITS) << 8;
  KPFF += static_cast<uint16_t>(f.data[4]);
  ASSERT_EQ(KP, KPFF) << "KP converted unsuccesfully";

  uint16_t KD = floatToUint(mitRunSettings->KD, 0, runLimits.KDMax, 12);
  uint16_t KDFF{0};
  KDFF += static_cast<uint16_t>(f.data[5]) << 4;
  KDFF += (static_cast<uint16_t>(f.data[6]) & HIGH4BITS) >> 4;
  ASSERT_EQ(KD, KDFF) << "KD converted unsuccesfully";

  uint16_t curr = floatToUint(mitRunSettings->current, -runLimits.torque, runLimits.torque, 12);
  uint16_t currFF{0};
  currFF += static_cast<uint16_t>(f.data[6] & LOW4BITS) << 8;
  currFF += static_cast<uint16_t>(f.data[7]);
  ASSERT_EQ(curr, currFF) << "Current converted unsuccesfully";
}

TEST_F(MitFrameTests, RecvFrameTests) {

  auto lsb = [](float min, float max, int bits) -> float {
    return (max - min) / static_cast<float>((1 << bits) - 1);
  };
  ASSERT_EQ(recvFrame->getId(), id) << "ID was incorrect";
  float lsbVal = lsb(-runLimits.pos, runLimits.pos, 28);
  ASSERT_NEAR(recvFrame->getPosition(), fakePos, lsbVal) << "Position was incorrect";
  lsbVal = lsb(-runLimits.torque, runLimits.torque, 12);
  ASSERT_EQ(recvFrame->getCurrent(), fakeCurrent) << "Current was incorrect";
  ASSERT_EQ(recvFrame->getTemperature(), fakeTemp) << "Temperature value was incorrect";
  ASSERT_EQ(recvFrame->getErrorCode(), fakeError) << "Error code was incorrect";
}
