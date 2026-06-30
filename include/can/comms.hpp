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

enum class IOError {
  TIMEOUT = 0,
  SOCKET_ERROR,
  BIND_ERROR,
  FAILED_WRITE,
  FAILED_READ,
  FAILED_POLL,
  NONE,
};

using std::shared_ptr;

namespace AKSeries {
class CanWriter {
  struct WriterPair {
    const char *name{nullptr};
    shared_ptr<CanWriter> ptr{nullptr};
    int canFd;
    WriterPair() : canFd{-1} {}
    WriterPair(const char *str, CanWriter *w, int fd)
        : name{str}, ptr{shared_ptr<CanWriter>(w)}, canFd{fd} {}
  };

  inline static int writers = 4;
  inline static WriterPair *avlblWriters = new WriterPair[4];
  /*
   can0, can1, vcan0 vcan1 are typcally the 4 defaults
   allocating some extra memory helps store these just for safety
   */

  CanWriter(const char *);
  /*
  This returns the FD for the canline file, DO NOT DISCARD
  This also should only run once per can file,
  it will error out if attempted to call on same canline
  multiple times
   */
  [[nodiscard]] Expected<int, IOError> initCan(const char *);
  const char *canIF;
  int canFD{-1};
  uint32_t pollTime{10};

public:
  /*
  This class is a factory, we dont want people making them willy nilly and should manage the
  resources ourselves
  */

  CanWriter() = delete;
  static shared_ptr<CanWriter> getCanWriter(const char *canIF);
  /*
  Use for mit mode preferably
  No discard because we have a method to circumvent the received frame
  which will be more performant
  */
  [[nodiscard]] Expected<can_frame, IOError> sendAndRead(can_frame &frame);
  /*
  Can be used for mit mode if we dont care about the return frame
  preferably use for servo mode
  */
  IOError send(can_frame &frame);
  Expected<can_frame, IOError> read();
  void setPollTimeout(uint32_t pTime) { pollTime = pTime; };
  // if someone deletes a can writer it should remove itself from the list
  ~CanWriter();
  /*
   We dont want people moving or modifying the values,
   the CanWriter should be a resource one accesses and thats it
   */
  CanWriter(const CanWriter &) = delete;
  CanWriter &operator=(const CanWriter &) = delete;
  CanWriter &operator=(CanWriter &&) = delete;
  CanWriter(CanWriter &&) = delete;
};

}; // namespace AKSeries

#endif
