#ifndef BUILD_TESTING
#define BUILD_TESTING
#endif
#include "can/comms.hpp"
#include "../TestInclude.hpp"

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
#include <cstring>
#include <gtest/gtest.h>
#include <stdexcept>

using std::string;

void CanInterfaceTests::SetUp() {
  int s = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);

  if (s < 0) {
    throw std::runtime_error("Couldn't open socket, please check that you enabled the canline");
  }
  canIfName = std::getenv("SETUP_TEST_IFNAME");
  struct ifreq ifr;
  struct sockaddr_can addr;
  std::strcpy(ifr.ifr_ifrn.ifrn_name, canIfName.c_str());
  ::ioctl(s, SIOCGIFINDEX, &ifr);

  std::memset(&addr, 0, sizeof(struct sockaddr_can));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  // C style cast same as in comms source code
  if (::bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    std::fprintf(stderr, "CAN IF RECEIVED: %s Error code: %d, Message %s \n", canIfName.c_str(),
                 errno, strerror(errno));

    throw std::runtime_error("Failed to bind to socket");
  }
  // Finally give the sock fd
  socketFD = s;
}

// We arent managing memory it just needs to exist
// to compile
void TearDown() {}

TEST_F(CanInterfaceTests, CanInterfaceTesting) {
  // TODO check can functionality tests
  using AKSeries::CanInterface;
  auto writer = CanInterface(canIfName.c_str());
  can_frame f = static_cast<can_frame>(ServoSendFrame::setCurrentBrake(11, 10.0));
  can_frame cmp{};

  CanIOError checkSuccess = writer.send(f);
  ASSERT_TRUE(checkSuccess == CanIOError::NONE);

  ::read(socketFD, &cmp, sizeof(can_frame));
  EXPECT_TRUE(std::memcmp(&f, &cmp, sizeof(can_frame)) == 0) << "Can frames failed";

  ::write(socketFD, &f, sizeof(can_frame));
  auto recv = writer.sendAndRead(cmp);
  ASSERT_TRUE(recv.successful);
  ASSERT_TRUE(std::memcmp(&recv.value, &cmp, sizeof(can_frame)) == 0);

  recv = writer.sendAndRead(f);
  ASSERT_TRUE(!recv.successful);
  ASSERT_TRUE(recv.error == CanIOError::TIMEOUT);

  ::write(socketFD, &f, sizeof(can_frame));
  recv = writer.read();
  ASSERT_TRUE(std::memcmp(&recv.value, &f, sizeof(can_frame)) == 0);

  recv = writer.read();
  ASSERT_TRUE(!recv.successful);
  ASSERT_TRUE(recv.error == CanIOError::TIMEOUT);
}

TEST_F(CanInterfaceTests, NonBlockingTesting) {
  using AKSeries::CanInterface;
  auto writer = CanInterface(canIfName.c_str());
  can_frame f = static_cast<can_frame>(ServoSendFrame::setCurrentBrake(11, 10.0));
  writer.setPollTimeout(0);
  can_frame cmp{};

  CanIOError checkSuccess = writer.send(f);
  ASSERT_TRUE(checkSuccess == CanIOError::NONE);

  ::read(socketFD, &cmp, sizeof(can_frame));
  EXPECT_TRUE(std::memcmp(&f, &cmp, sizeof(can_frame)) == 0) << "Can frames failed";

  ::write(socketFD, &f, sizeof(can_frame));
  auto recv = writer.sendAndRead(cmp);
  ASSERT_TRUE(recv.successful);
  ASSERT_TRUE(std::memcmp(&recv.value, &cmp, sizeof(can_frame)) == 0);

  recv = writer.sendAndRead(f);
  ASSERT_TRUE(!recv.successful);
  ASSERT_TRUE(recv.error == CanIOError::TIMEOUT);

  ::write(socketFD, &f, sizeof(can_frame));
  recv = writer.read();
  ASSERT_TRUE(std::memcmp(&recv.value, &f, sizeof(can_frame)) == 0);

  recv = writer.read();
  ASSERT_TRUE(!recv.successful);
  ASSERT_TRUE(recv.error == CanIOError::TIMEOUT);
}
