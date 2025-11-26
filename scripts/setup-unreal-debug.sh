#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
NATIVE_DIR="$ROOT_DIR/unreal_engine/Source/ReactorMQ/Native"

mkdir -p "$NATIVE_DIR"

ln -sfn "$ROOT_DIR/src" "$NATIVE_DIR/src"
ln -sfn "$ROOT_DIR/include" "$NATIVE_DIR/include"

# Create fixtures directory and link only the specific fixture files used by Unreal
mkdir -p "$NATIVE_DIR/fixtures"

[ -f "$ROOT_DIR/tests/fixtures/docker_container.h" ] && ln -sfn "$ROOT_DIR/tests/fixtures/docker_container.h" "$NATIVE_DIR/fixtures/docker_container.h"
[ -f "$ROOT_DIR/tests/fixtures/docker_container.cpp" ] && ln -sfn "$ROOT_DIR/tests/fixtures/docker_container.cpp" "$NATIVE_DIR/fixtures/docker_container.cpp"
[ -f "$ROOT_DIR/tests/fixtures/port_utils.h" ] && ln -sfn "$ROOT_DIR/tests/fixtures/port_utils.h" "$NATIVE_DIR/fixtures/port_utils.h"
