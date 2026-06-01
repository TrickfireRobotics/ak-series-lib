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
  canid_t m_can_id;
  const uint8_t len{8};
  std::array<uint8_t, 8> m_data{};

public:
  Frame() = delete;
  Frame(canid_t id) : m_can_id{id} { std::memset(m_data.begin(), 0, 8); };
  // Every can frame should be an easy conversion
  virtual explicit operator can_frame() const;
};
#endif
