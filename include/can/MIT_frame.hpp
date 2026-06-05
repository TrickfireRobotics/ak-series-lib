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
  AkSeriesMotors mMotor{};
  const MotorRunLimits &mLims;
  void padData();

public:
  MitRunSettings *mSettings{nullptr};
  MitSendFrame(canid_t can_id, AkSeriesMotors motor, MitRunSettings *settings = nullptr)
      : Frame(can_id), mMotor{motor},
        mSettings{settings == nullptr ? new MitRunSettings : settings},
        mLims{motorRunLimits[static_cast<uint8_t>(mMotor)]} {}

  MitSendFrame() = delete;
  explicit operator can_frame();
};

class MitRecvFrame : public Frame {
private:
  AkSeriesMotors mMotor;
  const MotorRunLimits &mLims;

public:
  MitRecvFrame() = delete;
  MitRecvFrame(const can_frame &frame, AkSeriesMotors motor);
  uint8_t getId();
  float getPosition();
  float getCurrent();
  uint8_t getTemperature();
  ErrorCode getErrorCode();
};

#endif
