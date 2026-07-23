#ifndef __COMMS_AK_SERIES
#define __COMMS_AK_SERIES
#pragma once

#ifdef __cplusplus
extern "C" {
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
}
#else
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include "Errors.hpp"
#include <cstring>
#include <iostream>
#include <memory>

enum class CanIOError {
  TIMEOUT = 0,
  SOCKET_ERROR,
  BIND_ERROR,
  FAILED_WRITE,
  FAILED_READ,
  NO_DATA,
  BUSY = 16,
  RESOURCE_UNAVAILABLE = 35,
  NO_BUFFER_SPACE = 55,
  NONE,
};

using std::shared_ptr;

namespace AKSeries {
class CanInterface {
  const char *canIF;
  int canFD{-1};
  uint32_t pollTime{10};

public:
  CanInterface(const char *);
  [[nodiscard]] static Expected<int, CanIOError> initCan(const char *);
  CanInterface() = delete;
  /*
Use for mit mode preferably
No discard because we have a method to circumvent the received frame
which will be more performant
*/
  [[nodiscard]] Expected<can_frame, CanIOError> sendAndRead(can_frame &frame);
  /*
  Can be used for mit mode if we dont care about the return frame
  preferably use for servo mode
  */
  CanIOError send(can_frame &frame);
  Expected<can_frame, CanIOError> read();
  void setPollTimeout(uint32_t pTime) { pollTime = pTime; };

  ~CanInterface() = default;
  /*
   We dont want people moving or modifying the values,
   the CanInterface should be a resource one accesses and thats it
   */
  CanInterface(const CanInterface &) = delete;
  CanInterface &operator=(const CanInterface &) = delete;
  CanInterface &operator=(CanInterface &&) = delete;
  CanInterface(CanInterface &&) = delete;
};

}; // namespace AKSeries

#endif
