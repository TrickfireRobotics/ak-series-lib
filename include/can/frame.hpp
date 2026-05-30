#ifndef __FRAME_AK_SERIES
#pragma once
#include <array>
#include <stdint.h>

class Frame {
public:
  Frame() = default;

private:
  std::array<uint8_t, 8> data{};
};

#endif
