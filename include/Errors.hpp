#ifndef __EXCEPTIONS_AK_SERIES
#define __EXCEPTIONS_AK_SERIES

#pragma once

#ifdef __cplusplus
extern "C" {
#include <linux/can.h>
}
#else
#include <linux/can.h>
#endif

#include <stdexcept>

enum class ErrorCode {
  NoFault = 0,
  OverTemperature,
  OverCurrent,
  OverVoltage,
  UnderVoltage,
  EncoderFault,
  MOSFETOverTemp,
  MotorStall,
};

static const char *errCodeToStr(ErrorCode s) {
  switch (s) {
  case (ErrorCode::NoFault):
    return "No fault";
  case (ErrorCode::OverTemperature):
    return "Over Temperature";
  case (ErrorCode::OverCurrent):
    return "Over current";
  case (ErrorCode::OverVoltage):
    return "Over voltage";
  case (ErrorCode::UnderVoltage):
    return "Under voltage";
  case (ErrorCode::EncoderFault):
    return "Encoder fault";
  case (ErrorCode::MOSFETOverTemp):
    return "Mosfet over temperature";
  case (ErrorCode::MotorStall):
    return "Motor stall";
  };
}

namespace AKSeries {
template <typename T, typename U> struct Expected {
  T success;
  U failed;
  bool successful;
};

} // namespace AKSeries

#endif
