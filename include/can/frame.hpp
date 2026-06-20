#ifndef __FRAME_AK_SERIES
#define __FRAME_AK_SERIES

#pragma once
#ifdef __cplusplus
extern "C" {
#include <linux/can.h>
}
#endif

#include <array>
#include <cstring>
#include <stdint.h>

typedef struct can_frame can_frame;

class Frame {
protected:
  canid_t mCanId;
  const uint8_t len{8};
  std::array<uint8_t, 8> mData{};

public:
  Frame() = delete;
  Frame(canid_t id) : mCanId{id} { std::memset(mData.begin(), 0, 8); }
  // Every can frame should be an easy conversion
  virtual explicit operator can_frame() {
    can_frame frame{};
    return frame;
  };
};
#endif
