#ifndef __AK_SERIES_INTERFACE
#define __AK_SERIES_INTERFACE
#pragma once
#include <Errors.hpp>
#include <can/MIT_frame.hpp>
#include <can/comms.hpp>
#include <memory>
#include <motors/Motors.hpp>
#include <optional>
#include <stdint.h>

namespace AKSeries {

class AKSeriesInterface;

class Motor {
  friend class AKSeriesInterface;

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
public:
  [[nodiscard]] std::optional<MitRecvFrame> sendAndRecieve(MitRunSettings &);
  void send(MitRunSettings &);
};

class ServoModeMotor : public Motor {
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
  AKSeriesInterface(const char *canif);
  AKSeriesInterface() = delete;
  Motor createMotor(AKSeriesMotor motor, uint32_t);
  Motor createMotor(const MotorRunLimits *motor, uint32_t);

  AKSeriesInterface(const AKSeriesInterface &);
  AKSeriesInterface &operator=(const AKSeriesInterface &);
  AKSeriesInterface(AKSeriesInterface &&) noexcept;
  AKSeriesInterface &operator=(AKSeriesInterface &&) noexcept;

  // Be careful NOT to delete the resource
  // Additionally shouldnt be allowed to delete itself if motors still exist in scope
  ~AKSeriesInterface();
};

} // namespace AKSeries

#endif
