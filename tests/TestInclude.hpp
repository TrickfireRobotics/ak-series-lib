#ifndef __TEST_AK_SERIES
#define __TEST_AK_SERIES
#include "motors/MotorLimits.hpp"
#pragma once
#include "Exceptions.hpp"
#include "can/MIT_frame.hpp"
#include "can/Servo_frame.hpp"
#include <gtest/gtest.h>
#include <memory>

class ServoFrameTests : public testing::Test {
protected:
public:
  ServoFrameTests();
  ~ServoFrameTests() override;
  void SetUp() override;
  void TearDown() override;
  // Tests below this
  void testSendFrames();
};

class MitFrameTests : public testing::Test {
protected:
  std::unique_ptr<MitSendFrame> sendFrame;
  std::unique_ptr<MitRecvFrame> recvFrame;
  std::unique_ptr<MitRunSettings> mitRunSettings;
  MotorRunLimits runLimits;
  uint8_t id{};
  float fakePos{};
  float fakeCurrent{};
  int8_t fakeTemp{};
  ErrorCode fakeError{};

public:
  MitFrameTests();
  ~MitFrameTests() override;
  void SetUp() override;
  void TearDown() override;
};

#endif
