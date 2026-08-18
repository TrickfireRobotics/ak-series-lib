#include "AKSeries.hpp"
#include "can/MIT_frame.hpp"
#include "motors/Motors.hpp"
#include <can/Servo_frame.hpp>
#include <can/comms.hpp>
#include <stdexcept>

using namespace AKSeries;
AKSeriesInterface::AKSeriesInterface(const char *canif)
    : canInterface{std::shared_ptr<CanInterface>(new CanInterface(canif))} {}

AKSeriesInterface::~AKSeriesInterface() { canInterface.reset(); }

AKSeriesInterface::AKSeriesInterface(AKSeriesInterface &&other) noexcept {
  canInterface = other.canInterface;
  other.canInterface.reset();
}

AKSeriesInterface &AKSeriesInterface::operator=(AKSeriesInterface &&other) noexcept {
  AKSeriesInterface tmp(std::move(other));
  this->canInterface = tmp.canInterface;
  tmp.canInterface.reset();
  return *this;
}

Motor::Motor(AKSeriesMotor motor, uint32_t canId, std::shared_ptr<CanInterface> interface)
    : mInterface{interface}, mLims{&motorRunLimits[static_cast<uint8_t>(motor)]}, mCanId{canId} {}

Motor::Motor(const MotorRunLimits *lims, uint32_t canId, std::shared_ptr<CanInterface> interface)
    : mInterface{interface}, mLims{lims}, mCanId{canId} {}

MitModeMotor AKSeriesInterface::createMitMotor(const AKSeriesMotor motor, uint32_t canId) {
  return MitModeMotor(motor, canId, canInterface);
}
MitModeMotor AKSeriesInterface::createMitMotor(const MotorRunLimits *lims, uint32_t canId) {
  if (lims == nullptr) {
    throw std::invalid_argument(
        "Motor run limits passed to MitModeMotor object was null, pass valid run limits");
  }
  return MitModeMotor(lims, canId, canInterface);
}

ServoModeMotor AKSeriesInterface::createServoMotor(const AKSeriesMotor motor, uint32_t canId) {
  return ServoModeMotor(motor, canId, canInterface);
}
ServoModeMotor AKSeriesInterface::createServoMotor(const MotorRunLimits *lims, uint32_t canId) {
  if (lims == nullptr) {
    throw std::invalid_argument(
        "Motor run limits passed to ServoModeMotor object was null, pass valid run limits");
  }
  return ServoModeMotor(lims, canId, canInterface);
}

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
  if (dutyCycle < -mLims->torque || dutyCycle > mLims->torque) {
    std::fprintf(stderr, "Argument for PLACEHOLDER was invalid, rounding to limits");
    dutyCycle = mLims->torque < dutyCycle ? mLims->torque : -mLims->torque;
  }
  ServoSendFrame f = ServoSendFrame::setDutyCycle(mCanId, dutyCycle);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendCurrentLoop(float currentLoop) {
  if (currentLoop < -mLims->torque || currentLoop > mLims->torque) {
    std::fprintf(stderr, "Argument for current loop was invalid, rounding to limits");
    currentLoop = mLims->torque < currentLoop ? mLims->torque : -mLims->torque;
  }
  ServoSendFrame f = ServoSendFrame::setCurrentLoop(mCanId, currentLoop);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendCurrentBrake(float currentBrake) {
  if (currentBrake < -mLims->torque || currentBrake > mLims->torque) {
    std::fprintf(stderr, "Argument for current brake was invalid, rounding to limits");
    currentBrake = mLims->torque < currentBrake ? mLims->torque : -mLims->torque;
  }

  ServoSendFrame f = ServoSendFrame::setCurrentBrake(mCanId, currentBrake);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendRPM(float rpm) {
  if (rpm < -mLims->speed || rpm > mLims->speed) {
    std::fprintf(stderr, "Argument for rpm was invalid, rounding to limits");
    rpm = mLims->speed < rpm ? mLims->speed : -mLims->speed;
  }

  ServoSendFrame f = ServoSendFrame::setRPM(mCanId, rpm);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

void ServoModeMotor::sendPosition(float pos) {
  if (pos < -mLims->pos || pos > mLims->pos) {
    std::fprintf(stderr, "Argument for pos was invalid, rounding to limits");
    pos = mLims->pos < pos ? mLims->pos : -mLims->pos;
  }
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
  if (pos < -mLims->pos || pos > mLims->pos) {
    std::fprintf(stderr, "Argument for pos was invalid, rounding to limits");
    pos = mLims->pos < pos ? mLims->pos : -mLims->pos;
  }
  if (speed < -mLims->speed || speed > mLims->speed) {
    std::fprintf(stderr, "Argument for speed was invalid, rounding to limits");
    speed = mLims->speed < speed ? mLims->speed : -mLims->speed;
  }
  // Accel check gets done in the constructor for the object, ignoring
  ServoSendFrame f = ServoSendFrame::setPositionAndVelo(mCanId, pos, speed, accel);
  can_frame sf = static_cast<can_frame>(f);
  CanIOError err = this->mInterface->send(sf);
  if (err != CanIOError::NONE) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
  }
}

std::optional<ServoRecvFrame> AKSeriesInterface::readServoFrame() {
  auto f = canInterface->read();
  if (!f.successful) {
    std::fprintf(stderr, "IOError in the received can frame, logging");
    return std::nullopt;
  }
  return ServoRecvFrame(f.success);
}
