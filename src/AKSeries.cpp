#include "AKSeries.hpp"
#include "motors/Motors.hpp"
#include <can/comms.hpp>

using namespace AKSeries;
AKSeriesInterface::AKSeriesInterface(const char *canif)
    : canInterface{std::shared_ptr<CanInterface>(new CanInterface(canif))} {}

template <typename T> Motor AKSeriesInterface::createMotor(T motor, uint32_t canID, bool mitMode) {
  static_assert(std::is_same_v<T, AKSeriesMotor> || std::is_same_v<T, MotorRunLimits>,
                "Can only construct using AKSeriesMotor or MotorRunLimits objects");
  Motor m(motor, canID, canInterface, mitMode);
  return m;
}

//
template <typename T>
Motor::Motor(T m, uint32_t canId, std::shared_ptr<CanInterface> interface, bool mitMode)
    : mInterface{interface}, mMitMode{mitMode} {
  // TODO finish template programming
  if constexpr (std::is_same_v<T, AKSeriesMotor>) {
    //
  } else if constexpr (std::is_same_v<T, MotorRunLimits>) {
    //
  } else {
    static_assert(!sizeof(T *), "unsupported type");
  }
}
