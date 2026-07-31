#include "AKSeries.hpp"
#include "can/MIT_frame.hpp"
#include "motors/Motors.hpp"
#include <can/Servo_frame.hpp>
#include <can/comms.hpp>
#include <stdexcept>

using namespace AKSeries;
AKSeriesInterface::AKSeriesInterface(const char *canif)
    : canInterface{std::shared_ptr<CanInterface>(new CanInterface(canif))} {}

Motor AKSeriesInterface::createMotor(AKSeriesMotor motor, uint32_t canID) {
  Motor m(motor, canID, canInterface);
  return m;
}
Motor AKSeriesInterface::createMotor(const MotorRunLimits *motor, uint32_t canID) {
  if (motor == nullptr) {
    throw std::invalid_argument("Invalid argument passed to create motor object, please pass a "
                                "valid pointer `MotorRunLimits` object");
  }
  Motor m(motor, canID, canInterface);
  return m;
}

Motor::Motor(AKSeriesMotor motor, uint32_t canId, std::shared_ptr<CanInterface> interface)
    : mInterface{interface}, mLims{&motorRunLimits[static_cast<uint8_t>(motor)]}, mCanId{canId} {}

Motor::Motor(const MotorRunLimits *lims, uint32_t canId, std::shared_ptr<CanInterface> interface)
    : mInterface{interface}, mLims{lims}, mCanId{canId} {}

// TODO finish implementing both MIT and servo mode binding

std::optional<MitRecvFrame> MitModeMotor::sendAndRecieve(MitRunSettings &settings) {
  MitSendFrame f(static_cast<canid_t>(mCanId), *this->mLims, &settings);
  can_frame sf = static_cast<can_frame>(f);

  auto recv = this->mInterface->sendAndRead(sf);

  if (!recv.successful) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
    return std::nullopt;
  }
  return MitRecvFrame(recv.success, *this->mLims);
}
void MitModeMotor::send(MitRunSettings &settings) {
  MitSendFrame f(static_cast<canid_t>(mCanId), *this->mLims, &settings);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendDutyCycle(float dutyCycle) {
  ServoSendFrame f = ServoSendFrame::setDutyCycle(mCanId, dutyCycle);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendCurrentLoop(float currentLoop) {
  ServoSendFrame f = ServoSendFrame::setCurrentLoop(mCanId, currentLoop);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendCurrentBrake(float currentBrake) {
  ServoSendFrame f = ServoSendFrame::setCurrentBrake(mCanId, currentBrake);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendRPM(float rpm) {
  ServoSendFrame f = ServoSendFrame::setRPM(mCanId, rpm);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendPosition(float pos) {
  ServoSendFrame f = ServoSendFrame::setPosition(mCanId, pos);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendOrigin(uint8_t originMode) {
  ServoSendFrame f = ServoSendFrame::setOrigin(mCanId, originMode);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendPositionAndVelo(float pos, float speed, float accel) {
  ServoSendFrame f = ServoSendFrame::setPositionAndVelo(mCanId, pos, speed, accel);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}
