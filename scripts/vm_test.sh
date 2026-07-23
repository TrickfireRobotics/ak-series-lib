#!/usr/bin/env bash

ls -la | grep .git >/dev/null
if [ $? -ne 0 ]; then
  echo "Please run script from project root"
  exit 1
fi
multipass >/dev/null
if ! [ $? -ne 0 ]; then
  echo "Check binary exists in path before launching"
fi

# Testing should cleanup artifacts since we need to pull down a new folder each time
# Multipass executes default from home directory
#
multipass exec socketcan-dev -d /home/ubuntu -- sudo rm -rf socketcan-testing >/dev/null 2>&1

multipass exec socketcan-dev -- mkdir socketcan-testing
multipass transfer -r . socketcan-dev:/home/ubuntu/socketcan-testing/
multipass exec socketcan-dev -d /home/ubuntu/socketcan-testing/ -- sudo rm -rf build
multipass exec socketcan-dev -d /home/ubuntu/socketcan-testing/ -- cmake -S . -B build -DSETUP_TEST_IFNAME=ON -DBUILD_TESTING=ON
multipass exec socketcan-dev -d /home/ubuntu/socketcan-testing/build/ -- make
multipass exec socketcan-dev -d /home/ubuntu/socketcan-testing/build/ -- sudo ctest

if [[ $? != 0 ]]; then
  multipass exec socketcan-dev -- sudo ip link add dev vcan_test type vcan
  if [[ $? != 0 ]]; then
    multipass exec socketcan-dev -- sudo ip link delete vcan_test
    sudo ctest
  else
    multipass exec socketcan-dev -- sudo ip link delete vcan_test
    multipass exec socketcan-dev -d /home/ubuntu/socketcan-testing/build/Testing/Temporary -- cat LastTest.log
  fi
fi
