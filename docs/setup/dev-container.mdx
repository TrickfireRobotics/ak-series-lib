---
title: Dev Container
description: Develop inside the Docker dev container with can-utils preinstalled.
---

The main development workflow runs inside a Docker dev container that ships the `can-utils`
tooling and the toolchain required to build and test the library.

## Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) installed and running
- [VS Code](https://code.visualstudio.com/) with the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension

## Open the container

Make sure that you have the Devconatiner plugin already installed on your IDE of choice, we mainly use VSCode here, so this guide will be tailored towards those users.

When you open VSCode, you should see a popup in the bottom that says

```text title="VSCode Popup"
Folder contains a Dev Container configuration file.
Reopen folder to develop in container.
```

Click `Reopen in Container` and VSCode should automatically start building the container.

Building the container usually takes a 5-15 minutes depending on the machine but once you build the Devcontainer the firs time around.

If you use an IDE that relies on a `compile_commands` file, the container automatically creates a `compile_commands.json` file for linux architecture inside the dev container. Use that file for the LSP.

## Virtual CAN interface

In our testing we are going to be creating and destroying a virtual can line `vcan_test` this is going to be used for testing our messages actually being sent down the can line. The way you setup the canline is by running the following.

```bash title="Terminal"
sudo modprobe vcan
sudo ip link add dev vcan_test type vcan
sudo ip link set up vcan_test
```

If you don't run these commands ahead of time before trying to send anything down the can line, you will error out.
