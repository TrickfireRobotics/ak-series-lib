#include "../TestInclude.hpp"

namespace {

uint8_t servoFrameId(const can_frame &frame) {
  return static_cast<uint8_t>((frame.can_id & CAN_EFF_MASK) >> 8);
}

uint8_t servoCanId(const can_frame &frame) { return static_cast<uint8_t>(frame.can_id & 0xFFu); }

int32_t readInt32BE(const uint8_t *data) {
  return (static_cast<int32_t>(data[0]) << 24) | (static_cast<int32_t>(data[1]) << 16) |
         (static_cast<int32_t>(data[2]) << 8) | static_cast<int32_t>(data[3]);
}

int16_t readInt16BE(const uint8_t *data) {
  return static_cast<int16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

void assertTrailingZeroes(const can_frame &frame, std::size_t start) {
  for (std::size_t i = start; i < 8; ++i) {
    ASSERT_EQ(frame.data[i], 0) << "Expected zero padding at byte " << i;
  }
}

can_frame makeServoFeedbackFrame(canid_t canId, const std::array<uint8_t, 8> &data) {
  can_frame frame{};
  frame.can_id = (static_cast<uint32_t>(0x29) << 8) | (static_cast<uint32_t>(canId) & 0xFFu);
  frame.can_id |= CAN_EFF_FLAG;
  frame.len8_dlc = 8;
  std::copy(data.begin(), data.end(), frame.data);
  return frame;
}

} // namespace

ServoFrameTests::ServoFrameTests() = default;
ServoFrameTests::~ServoFrameTests() = default;
void ServoFrameTests::SetUp() {}
void ServoFrameTests::TearDown() {}

TEST_F(ServoFrameTests, ServoSendDutyCycle) {
  canid_t canId = 11;
  float dutyCycleVal = 20.0f;

  ServoSendFrame dutyCycleFrame = ServoSendFrame::setDutyCycle(canId, dutyCycleVal);
  can_frame frame = static_cast<can_frame>(dutyCycleFrame);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::DutyCycleMode))
      << "Header for duty cycle was invalid";
  ASSERT_EQ(servoCanId(frame), canId) << "Can ID for duty cycle was invalid";
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(dutyCycleVal * 100000.0f))
      << "Duty cycle value was invalid";
  assertTrailingZeroes(frame, 4);
}

TEST_F(ServoFrameTests, ServoSendCurrentLoop) {
  canid_t canId = 11;
  float currentValue = 20.0f;
  ServoSendFrame currentFrame = ServoSendFrame::setCurrentLoop(canId, currentValue);
  can_frame frame = static_cast<can_frame>(currentFrame);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::CurrentLoopMode))
      << "Header for current loop was invalid";
  ASSERT_EQ(servoCanId(frame), canId) << "Can ID for current loop was invalid";
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(currentValue * 1000.0f))
      << "Current loop value was invalid";
  assertTrailingZeroes(frame, 4);
}

TEST_F(ServoFrameTests, ServoSendCurrentBrake) {
  canid_t canId = 11;
  float brakeValue = 15.0f;
  int32_t brakeValueInt = static_cast<int32_t>(brakeValue * 1000.0f);
  ServoSendFrame brakeFrame = ServoSendFrame::setCurrentBrake(canId, brakeValue);
  can_frame frame = static_cast<can_frame>(brakeFrame);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::CurrentBrakeMode))
      << "Header for current brake was invalid";
  ASSERT_EQ(servoCanId(frame), canId) << "Can ID for current brake was invalid";
  ASSERT_EQ(readInt32BE(frame.data), brakeValueInt) << "Current brake value was invalid";
  assertTrailingZeroes(frame, 4);
}

TEST_F(ServoFrameTests, ServoSendRPM) {
  canid_t canId = 11;
  float rpmValue = 5000.0f;
  int32_t rpmValueInt = static_cast<int32_t>(rpmValue);
  ServoSendFrame rpmFrame = ServoSendFrame::setRPM(canId, rpmValue);
  can_frame frame = static_cast<can_frame>(rpmFrame);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::RPMmode))
      << "Header for RPM mode was invalid";
  ASSERT_EQ(servoCanId(frame), canId) << "Can ID for RPM mode was invalid";
  ASSERT_EQ(readInt32BE(frame.data), rpmValueInt) << "RPM value was invalid";
  assertTrailingZeroes(frame, 4);
}

TEST_F(ServoFrameTests, ServoSendPosition) {
  canid_t canId = 11;
  float positionValue = -12345.678;
  int32_t positionValueInt = static_cast<int32_t>(positionValue * 10000.0f);
  ServoSendFrame positionFrame = ServoSendFrame::setPosition(canId, positionValue);
  can_frame frame = static_cast<can_frame>(positionFrame);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::PositionMode))
      << "Header for position mode was invalid";
  ASSERT_EQ(servoCanId(frame), canId) << "Can ID for position mode was invalid";
  ASSERT_EQ(readInt32BE(frame.data), positionValueInt) << "Position value was invalid";
  assertTrailingZeroes(frame, 4);
}

TEST_F(ServoFrameTests, ServoSendOrigin) {
  canid_t canId = 11;

  ServoSendFrame originFrame = ServoSendFrame::setOrigin(canId, 1);
  can_frame frame = static_cast<can_frame>(originFrame);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::SetOriginMode))
      << "Header for set origin was invalid";
  ASSERT_EQ(servoCanId(frame), canId) << "Can ID for set origin was invalid";
  ASSERT_EQ(frame.data[0], 1) << "Origin mode value was invalid";
  assertTrailingZeroes(frame, 1);

  EXPECT_THROW(auto discard = ServoSendFrame::setOrigin(canId, 2), std::invalid_argument);
}

TEST_F(ServoFrameTests, ServoSendPositionAndVelo) {
  canid_t canId = 11;
  int32_t positionValue = 360000;
  int16_t speedValue = -2500;
  int16_t accelValue = 800;
  float positionValueFloat = static_cast<float>(positionValue) / 10000.0f;
  float speedValueFloat = static_cast<float>(speedValue) * 10.0f;
  float accelValueFloat = static_cast<float>(accelValue) * 10.0f;

  ServoSendFrame pvFrame = ServoSendFrame::setPositionAndVelo(canId, positionValueFloat,
                                                              speedValueFloat, accelValueFloat);
  can_frame frame = static_cast<can_frame>(pvFrame);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::PositionVelocityMode))
      << "Header for position/velocity mode was invalid";
  ASSERT_EQ(servoCanId(frame), canId) << "Can ID for position/velocity mode was invalid";
  ASSERT_EQ(readInt32BE(frame.data), positionValue) << "Position value was invalid";
  ASSERT_EQ(readInt16BE(&frame.data[4]), speedValue) << "Speed value was invalid";
  ASSERT_EQ(readInt16BE(&frame.data[6]), accelValue) << "Acceleration value was invalid";
}

TEST_F(ServoFrameTests, ServoRecvFrameTests) {
  canid_t canId = 11;
  const float expectedPosition = 12.34f;
  const int32_t expectedSpeed = 500;
  const float expectedCurrent = 5.67f;
  const int8_t expectedTemp = -10;
  const ErrorCode expectedError = ErrorCode::OverTemperature;

  std::array<uint8_t, 8> data{};
  const int16_t positionRaw = static_cast<int16_t>(expectedPosition * 10.0f);
  const int16_t speedRaw = static_cast<int16_t>(expectedSpeed / 10);
  const int16_t currentRaw = static_cast<int16_t>(expectedCurrent * 100.0f);

  data[0] = static_cast<uint8_t>(positionRaw >> 8);
  data[1] = static_cast<uint8_t>(positionRaw);
  data[2] = static_cast<uint8_t>(speedRaw >> 8);
  data[3] = static_cast<uint8_t>(speedRaw);
  data[4] = static_cast<uint8_t>(currentRaw >> 8);
  data[5] = static_cast<uint8_t>(currentRaw);
  data[6] = static_cast<uint8_t>(expectedTemp);
  data[7] = static_cast<uint8_t>(expectedError);

  can_frame frame = makeServoFeedbackFrame(canId, data);
  ServoRecvFrame recvFrame(frame);

  ASSERT_EQ(recvFrame.getId(), canId) << "ID was incorrect";
  ASSERT_NEAR(recvFrame.getPosition(), expectedPosition, 0.1f) << "Position was incorrect";
  ASSERT_EQ(recvFrame.getSpeed(), expectedSpeed) << "Speed was incorrect";
  ASSERT_FLOAT_EQ(recvFrame.getCurrent(), expectedCurrent) << "Current was incorrect";
  ASSERT_EQ(recvFrame.getTemperature(), expectedTemp) << "Temperature value was incorrect";
  ASSERT_EQ(recvFrame.getErrorCode(), expectedError) << "Error code was incorrect";
}
