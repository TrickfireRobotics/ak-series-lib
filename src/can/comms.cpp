#include "can/comms.hpp"
#include "Errors.hpp"
#include <cerrno>
#include <cstdio>

using AKSeries::CanInterface;
using AKSeries::Expected;

CanInterface::CanInterface(const char *canif) {
  auto canID = CanInterface::initCan(canif);
  if (!canID.successful) {
    std::fprintf(stderr, "Failed to create CanInterface object for %s\n", canif);
    return;
  }
  canIF = canif;
  canFD = canID.value;
}

Expected<int, CanIOError> CanInterface::initCan(const char *str) {
  int s = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0) {
    std::fprintf(stderr, "Failed to initialize can socket file descriptor");
    return Expected<int, CanIOError>(CanIOError::SOCKET_ERROR);
  }
  struct ifreq ifr;
  struct sockaddr_can addr;
  std::strcpy(ifr.ifr_ifrn.ifrn_name, str);
  ::ioctl(s, SIOCGIFINDEX, &ifr);

  std::memset(&addr, 0, sizeof(sockaddr_can));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifru.ifru_ivalue;
  // C style cast, techincally unsafe but since its a C struct theres no way
  // to safely cast to the other

  if (::bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    std::fprintf(stderr, "Failed to bind to can address socket");
    return Expected<int, CanIOError>(CanIOError::BIND_ERROR);
  }
  return Expected<int, CanIOError>(s);
}

Expected<can_frame, CanIOError> CanInterface::sendAndRead(can_frame &frame) {
  auto result = this->send(frame);
  if (result != CanIOError::NONE) {
    return Expected<can_frame, CanIOError>(result);
  }
  auto readResult = this->read();
  return readResult;
}

CanIOError CanInterface::send(can_frame &frame) {
  size_t sWrite = ::write(canFD, &frame, sizeof(can_frame));
  if (sWrite < sizeof(can_frame) && (errno & static_cast<int>(CanIOError::RESOURCE_UNAVAILABLE) ||
                                     errno & static_cast<int>(CanIOError::NO_BUFFER_SPACE) ||
                                     errno & static_cast<int>(CanIOError::BUSY))) {
    return static_cast<CanIOError>(errno);
  } else if (sWrite < sizeof(can_frame)) {
    return CanIOError::FAILED_WRITE;
  }

  return CanIOError::NONE;
}

Expected<can_frame, CanIOError> CanInterface::read() {
  can_frame f{};
  struct pollfd pfd;
  pfd.fd = canFD;
  pfd.events = (POLLERR | POLLHUP | POLLIN);
  ::poll(&pfd, 1, pollTime);
  if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
    return Expected<can_frame, CanIOError>(CanIOError::NO_DATA);
  }
  if (pfd.revents & POLLIN) {
    size_t s = ::read(canFD, &f, sizeof(can_frame));
    if (s < sizeof(can_frame)) {
      return Expected<can_frame, CanIOError>(CanIOError::FAILED_READ);
    }
    return Expected<can_frame, CanIOError>(f);
  } else {
    return Expected<can_frame, CanIOError>(CanIOError::TIMEOUT);
  }
}
