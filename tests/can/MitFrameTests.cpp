#include "../TestInclude.hpp"
#include "Exceptions.hpp"
#include "can/MIT_frame.hpp"
#include "motors/MotorLimits.hpp"
#include <gtest/gtest.h>
#include <memory>

static float uintToFloat(int32_t num, float nLims, float pLims, int bits) {
  //
  float span = pLims - nLims;
  float offset = nLims;
  return static_cast<float>(num) * span / (static_cast<float>(1 << bits) - 1) + offset;
}

static uint32_t floatToUint(float num, float nLims, float pLims, int bits) {
  float span = pLims - nLims;
  if (num < nLims)
    num = nLims;
  else if (num > pLims)
    num = pLims;
  return static_cast<int32_t>((num - nLims) * (float)(1 << bits) / span);
}

void MitFrameTests::SetUp() {
  uint8_t ind = static_cast<uint8_t>(AkSeriesMotors::AK10_9);
  runLimits = motorRunLimits[ind];

  MitRunSettings *runSettings =
      new MitRunSettings{12.5f, runLimits.pos - 5.0f, runLimits.torque - 10.0f, 100.0f, 2.0f};

  mitRunSettings = std::make_unique<MitRunSettings>(12.5f, runLimits.pos - 5.0f,
                                                    runLimits.torque - 10.0f, 100.0f, 2.0f);

  sendFrame = std::make_unique<MitSendFrame>(11, AkSeriesMotors::AK10_9, runSettings);

  id = 11;
  uint32_t fPos{1000};
  fPos = fPos >> 4;
  fakePos = uintToFloat(fPos, -runLimits.pos, runLimits.pos, 28);
  uint16_t fCurr = {2000};
  fakeCurrent = uintToFloat(fCurr, -runLimits.torque, runLimits.torque, 12);
  fakeTemp = -10;
  fakeError = ErrorCode::OverTemperature;

  // Set this up to compare outputs later
  can_frame f{};
  f.data[0] = id;
  std::copy(&f.data[1], &f.data[4], &fPos);
  f.data[4] = (fCurr >> 8) | f.data[4];
  f.data[5] = static_cast<uint8_t>(fCurr);
  f.data[6] = static_cast<uint8_t>(fakeTemp);
  f.data[7] = static_cast<uint8_t>(fakeError);
  recvFrame = std::make_unique<MitRecvFrame>(f, AkSeriesMotors::AK10_9);
}

TEST_F(MitFrameTests, SendFrameTests) {
  can_frame f = static_cast<can_frame>(*sendFrame);

  uint16_t pos{0};
  std::copy(&f.data[0], &f.data[1], &pos);
  float fPos = uintToFloat(pos, -runLimits.pos, runLimits.pos, 16);
  ASSERT_EQ(fPos, mitRunSettings->position) << "Position failed to convert properly";

  uint16_t speed{f.data[2]};
  speed = speed << 4;
  speed += (f.data[3] & FIRST4BITS);
  float fSpeed = uintToFloat(speed, -runLimits.speed, runLimits.speed, 12);
  ASSERT_EQ(fSpeed, mitRunSettings->speed) << "Speed failed to convert properly";

  uint16_t KP{0};
  KP += f.data[3] & LAST4BITS;
  KP = KP >> 8;
  KP += f.data[4];
  float fKP = uintToFloat(KP, 0, runLimits.KPMax, 12);
  ASSERT_EQ(fKP, mitRunSettings->KP) << "KP failed to convert properly";

  uint16_t KD{f.data[5]};
  KD = KD >> 4;
  KD += (f.data[6] & LAST4BITS);
  float fKD = uintToFloat(KD, 0, runLimits.KDMax, 12);
  ASSERT_EQ(fKD, mitRunSettings->KD) << "KD failed to convert properly";

  uint16_t current{0};
  current += (f.data[6] & FIRST4BITS);
  current = current >> 8;
  current += f.data[7];
  float fCurr = uintToFloat(current, -runLimits.torque, runLimits.torque, 12);
  ASSERT_EQ(fCurr, mitRunSettings->current) << "Current conversion failed";
}

TEST_F(MitFrameTests, RecvFrameTests) {
  ASSERT_EQ(recvFrame->getId(), id) << "ID was incorrect";
  ASSERT_EQ(recvFrame->getPosition(), fakePos) << "Position was incorrect";
  ASSERT_EQ(recvFrame->getCurrent(), fakeCurrent) << "Current was incorrect";
  ASSERT_EQ(recvFrame->getTemperature(), fakeTemp) << "Temperature value was incorrect";
  ASSERT_EQ(recvFrame->getErrorCode(), fakeError) << "Error code was incorrect";
}
