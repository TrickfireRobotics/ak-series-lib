#ifndef __AK_SERIES_INTERFACE
#define __AK_SERIES_INTERFACE
#pragma once
#include <Errors.hpp>
#include <can/comms.hpp>
#include <memory>
#include <motors/Motors.hpp>
#include <stdint.h>

namespace AKSeries {

class AKSeriesInterface;

class Motor {
  friend class AKSeriesInterface;
  std::shared_ptr<CanInterface> mInterface;
  bool mMitMode;
  template <typename T>
  Motor(T motor, uint32_t canId, std::shared_ptr<CanInterface> interface, bool mitMode = true);
  MotorRunLimits lims;
  AKSeriesMotor motor;

public:
  // TODO finish writing all servo and Mit mode bindings
  bool isMitMode() { return mMitMode; }
  // MITMode bindings
  can_frame sendAndReceiveMit(MitRunSettings &m);
  void sendMit(MitRunSettings &m);
  // ServoModeBindings

  Motor() = delete;
  // Object should be moved
  Motor(const Motor &) = delete;
  Motor &operator=(const Motor &) = delete;
  Motor(Motor &&) noexcept;
  Motor &operator=(Motor &&) noexcept;
  // Should properly decrement refcount
  ~Motor();
};

class AKSeriesInterface {
  std::shared_ptr<CanInterface> canInterface;

public:
  AKSeriesInterface(const char *canif);
  AKSeriesInterface() = delete;
  template <typename T> Motor createMotor(T motor, uint32_t, bool mitMode = true);

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
