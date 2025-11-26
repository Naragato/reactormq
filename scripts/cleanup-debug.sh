#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
UNREAL_NATIVE="$ROOT_DIR/unreal_engine/Source/ReactorMQ/Native"
O3DE_CODE="$ROOT_DIR/o3de/Code"

rm -rf "$UNREAL_NATIVE/src" "$UNREAL_NATIVE/include" "$UNREAL_NATIVE/fixtures"
rm -rf "$O3DE_CODE/src" "$O3DE_CODE/include"
