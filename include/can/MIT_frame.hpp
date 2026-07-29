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

#include "Errors.hpp"
#include "can/frame.hpp"
#include "motors/Motors.hpp"

#define LOW4BITS (15)
#define HIGH4BITS (15 << 4)

void beginMitMode(canid_t can_id);

class MitSendFrame : public Frame {
private:
  AKSeriesMotor mMotor{};
  const MotorRunLimits &mLims;
  void padData();

public:
  MitSendFrame() = delete;
  MitRunSettings *mSettings{nullptr};
  explicit MitSendFrame(canid_t can_id, AKSeriesMotor motor, MitRunSettings *settings = nullptr)
      : Frame(can_id), mMotor{motor},
        mSettings{settings == nullptr ? new MitRunSettings() : settings},
        mLims{motorRunLimits[static_cast<uint8_t>(mMotor)]} {}
  explicit operator can_frame();
};

class MitRecvFrame : public Frame {
private:
  AKSeriesMotor mMotor;
  const MotorRunLimits &mLims;

public:
  MitRecvFrame() = delete;
  MitRecvFrame(const can_frame &frame, AKSeriesMotor motor);
  uint8_t getId();
  float getPosition();
  float getCurrent();
  int8_t getTemperature();
  ErrorCode getErrorCode();
  explicit operator can_frame();
};

#endif
