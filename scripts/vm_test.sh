#!/usr/bin/env bash

set -euo pipefail

VM="socketcan-dev"
REMOTE="/home/ubuntu/socketcan-testing"
DEPS_FILE="scripts/deps.txt"

if [ ! -d .git ]; then
  echo "Please run script from project root" >&2
  exit 1
fi

if ! command -v multipass >/dev/null 2>&1; then
  echo "multipass not found in PATH" >&2
  exit 1
fi

if [ ! -f "$DEPS_FILE" ]; then
  echo "Missing $DEPS_FILE" >&2
  exit 1
fi

PKGS=()
while IFS= read -r line; do
  case "$line" in
  '' | \#*) continue ;;
  esac
  PKGS+=("$line")
done <"$DEPS_FILE"

if [ ${#PKGS[@]} -eq 0 ]; then
  echo "No packages listed in $DEPS_FILE" >&2
  exit 1
fi

vm_count="$(multipass list --format csv | tail -n +2 | cut -d, -f1 | grep -cx "$VM" || true)"
if [ "$vm_count" -eq 0 ]; then
  multipass launch 24.04 --name "$VM"
fi

if ! multipass exec "$VM" -- sh -c 'dpkg -s "$@" >/dev/null 2>&1' _ "${PKGS[@]}"; then
  echo "Provisioning $VM..."
  multipass exec "$VM" -- sudo apt-get update
  multipass exec "$VM" -- sudo apt-get install -y --no-install-recommends "${PKGS[@]}"
  KERNEL_TYPE="$(multipass exec "$VM" -- uname -r)"
  echo "$KERNEL_TYPE"
  multipass exec "$VM" -- sudo apt-get install -y --no-install-recommends linux-modules-extra-"$KERNEL_TYPE"

fi

multipass exec "$VM" -d /home/ubuntu -- sudo rm -rf socketcan-testing
multipass exec "$VM" -- mkdir -p socketcan-testing
multipass transfer -r . "$VM:$REMOTE/"
multipass exec "$VM" -d "$REMOTE" -- sudo rm -rf build

multipass exec "$VM" -d "$REMOTE" -- cmake -S . -B build -DSETUP_TEST_IFNAME=ON -DBUILD_TESTING=ON
multipass exec "$VM" -d "$REMOTE/build" -- make

if ! multipass exec "$VM" -d "$REMOTE/build" -- sudo ctest --output-on-failure; then
  multipass exec "$VM" -d "$REMOTE/build/Testing/Temporary" -- cat LastTest.log || true
  exit 1
fi
