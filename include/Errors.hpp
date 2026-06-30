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
/*
This should ONLY be used internally in the library
*/

template <typename T, typename U> struct Expected {
  union {
    T sVal;
    U fVal;
  };
  bool successful;
  Expected(T val) : sVal{val}, successful{true} {};
  Expected(U val) : fVal{val}, successful{false} {};
};

} // namespace AKSeries

#endif
