#ifndef __COMMS_AK_SERIES
#define __COMMS_AK_SERIES
#pragma once

#ifndef NUMBER_OF_CANLINES
#define NUMBER_OF_CANLINES 2
#endif

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
  struct WriterPair {
    const char *name{nullptr};
    shared_ptr<CanInterface> ptr{nullptr};
    int canFd;
    WriterPair() : canFd{-1} {}
    WriterPair(const char *str, CanInterface *w, int fd)
        : name{str}, ptr{shared_ptr<CanInterface>(w)}, canFd{fd} {}
  };
  CanInterface(const char *);
  /*
  This returns the FD for the canline file, DO NOT DISCARD
  This also should only run once per can file,
  it will error out if attempted to call on same canline
  multiple times
   */
  [[nodiscard]] Expected<int, CanIOError> initCan(const char *);
  const char *canIF;
  int canFD{-1};
  uint32_t pollTime{10};

  /*
   can0, can1, vcan0 vcan1 are typcally the 4 defaults
   allocating some extra memory helps store these just for safety

   I wanted to add this to reduce some compilation overhead during testing
   saves us from adding a friend method here to enable it
  */
#ifndef BUILD_TESTING
  inline static WriterPair writers[NUMBER_OF_CANLINES]{};

public:
#else
public:
  inline static WriterPair writers[NUMBER_OF_CANLINES]{};
#endif

  /*
  This class is a factory, we dont want people making them willy nilly and should manage the
  resources ourselves
  */
  CanInterface() = delete;
  static shared_ptr<CanInterface> getCanInterface(const char *canIF);
  // Must be called when you are finished with a writer/in the destructor of the motor object
  static void deleteInterface(const char *canif);
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
