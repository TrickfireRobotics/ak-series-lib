#ifndef __AK_SERIES_INTERFACE
#define __AK_SERIES_INTERFACE
#pragma once
#include <Errors.hpp>
#include <can/MIT_frame.hpp>
#include <can/Servo_frame.hpp>
#include <memory>
#include <motors/Motors.hpp>
#include <optional>
#include <stdint.h>

namespace AKSeries {
class CanInterface;
class AKSeriesInterface;

class Motor {
protected:
  std::shared_ptr<CanInterface> mInterface;
  Motor(AKSeriesMotor motor, uint32_t canId, std::shared_ptr<CanInterface> interface);
  Motor(const MotorRunLimits *motor, uint32_t canId, std::shared_ptr<CanInterface> interface);
  const MotorRunLimits *mLims;
  uint32_t mCanId;

public:
  Motor() = delete;
  Motor(const Motor &) = delete;
  Motor &operator=(const Motor &) = delete;
  Motor(Motor &&) noexcept = default;
  Motor &operator=(Motor &&) noexcept = default;
  virtual ~Motor() = default;
};

class MitModeMotor : public Motor {
  friend class AKSeriesInterface;
  MitModeMotor(AKSeriesMotor motor, uint32_t canId, std::shared_ptr<CanInterface> interface)
      : Motor(motor, canId, interface) {}
  MitModeMotor(const MotorRunLimits *motor, uint32_t canId, std::shared_ptr<CanInterface> interface)
      : Motor(motor, canId, interface) {}

public:
  [[nodiscard]] std::optional<MitRecvFrame> sendAndRecieve(MitRunSettings &);
  void send(MitRunSettings &);
};

class ServoModeMotor : public Motor {
  friend class AKSeriesInterface;
  ServoModeMotor(AKSeriesMotor motor, uint32_t canId, std::shared_ptr<CanInterface> interface)
      : Motor(motor, canId, interface) {}
  ServoModeMotor(const MotorRunLimits *motor, uint32_t canId,
                 std::shared_ptr<CanInterface> interface)
      : Motor(motor, canId, interface) {}

public:
  void sendDutyCycle(float dutyCycle);
  void sendCurrentLoop(float currentLoop);
  void sendCurrentBrake(float current);
  void sendRPM(float rpm);
  void sendPosition(float pos);
  void sendOrigin(uint8_t origin_mode);
  void sendPositionAndVelo(float position, float speed, float accel);
};

class AKSeriesInterface {
  std::shared_ptr<CanInterface> canInterface;

public:
  explicit AKSeriesInterface(const char *canif);
  AKSeriesInterface() = delete;

  MitModeMotor createMitMotor(const AKSeriesMotor, uint32_t);
  MitModeMotor createMitMotor(const MotorRunLimits *, uint32_t);
  ServoModeMotor createServoMotor(const AKSeriesMotor, uint32_t);
  ServoModeMotor createServoMotor(const MotorRunLimits *, uint32_t);
  AKSeriesInterface(const AKSeriesInterface &) = delete;
  AKSeriesInterface &operator=(const AKSeriesInterface &) = delete;
  AKSeriesInterface(AKSeriesInterface &&) noexcept;
  AKSeriesInterface &operator=(AKSeriesInterface &&) noexcept;

  std::optional<ServoRecvFrame> readServoFrame();
  // Be careful NOT to delete the resource
  // Additionally shouldnt be allowed to delete itself if motors still exist in scope
  ~AKSeriesInterface();
};

} // namespace AKSeries

#endif
