#!/usr/bin/env bash

#Prints the location of the script, which is why the `dirname -- "${BASH_SOURCE[0]}` is necessary
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
#Prints the location it was called from. above option is safer
# SCRIPT_DIR="$(cd -- . &>/dev/null && pwd)"
REPO_ROOT="${SCRIPT_DIR%%scripts}"

touch "${REPO_ROOT}/python/AKSeriesOut.cpp"
touch "${REPO_ROOT}/_stubs/ak_series/__init__.pyi"
