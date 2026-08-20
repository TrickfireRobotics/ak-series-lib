#include "./TestInclude.hpp"
#include "AKSeries.hpp"
#include "can/MIT_frame.hpp"
#include "can/Servo_frame.hpp"
#include "motors/Motors.hpp"
#include <array>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

using namespace AKSeries;

namespace {

float uintToFloat(int32_t num, float nLims, float pLims, int bits) {
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

can_frame makeServoFeedbackFrame(canid_t canId, const std::array<uint8_t, 8> &data) {
  can_frame frame{};
  frame.can_id = (static_cast<uint32_t>(0x29) << 8) | (static_cast<uint32_t>(canId) & 0xFFu);
  frame.can_id |= CAN_EFF_FLAG;
  frame.len8_dlc = 8;
  std::copy(data.begin(), data.end(), frame.data);
  return frame;
}

can_frame readFrame(int fd) {
  can_frame frame{};
  ::read(fd, &frame, sizeof(can_frame));
  return frame;
}

MotorRunLimits customServoLimits() {
  return MotorRunLimits{.pos = 5.0f, .speed = 32.0f, .torque = 10.0f};
}

} // namespace

void MotorInterfaceTests::SetUp() {
  int s = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0) {
    throw std::runtime_error("Couldn't open socket, please check that you enabled the canline");
  }

  struct ifreq ifr{};
  struct sockaddr_can addr{};
  std::strcpy(ifr.ifr_ifrn.ifrn_name, canIfName.c_str());
  ::ioctl(s, SIOCGIFINDEX, &ifr);

  std::memset(&addr, 0, sizeof(struct sockaddr_can));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (::bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    throw std::runtime_error("Failed to bind to socket");
  }

  socketFD = s;
}

// ---------------------------------------------------------------------------
// Move-only semantics (compile-time only, no fixture needed)
// ---------------------------------------------------------------------------

TEST(MotorTypeTraits, MotorSubclassesAreMoveOnly) {
  static_assert(std::is_move_constructible_v<MitModeMotor>,
                "MitModeMotor should be move-constructible");
  static_assert(std::is_move_constructible_v<ServoModeMotor>,
                "ServoModeMotor should be move-constructible");
}

// ---------------------------------------------------------------------------
// Motor creation / validation
// ---------------------------------------------------------------------------

TEST_F(MotorInterfaceTests, CreateMitMotorNullLimitsThrows) {
  EXPECT_THROW(interface.createMitMotor(nullptr, 11), std::invalid_argument);
}

TEST_F(MotorInterfaceTests, CreateServoMotorNullLimitsThrows) {
  EXPECT_THROW(interface.createServoMotor(nullptr, 11), std::invalid_argument);
}

TEST_F(MotorInterfaceTests, CreateServoMotorWithEnumIsUsable) {
  ServoModeMotor m = interface.createServoMotor(AKSeriesMotor::AK45_10, 11);
  m.sendRPM(1.0f);
  can_frame frame = readFrame(socketFD);
  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::RPMmode));
  ASSERT_EQ(servoCanId(frame), 11);
}

// ---------------------------------------------------------------------------
// MIT mode
// ---------------------------------------------------------------------------

TEST_F(MotorInterfaceTests, MitSendMatchesMitSendFrameEncodingWithEnum) {
  const AKSeriesMotor motor = AKSeriesMotor::AK10_9;
  const uint32_t canId = 11;
  MitModeMotor m = interface.createMitMotor(motor, canId);

  MitRunSettings settings{1.0f, 2.0f, 3.0f, 50.0f, 1.0f};
  m.send(settings);
  can_frame actual = readFrame(socketFD);

  MitSendFrame expectedFrame(canId, motor, &settings);
  can_frame expected = static_cast<can_frame>(expectedFrame);

  ASSERT_EQ(actual.can_id, expected.can_id);
  ASSERT_EQ(std::memcmp(actual.data, expected.data, 8), 0);
}

TEST_F(MotorInterfaceTests, MitSendMatchesMitSendFrameEncodingWithCustomLimits) {
  MotorRunLimits customLims{.pos = 5.0f, .speed = 20.0f, .torque = 8.0f};
  const uint32_t canId = 12;
  MitModeMotor m = interface.createMitMotor(&customLims, canId);

  MitRunSettings settings{4.0f, 5.0f, 3.0f, 100.0f, 2.0f};
  m.send(settings);
  can_frame actual = readFrame(socketFD);

  MitSendFrame expectedFrame(canId, customLims, &settings);
  can_frame expected = static_cast<can_frame>(expectedFrame);
  ASSERT_EQ(actual.can_id, expected.can_id);
  ASSERT_EQ(std::memcmp(actual.data, expected.data, 8), 0);

  // Proves the custom limits were actually used: encoding the same settings against the
  // AK10_9 table entry (pos limit 12.5 instead of 5.0) should quantize differently.
  MitSendFrame defaultTableFrame(canId, AKSeriesMotor::AK10_9, &settings);
  can_frame defaultEncoded = static_cast<can_frame>(defaultTableFrame);
  EXPECT_NE(std::memcmp(actual.data, defaultEncoded.data, 8), 0);
}

TEST_F(MotorInterfaceTests, MitSendAndReceiveReturnsDecodedFrame) {
  const AKSeriesMotor motor = AKSeriesMotor::AK10_9;
  const uint32_t canId = 11;
  MitModeMotor m = interface.createMitMotor(motor, canId);
  const MotorRunLimits &lims = motorRunLimits[static_cast<uint8_t>(motor)];

  const uint8_t driveId = 11;
  int32_t rawPos{2000};
  rawPos = rawPos >> 4;
  const float expectedPos = uintToFloat(rawPos, -lims.pos, lims.pos, 28);
  const uint16_t rawCurrent{1500};
  const float expectedCurrent = uintToFloat(rawCurrent, -lims.torque, lims.torque, 12);
  const int8_t expectedTemp = -12;
  const ErrorCode expectedError = ErrorCode::EncoderFault;

  const int32_t posWire = rawPos << 4;
  can_frame reply{};
  reply.can_id = canId;
  reply.len = 8;
  reply.data[0] = driveId;
  reply.data[1] = static_cast<uint8_t>(posWire >> 24);
  reply.data[2] = static_cast<uint8_t>(posWire >> 16);
  reply.data[3] = static_cast<uint8_t>(posWire >> 8);
  reply.data[4] =
      static_cast<uint8_t>((((posWire >> 4) & LOW4BITS) << 4) | ((rawCurrent >> 8) & LOW4BITS));
  reply.data[5] = static_cast<uint8_t>(rawCurrent);
  reply.data[6] = static_cast<uint8_t>(expectedTemp);
  reply.data[7] = static_cast<uint8_t>(expectedError);

  ::write(socketFD, &reply, sizeof(can_frame));

  MitRunSettings settings{};
  std::optional<MitRecvFrame> recv = m.sendAndRecieve(settings);

  ASSERT_TRUE(recv.has_value());
  ASSERT_EQ(recv->getId(), driveId);
  ASSERT_NEAR(recv->getPosition(), expectedPos, 0.01f);
  ASSERT_EQ(recv->getCurrent(), expectedCurrent);
  ASSERT_EQ(recv->getTemperature(), expectedTemp);
  ASSERT_EQ(recv->getErrorCode(), expectedError);
}

TEST_F(MotorInterfaceTests, MitSendAndReceiveReturnsNulloptOnTimeout) {
  MitModeMotor m = interface.createMitMotor(AKSeriesMotor::AK10_9, 11);
  MitRunSettings settings{};
  std::optional<MitRecvFrame> recv = m.sendAndRecieve(settings);
  ASSERT_FALSE(recv.has_value());
}

// ---------------------------------------------------------------------------
// Servo mode
// ---------------------------------------------------------------------------

TEST_F(MotorInterfaceTests, ServoSendDutyCycleWireFormat) {
  MotorRunLimits customLims = customServoLimits();
  const uint32_t canId = 11;
  ServoModeMotor m = interface.createServoMotor(&customLims, canId);

  const float dutyCycle = 5.0f;
  m.sendDutyCycle(dutyCycle);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::DutyCycleMode));
  ASSERT_EQ(servoCanId(frame), canId);
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(dutyCycle * 100000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendDutyCycleClampsToTorqueLimit) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  m.sendDutyCycle(50.0f);
  can_frame highFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(highFrame.data), static_cast<int32_t>(customLims.torque * 100000.0f));

  m.sendDutyCycle(-50.0f);
  can_frame lowFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(lowFrame.data), static_cast<int32_t>(-customLims.torque * 100000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendCurrentLoopWireFormat) {
  MotorRunLimits customLims = customServoLimits();
  const uint32_t canId = 11;
  ServoModeMotor m = interface.createServoMotor(&customLims, canId);

  const float current = 4.0f;
  m.sendCurrentLoop(current);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::CurrentLoopMode));
  ASSERT_EQ(servoCanId(frame), canId);
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(current * 1000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendCurrentLoopClampsToTorqueLimit) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  m.sendCurrentLoop(50.0f);
  can_frame highFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(highFrame.data), static_cast<int32_t>(customLims.torque * 1000.0f));

  m.sendCurrentLoop(-50.0f);
  can_frame lowFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(lowFrame.data), static_cast<int32_t>(-customLims.torque * 1000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendCurrentBrakeWireFormat) {
  MotorRunLimits customLims = customServoLimits();
  const uint32_t canId = 11;
  ServoModeMotor m = interface.createServoMotor(&customLims, canId);

  const float brake = 3.0f;
  m.sendCurrentBrake(brake);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::CurrentBrakeMode));
  ASSERT_EQ(servoCanId(frame), canId);
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(brake * 1000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendCurrentBrakeClampsToTorqueLimit) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  m.sendCurrentBrake(50.0f);
  can_frame frame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(customLims.torque * 1000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendCurrentBrakeNegativeClampsToZero) {
  // The binding clamps -50 down to -torque; ServoSendFrame::setCurrentBrake then floors any
  // negative value to 0, since brake current is protocol-defined as [0, 60000].
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  m.sendCurrentBrake(-50.0f);
  can_frame frame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(frame.data), 0);
}

TEST_F(MotorInterfaceTests, ServoSendRPMWireFormat) {
  MotorRunLimits customLims = customServoLimits();
  const uint32_t canId = 11;
  ServoModeMotor m = interface.createServoMotor(&customLims, canId);

  const float rpm = 20.0f;
  m.sendRPM(rpm);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::RPMmode));
  ASSERT_EQ(servoCanId(frame), canId);
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(rpm));
}

TEST_F(MotorInterfaceTests, ServoSendRPMClampsToSpeedLimit) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  m.sendRPM(100.0f);
  can_frame highFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(highFrame.data), static_cast<int32_t>(customLims.speed));

  m.sendRPM(-100.0f);
  can_frame lowFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(lowFrame.data), static_cast<int32_t>(-customLims.speed));
}

TEST_F(MotorInterfaceTests, ServoSendRPMUsesCustomLimitsNotEnumTable) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  // AK45_10's table entry caps speed at 20.0; a value above that but below the custom 32.0
  // limit should pass through unclamped only if the custom limits are actually in effect.
  const float rpm = 25.0f;
  m.sendRPM(rpm);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(rpm))
      << "Expected the custom limits to be used instead of the AK45_10 enum table";
}

TEST_F(MotorInterfaceTests, ServoSendPositionWireFormat) {
  MotorRunLimits customLims = customServoLimits();
  const uint32_t canId = 11;
  ServoModeMotor m = interface.createServoMotor(&customLims, canId);

  const float pos = 2.0f;
  m.sendPosition(pos);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::PositionMode));
  ASSERT_EQ(servoCanId(frame), canId);
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(pos * 10000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendPositionClampsToPosLimit) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  m.sendPosition(20.0f);
  can_frame highFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(highFrame.data), static_cast<int32_t>(customLims.pos * 10000.0f));

  m.sendPosition(-20.0f);
  can_frame lowFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(lowFrame.data), static_cast<int32_t>(-customLims.pos * 10000.0f));
}

TEST_F(MotorInterfaceTests, ServoSendOriginWireFormat) {
  MotorRunLimits customLims = customServoLimits();
  const uint32_t canId = 11;
  ServoModeMotor m = interface.createServoMotor(&customLims, canId);

  m.sendOrigin(1);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::SetOriginMode));
  ASSERT_EQ(servoCanId(frame), canId);
  ASSERT_EQ(frame.data[0], 1);
}

TEST_F(MotorInterfaceTests, ServoSendOriginInvalidValuePropagatesThrow) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  EXPECT_THROW(m.sendOrigin(2), std::invalid_argument);
}

TEST_F(MotorInterfaceTests, ServoSendPositionAndVeloWireFormat) {
  MotorRunLimits customLims = customServoLimits();
  const uint32_t canId = 11;
  ServoModeMotor m = interface.createServoMotor(&customLims, canId);

  const float pos = 2.0f;
  const float speed = 10.0f;
  const float accel = 500.0f;
  m.sendPositionAndVelo(pos, speed, accel);
  can_frame frame = readFrame(socketFD);

  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::PositionVelocityMode));
  ASSERT_EQ(servoCanId(frame), canId);
  ASSERT_EQ(readInt32BE(frame.data), static_cast<int32_t>(pos * 10000.0f));
  ASSERT_EQ(readInt16BE(&frame.data[4]), static_cast<int16_t>(speed / 10.0f));
  ASSERT_EQ(readInt16BE(&frame.data[6]), static_cast<int16_t>(accel / 10.0f));
}

TEST_F(MotorInterfaceTests, ServoSendPositionAndVeloClampsPositionAndSpeed) {
  MotorRunLimits customLims = customServoLimits();
  ServoModeMotor m = interface.createServoMotor(&customLims, 11);

  m.sendPositionAndVelo(20.0f, 100.0f, 500.0f);
  can_frame highFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(highFrame.data), static_cast<int32_t>(customLims.pos * 10000.0f));
  ASSERT_EQ(readInt16BE(&highFrame.data[4]), static_cast<int16_t>(customLims.speed / 10.0f));

  m.sendPositionAndVelo(-20.0f, -100.0f, 500.0f);
  can_frame lowFrame = readFrame(socketFD);
  ASSERT_EQ(readInt32BE(lowFrame.data), static_cast<int32_t>(-customLims.pos * 10000.0f));
  ASSERT_EQ(readInt16BE(&lowFrame.data[4]), static_cast<int16_t>(-customLims.speed / 10.0f));
}

// ---------------------------------------------------------------------------
// AKSeriesInterface::readServoFrame
// ---------------------------------------------------------------------------

TEST_F(MotorInterfaceTests, ReadServoFrameReturnsDecodedFrame) {
  const canid_t canId = 11;
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
  ::write(socketFD, &frame, sizeof(can_frame));

  std::optional<ServoRecvFrame> recv = interface.readServoFrame();

  ASSERT_TRUE(recv.has_value());
  ASSERT_EQ(recv->getId(), canId);
  ASSERT_NEAR(recv->getPosition(), expectedPosition, 0.1f);
  ASSERT_EQ(recv->getSpeed(), expectedSpeed);
  ASSERT_FLOAT_EQ(recv->getCurrent(), expectedCurrent);
  ASSERT_EQ(recv->getTemperature(), expectedTemp);
  ASSERT_EQ(recv->getErrorCode(), expectedError);
}

TEST_F(MotorInterfaceTests, ReadServoFrameReturnsNulloptOnTimeout) {
  std::optional<ServoRecvFrame> recv = interface.readServoFrame();
  ASSERT_FALSE(recv.has_value());
}

// ---------------------------------------------------------------------------
// AKSeriesInterface move construction
//
// AKSeriesInterface declares these in include/AKSeries.hpp but they aren't defined yet in
// src/AKSeries.cpp, so run_tests will fail to link until they're implemented. That's expected.
// ---------------------------------------------------------------------------

TEST_F(MotorInterfaceTests, MoveConstructorTransfersUsableInterface) {
  AKSeriesInterface original(canIfName.c_str());
  AKSeriesInterface moved(std::move(original));

  ServoModeMotor m = moved.createServoMotor(AKSeriesMotor::AK45_10, 11);
  m.sendRPM(5.0f);

  can_frame frame = readFrame(socketFD);
  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::RPMmode));
  ASSERT_EQ(servoCanId(frame), 11);
}

TEST_F(MotorInterfaceTests, MoveAssignmentTransfersUsableInterface) {
  AKSeriesInterface original(canIfName.c_str());
  AKSeriesInterface other(canIfName.c_str());
  other = std::move(original);

  ServoModeMotor m = other.createServoMotor(AKSeriesMotor::AK45_10, 11);
  m.sendRPM(5.0f);

  can_frame frame = readFrame(socketFD);
  ASSERT_EQ(servoFrameId(frame), static_cast<uint8_t>(ServoFrameID::RPMmode));
  ASSERT_EQ(servoCanId(frame), 11);
}
