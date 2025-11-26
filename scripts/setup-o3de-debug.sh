#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
CODE_DIR="$ROOT_DIR/o3de/Code"

mkdir -p "$CODE_DIR"

ln -sfn "$ROOT_DIR/src" "$CODE_DIR/src"
ln -sfn "$ROOT_DIR/include" "$CODE_DIR/include"

# Create fixtures directory and link only the specific fixture files used by O3DE
mkdir -p "$CODE_DIR/fixtures"

[ -f "$ROOT_DIR/tests/fixtures/docker_container.h" ] && ln -sfn "$ROOT_DIR/tests/fixtures/docker_container.h" "$CODE_DIR/fixtures/docker_container.h"
[ -f "$ROOT_DIR/tests/fixtures/docker_container.cpp" ] && ln -sfn "$ROOT_DIR/tests/fixtures/docker_container.cpp" "$CODE_DIR/fixtures/docker_container.cpp"
[ -f "$ROOT_DIR/tests/fixtures/port_utils.h" ] && ln -sfn "$ROOT_DIR/tests/fixtures/port_utils.h" "$CODE_DIR/fixtures/port_utils.h"
