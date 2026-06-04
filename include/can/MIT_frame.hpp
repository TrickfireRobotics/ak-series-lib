#ifndef __MIT_AK_SERIES
#define __MIT_AK_SERIES
#pragma once

#ifdef __cplusplus
extern "C" {
#include <linux/can.h>
}
#else
#include <linux/can.h>
#endif

#include "Exceptions.hpp"
#include "can/frame.hpp"
#include "motors/MotorLimits.hpp"

#define FIRST4BYTES (15 << 4)
#define LAST4BYTES (15)

typedef struct MitRunSettings {
  // Rads
  float position{};
  // Rads/s
  float speed{};
  // current
  float current{};
  float KP{};
  float KD{};
} MitRunSettings;

void beginMitMode(canid_t can_id);

class MitSendFrame : public Frame {
private:
  AkSeriesMotors m_motor{};
  void padData();

public:
  MitRunSettings *m_settings{nullptr};
  MitSendFrame() = delete;
  MitSendFrame(canid_t can_id, AkSeriesMotors motor, MitRunSettings *settings = nullptr)
      : Frame(can_id), m_motor{motor},
        m_settings{settings == nullptr ? new MitRunSettings : settings} {}
  explicit operator can_frame();
};

class MitRecvFrame : public Frame {
private:
public:
  MitRecvFrame(const can_frame &frame);
  uint8_t getId();
  int32_t getPosition();
  float getCurrent();
  uint8_t getTemperature();
  ErrorCode getErrorCode();
};

#endif
