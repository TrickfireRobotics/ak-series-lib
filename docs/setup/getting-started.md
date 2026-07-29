---
title: Getting Started
description: Clone the AK Series driver repository and build the library.
---

Clone the repository, set up your build environment, then build the library with CMake.

## Clone the repository

```bash title="Terminal"
git clone https://github.com/TrickfireRobotics/ak-series.git
cd ak-series
```

:::caution
The build will fail if you are not on a Linux system or inside the dev container, since it depends on Linux SocketCAN.
:::

## Build the library

```bash title="Terminal"
mkdir build && cd build
cmake -S .. -B . -DSETUP_TEST_IFNAME=ON -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build .
```

| Option                          | Description                                 |
| ------------------------------- | ------------------------------------------- |
| `BUILD_TESTING`                 | Build the GTest suite                       |
| `SETUP_TEST_IFNAME`             | Configure a virtual CAN interface for tests |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | Emit `compile_commands.json` for your LSP   |

## Next steps

- Set up the [Dev Container](../dev-container/) for a reproducible CAN environment.
- Read the [protocol overview](../../guides/overview/) to understand Servo and MIT modes.
- Explore the [code reference](../../reference/overview/) for a deep dive into the library.

<!-- TODO: expand prerequisites and CAN interface setup details -->
