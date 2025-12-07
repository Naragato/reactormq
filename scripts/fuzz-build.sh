#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="$ROOT_DIR/cmake-build-fuzz"

C_COMPILER="${1:-}"
CXX_COMPILER="${2:-}"

C_COMPILER="${C_COMPILER#-DCMAKE_C_COMPILER=}"
CXX_COMPILER="${CXX_COMPILER#-DCMAKE_CXX_COMPILER=}"

CMAKE_ARGS="-DREACTORMQ_BUILD_TESTS=ON -DREACTORMQ_BUILD_FUZZERS=ON -DENABLE_ASAN=ON -DENABLE_UBSAN=ON"

if [ -n "$C_COMPILER" ]; then
  CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_C_COMPILER=$C_COMPILER"
  echo "[fuzz-build] Using C compiler: $C_COMPILER" >&2
fi

if [ -n "$CXX_COMPILER" ]; then
  CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_COMPILER=$CXX_COMPILER"
  echo "[fuzz-build] Using C++ compiler: $CXX_COMPILER" >&2
fi

echo "[fuzz-build] Configuring fuzz targets in: $BUILD_DIR" >&2

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" $CMAKE_ARGS

echo "[fuzz-build] Building fuzz targets..." >&2

cmake --build "$BUILD_DIR" --target \
  fuzz_mqtt_codec fuzz_fixed_header fuzz_variable_byte_integer \
  fuzz_publish_packet fuzz_connect_packet

echo "[fuzz-build] Done." >&2
