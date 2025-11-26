#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
OUT_DIR="$ROOT_DIR/dist"
PKG_NAME="reactormq-unreal"
PKG_DIR="$OUT_DIR/$PKG_NAME"

rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR"

cp -R "$ROOT_DIR/unreal_engine/." "$PKG_DIR/"

NATIVE_DIR="$PKG_DIR/Source/ReactorMQ/Native"
mkdir -p "$NATIVE_DIR/src" "$NATIVE_DIR/include" "$NATIVE_DIR/fixtures"
cp -R "$ROOT_DIR/src/." "$NATIVE_DIR/src/"
cp -R "$ROOT_DIR/include/." "$NATIVE_DIR/include/"

if [ -f "$ROOT_DIR/tests/fixtures/docker_container.h" ]; then
    cp "$ROOT_DIR/tests/fixtures/docker_container.h" "$NATIVE_DIR/fixtures/"
fi
if [ -f "$ROOT_DIR/tests/fixtures/docker_container.cpp" ]; then
    cp "$ROOT_DIR/tests/fixtures/docker_container.cpp" "$NATIVE_DIR/fixtures/"
fi
if [ -f "$ROOT_DIR/tests/fixtures/port_utils.h" ]; then
    cp "$ROOT_DIR/tests/fixtures/port_utils.h" "$NATIVE_DIR/fixtures/"
fi

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"
tar -czf "$PKG_NAME.tar.gz" "$PKG_NAME"
cd "$ROOT_DIR"
rm -rf "$PKG_DIR"
