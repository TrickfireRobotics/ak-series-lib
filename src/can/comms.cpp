#include "can/comms.hpp"
#include "Errors.hpp"
#include <cstdio>
#include <exception>

using AKSeries::Expected, AKSeries::CanWriter;

CanWriter::CanWriter(const char *canif) {
  auto canID = initCan(canif);
  if (!canID.successful) {
    std::fprintf(stderr, "Failed to create CanWriter object for %s\n", canif);
    return;
  }
  canIF = canif;
  canFD = canID.sVal;
}

// TODO implement proper return values
Expected<int, IOError> CanWriter::initCan(const char *str) {
  int s = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0) {
    std::fprintf(stderr, "Failed to initialize can socket file descriptor");
    return Expected<int, IOError>(IOError::SOCKET_ERROR);
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
    return Expected<int, IOError>(IOError::BIND_ERROR);
  }
  return Expected<int, IOError>(s);
}

Expected<can_frame, IOError> CanWriter::sendAndRead(can_frame &frame) {
  size_t s = ::write(canFD, &frame, sizeof(can_frame));
  if (s < sizeof(can_frame)) {
    return Expected<can_frame, IOError>(IOError::FAILED_WRITE);
  }
  struct pollfd pfd;
  pfd.fd = canFD;
  pfd.events = (POLLERR | POLLHUP | POLLIN);
  pfd.revents = (POLLERR | POLLHUP | POLLIN);
  ::poll(&pfd, 1, pollTime);

  if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
    return Expected<can_frame, IOError>(IOError::FAILED_POLL);
  }
  if (pfd.revents & POLLIN) {
    can_frame f{};
    std::memset(&f, 0, sizeof(can_frame));
    ::read(canFD, &f, sizeof(can_frame));
    return (Expected<can_frame, IOError>(f));
  } else {
    return Expected<can_frame, IOError>(IOError::TIMEOUT);
  }
}

IOError CanWriter::send(can_frame &frame) {
  size_t sWrite = ::write(canFD, &frame, sizeof(can_frame));
  if (sWrite < sizeof(can_frame)) {
    return IOError::FAILED_WRITE;
  }

  return IOError::NONE;
}

Expected<can_frame, IOError> CanWriter::read() {
  can_frame f{};
  struct pollfd pfd;
  pfd.fd = canFD;
  pfd.events = (POLLERR | POLLHUP | POLLIN);
  pfd.revents = (POLLERR | POLLHUP | POLLIN);
  ::poll(&pfd, 1, pollTime);
  if (pfd.revents & POLLERR || pfd.revents & POLLHUP) {
    return Expected<can_frame, IOError>(IOError::FAILED_POLL);
  }
  if (pfd.revents & POLLIN) {
    size_t s = ::read(canFD, &f, sizeof(can_frame));
    if (s < sizeof(can_frame)) {
      return Expected<can_frame, IOError>(IOError::FAILED_READ);
    }
    return Expected<can_frame, IOError>(f);
  } else {
    return Expected<can_frame, IOError>(IOError::TIMEOUT);
  }
}

// TODO

std::shared_ptr<CanWriter> CanWriter::getCanWriter(const char *canIF) {

  for (size_t i{0}; i < NUMBER_OF_CANLINES; i++) {
    if (CanWriter::writers[i].name != nullptr &&
        std::strcmp(CanWriter::writers[i].name, canIF) == 0) {
      return CanWriter::writers[i].ptr;
    }
  }
  // Else we need to make a new one
  CanWriter *wNew = new CanWriter(canIF);
  bool f{false};
  size_t i;
  for (i = 0; i < NUMBER_OF_CANLINES; i++) {
    if (CanWriter::writers[i].name == nullptr) {
      WriterPair newPair{canIF, wNew, wNew->canFD};
      CanWriter::writers[i] = newPair;
      f = true;
      break;
    }
  }
  if (!f) {
    std::fprintf(stderr, "Was unable to allocate space for the canline please change compile flag "
                         "to allow for number of necessary canlines");
  }
  return CanWriter::writers[i].ptr;
}

void CanWriter::deleteWriter(const char *canIf) {
  bool found{false};
  int i;
  for (i = 0; i < NUMBER_OF_CANLINES; i++) {
    if (CanWriter::writers[i].name != nullptr &&
        std::strcmp(canIf, CanWriter::writers[i].name) == 0) {
      found = true;
      break;
    }
  }
  if (!found) {
    std::fprintf(stderr, "Failed to delete can writer, canwriter wasnt found in internal array\n");
  }
  // Grab resource for now
  auto ptr = CanWriter::writers[i].ptr;
  // Delete resource
  for (int j{i}; j < NUMBER_OF_CANLINES - 1; j++) {
    CanWriter::writers[j] = CanWriter::writers[j + 1];
  }
  CanWriter::writers[NUMBER_OF_CANLINES - 1] = WriterPair{};
  // Delete resource
  // If other pointers to it dangle resource will continue to exist but it will go out of scope when
  // the resource does
  ptr.reset();
}
