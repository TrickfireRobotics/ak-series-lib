#include "can/comms.hpp"
#include "Errors.hpp"
#include <cerrno>
#include <cstdio>
#include <exception>

using AKSeries::Expected, AKSeries::CanInterface;

CanInterface::CanInterface(const char *canif) {
  auto canID = initCan(canif);
  if (!canID.successful) {
    std::fprintf(stderr, "Failed to create CanInterface object for %s\n", canif);
    return;
  }
  canIF = canif;
  canFD = canID.success;
}

// TODO implement proper return values
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
  size_t s = ::write(canFD, &frame, sizeof(can_frame));
  if (s < sizeof(can_frame) && (errno & static_cast<int>(CanIOError::RESOURCE_UNAVAILABLE) ||
                                errno & static_cast<int>(CanIOError::NO_BUFFER_SPACE) ||
                                errno & static_cast<int>(CanIOError::BUSY))) {
    return Expected<can_frame, CanIOError>(static_cast<CanIOError>(errno));
  } else if (s < sizeof(can_frame)) {
    return Expected<can_frame, CanIOError>(CanIOError::FAILED_WRITE);
  }
  struct pollfd pfd;
  pfd.fd = canFD;
  pfd.events = (POLLERR | POLLHUP | POLLIN);
  pfd.revents = (POLLERR | POLLHUP | POLLIN);
  ::poll(&pfd, 1, pollTime);

  if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
    return Expected<can_frame, CanIOError>(CanIOError::NO_DATA);
  }
  if (pfd.revents & POLLIN) {
    can_frame f{};
    std::memset(&f, 0, sizeof(can_frame));
    ::read(canFD, &f, sizeof(can_frame));
    return (Expected<can_frame, CanIOError>(f));
  } else {
    return Expected<can_frame, CanIOError>(CanIOError::TIMEOUT);
  }
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
  pfd.revents = (POLLERR | POLLHUP | POLLIN);
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

// TODO

std::shared_ptr<CanInterface> CanInterface::getCanInterface(const char *canIF) {

  for (size_t i{0}; i < NUMBER_OF_CANLINES; i++) {
    if (CanInterface::writers[i].name != nullptr &&
        std::strcmp(CanInterface::writers[i].name, canIF) == 0) {
      return CanInterface::writers[i].ptr;
    }
  }
  // Else we need to make a new one
  CanInterface *wNew = new CanInterface(canIF);
  bool f{false};
  size_t i;
  for (i = 0; i < NUMBER_OF_CANLINES; i++) {
    if (CanInterface::writers[i].name == nullptr) {
      WriterPair newPair{canIF, wNew, wNew->canFD};
      CanInterface::writers[i] = newPair;
      f = true;
      break;
    }
  }
  if (!f) {
    std::fprintf(stderr, "Was unable to allocate space for the canline please change compile flag "
                         "to allow for number of necessary canlines");
  }
  return CanInterface::writers[i].ptr;
}

void CanInterface::deleteInterface(const char *canIf) {
  bool found{false};
  int i;
  for (i = 0; i < NUMBER_OF_CANLINES; i++) {
    if (CanInterface::writers[i].name != nullptr &&
        std::strcmp(canIf, CanInterface::writers[i].name) == 0) {
      found = true;
      break;
    }
  }
  if (!found) {
    std::fprintf(stderr, "Failed to delete can writer, canwriter wasnt found in internal array\n");
  }
  // Grab resource for now
  auto ptr = CanInterface::writers[i].ptr;
  // Delete resource
  for (int j{i}; j < NUMBER_OF_CANLINES - 1; j++) {
    CanInterface::writers[j] = CanInterface::writers[j + 1];
  }
  CanInterface::writers[NUMBER_OF_CANLINES - 1] = WriterPair{};
  // Delete resource
  // If other pointers to it dangle resource will continue to exist but it will go out of scope when
  // the resource does
  ptr.reset();
}
