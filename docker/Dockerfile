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

# --------------------------------------------------------------------------------------------
# Build + test toolchain (SocketCAN, GTest, vcan tooling)
# The build/test package set is shared with scripts/vm_test.sh via
# scripts/deps.txt so the container and the Multipass VM cannot drift apart.
# Editing deps.txt intentionally invalidates this layer.
# --------------------------------------------------------------------------------------------
COPY scripts/deps.txt /tmp/deps.txt


# --------------------------------------------------------------------------------------------
# DEV - adds dev tooling on top of runtime  (devcontainer)
# --------------------------------------------------------------------------------------------
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        $(grep -vE '^[[:space:]]*(#|$)' /tmp/deps.txt) \
        python3 \
        python3-pip \
        python3-venv \
        sudo \
        curl \
        ca-certificates \
        clangd \
        npm \
        clang-format \
        python3-pybind11 \
        shellcheck \
        git && \
    rm -rf /var/lib/apt/lists/*


# --------------------------------------------------------------------------------------------
# User
# --------------------------------------------------------------------------------------------
RUN useradd trickfire --shell /bin/bash --create-home --no-log-init && \
    usermod -aG video trickfire
COPY docker/container.bashrc /home/trickfire/.bashrc
RUN echo "trickfire ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/user

USER trickfire

# --------------------------------------------------------------------------------------------
# DEV - adds dev tooling on top of runtime  (devcontainer)
# --------------------------------------------------------------------------------------------

FROM runtime AS dev

USER root

RUN pip3 install ruff --break-system-packages

RUN curl -sSL "https://github.com/mvdan/sh/releases/latest/download/shfmt_$(curl -sSL https://api.github.com/repos/mvdan/sh/releases/latest | grep -oP '(?<="tag_name": ")v[^"]+')_linux_amd64" \
    -o /usr/local/bin/shfmt && \
    chmod +x /usr/local/bin/shfmt

# --------------------------------------------------------------------------------------------
# Litgen dependencies
# --------------------------------------------------------------------------------------------
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        libxml2-dev \
        libxslt1-dev \
        default-jdk-headless && \
    rm -rf /var/lib/apt/lists/*
RUN pip3 install litgen --break-system-packages

USER trickfire
