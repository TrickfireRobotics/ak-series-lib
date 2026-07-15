
#ifndef __TEST_AK_SERIES
#define __TEST_AK_SERIES
#include "motors/MotorLimits.hpp"
#pragma once
#include "Errors.hpp"
#include "can/MIT_frame.hpp"
#include "can/Servo_frame.hpp"
#include "can/comms.hpp"
#include "can/frame.hpp"
#include <gtest/gtest.h>
#include <memory>

// Purely for LSP to allow for me to access
// internal private variables in CanInterface
#ifndef BUILD_TESTING
#define BUILD_TESTING
#endif

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
  MitFrameTests() {}
  ~MitFrameTests() = default;
  void SetUp() override;
  void TearDown() override {};
};

class CanInterfaceTests : public testing::Test {
protected:
  int socketFD;
  std::string canIfName;

public:
  CanInterfaceTests() {}
  ~CanInterfaceTests() = default;
  void SetUp() override;
  void TearDown() override {};
};

#endif
