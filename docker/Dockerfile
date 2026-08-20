FROM ros:jazzy-ros-base AS runtime

ARG DEBIAN_FRONTEND=noninteractive
ARG TZ="America/Los_Angeles"

# --------------------------------------
# Locale + timezone

RUN apt-get update && \
    apt-get install --no-install-recommends -y locales && \
    locale-gen en_US en_US.UTF-8 && \
    update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 && \
    rm -rf /var/lib/apt/lists/*

ENV LANG=en_US.UTF-8

RUN ln -snf "/usr/share/zoneinfo/${TZ}" /etc/localtime && \
    echo "${TZ}" > /etc/timezone

# --------------------------------------
# Build + test toolchain (SocketCAN, GTest, vcan tooling)

RUN apt-get update && \
    apt-get install --no-install-recommends -y \
    python3 \
    python3-pip \
    python3-venv \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libsocketcan-dev \
    libgtest-dev \
    libgmock-dev \
    iproute2 \
    kmod \
    can-utils \
    sudo \
    curl \
    ca-certificates \
    clangd \
    npm \
    clang-format \
    git && \
    rm -rf /var/lib/apt/lists/*

RUN npm install -g bash-language-server

# --------------------------------------
# User

RUN (userdel -r ubuntu 2>/dev/null || true) && \
    (groupdel ubuntu 2>/dev/null || true) && \
    groupadd --gid ${USER_GID} trickfire && \
    useradd trickfire --uid 1000 --gid 1000 --shell /bin/bash --create-home --no-log-init && \
    usermod -aG video trickfire
COPY docker/container.bashrc /home/trickfire/.bashrc
RUN echo "trickfire ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/user

USER trickfire

# --------------------------------------------------------------------------------------------
# TEST - one-shot image that configures vcan and runs ctest

FROM runtime AS test

USER trickfire
WORKDIR /home/trickfire/ak-series

CMD ["bash", "-lc", "\
    sudo modprobe vcan && \
    cmake -S . -B build -G Ninja \
    -DBUILD_TESTING=ON \
    -DSETUP_TEST_IFNAME=ON && \
    cmake --build build && \
    ctest --test-dir build --output-on-failure \
    "]

# --------------------------------------------------------------------------------------------
# DEV - adds dev tooling on top of runtime  (devcontainer)

FROM runtime AS dev

USER root

RUN pip3 install ruff --break-system-packages

RUN curl -sSL "https://github.com/mvdan/sh/releases/latest/download/shfmt_$(curl -sSL https://api.github.com/repos/mvdan/sh/releases/latest | grep -oP '(?<="tag_name": ")v[^"]+')_linux_amd64" \
    -o /usr/local/bin/shfmt && \
    chmod +x /usr/local/bin/shfmt

USER trickfire
