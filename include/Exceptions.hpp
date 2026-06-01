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

enum class ServoErrorCode {
  NoFault = 0,
  OverTemperature,
  OverCurrent,
  OverVoltage,
  UnderVoltage,
  EncoderFault,
  MOSFETOverTemp,
  MotorStall,
};

static const char *errCodeToStr(ServoErrorCode s) {
  switch (s) {
  case (ServoErrorCode::NoFault):
    return "No fault";
  case (ServoErrorCode::OverTemperature):
    return "Over Temperature";
  case (ServoErrorCode::OverCurrent):
    return "Over current";
  case (ServoErrorCode::OverVoltage):
    return "Over voltage";
  case (ServoErrorCode::UnderVoltage):
    return "Under voltage";
  case (ServoErrorCode::EncoderFault):
    return "Encoder fault";
  case (ServoErrorCode::MOSFETOverTemp):
    return "Mosfet over temperature";
  case (ServoErrorCode::MotorStall):
    return "Motor stall";
  };
}

namespace aks {

class invalid_call : public std::runtime_error {
private:
  std::string desc{};

public:
  invalid_call(const std::string &d) : std::runtime_error(d) {}
};

class ServoException : public std::exception {
private:
  ServoErrorCode code{};
  std::string desc{};

public:
  ServoException(const std::string err, ServoErrorCode s = ServoErrorCode::NoFault)
      : std::exception(), desc{err}, code{s} {};
  const char *what() {
    if (code == ServoErrorCode::NoFault) {
      return desc.c_str();
    } else {
      desc += ": ";
      desc += errCodeToStr(code);
      return desc.c_str();
    }
  }
};

} // namespace aks

#endif
