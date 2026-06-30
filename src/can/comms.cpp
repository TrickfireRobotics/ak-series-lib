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

  can_frame f{};
  std::memset(&f, 0, sizeof(can_frame));

  ::read(canFD, &f, sizeof(can_frame));
  return (Expected<can_frame, IOError>(f));
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

  size_t s = ::read(canFD, &f, sizeof(can_frame));
  if (s < sizeof(can_frame)) {
    return Expected<can_frame, IOError>(IOError::FAILED_READ);
  }
  return Expected<can_frame, IOError>(f);
}

// TODO

std::shared_ptr<CanWriter> CanWriter::getCanWriter(const char *canIF) {

  for (size_t i{0}; i < CanWriter::writers; i++) {
    if (CanWriter::avlblWriters[i].name != nullptr &&
        std::strcmp(CanWriter::avlblWriters[i].name, canIF) == 0) {
      return CanWriter::avlblWriters[i].ptr;
    }
  }
  // Else we need to make a new one
  CanWriter *wNew = new CanWriter(canIF);
  bool f{false};
  size_t i;
  for (i = 0; i < CanWriter::writers; i++) {
    if (CanWriter::avlblWriters[i].name == nullptr) {
      WriterPair newPair{canIF, wNew, wNew->canFD};
      CanWriter::avlblWriters[i] = newPair;
      break;
    }
  }

  // Resize array
  if (!f) {
    WriterPair *newPairs = new WriterPair[writers + 4];
    std::copy(avlblWriters, avlblWriters + CanWriter::writers, newPairs);
    delete[] avlblWriters;
    avlblWriters = newPairs;
    int ind = writers;
    avlblWriters[ind] = WriterPair{canIF, wNew, wNew->canFD};
    writers = writers + 4;
    return avlblWriters[ind].ptr;
  } else {
    return avlblWriters[i].ptr;
  }
}

// TODO

CanWriter::~CanWriter() {
  size_t i;
  bool found{false};
  for (i = 0; i < writers; i++) {
    if (avlblWriters[i].name != nullptr && std::strcmp(avlblWriters[i].name, canIF) == 0) {
      break;
    }
  }
  if (!found) {
    std::fprintf(
        stderr,
        "Failed to find the CanWriter object for %s while destroying, invalid constructor?\n",
        canIF);
    return;
  }

  for (int j{writers - 1}; j >= i + 1; j--) {
    avlblWriters[j - 1] = avlblWriters[j];
  }
  return;
}
