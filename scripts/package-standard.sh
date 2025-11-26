#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
OUT_DIR="$ROOT_DIR/dist"
PKG_NAME="reactormq-dev"
PKG_DIR="$OUT_DIR/$PKG_NAME"

rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR"

cp -R "$ROOT_DIR/cmake" "$PKG_DIR/"
cp -R "$ROOT_DIR/tests" "$PKG_DIR/"
cp -R "$ROOT_DIR/src" "$PKG_DIR/"
cp -R "$ROOT_DIR/include" "$PKG_DIR/"
cp "$ROOT_DIR/CMakeLists.txt" "$PKG_DIR/"
if [ -f "$ROOT_DIR/README.md" ]; then
    cp "$ROOT_DIR/README.md" "$PKG_DIR/"
fi

FIX_DIR="$PKG_DIR/tests/fixtures"
if [ -d "$FIX_DIR" ]; then
    find "$FIX_DIR" -type f \
        ! -name 'docker_container.cpp' \
        ! -name 'docker_container.h' \
        ! -name 'port_utils.h' \
        -exec rm -f {} +
fi

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"
tar -czf "$PKG_NAME.tar.gz" "$PKG_NAME"
cd "$ROOT_DIR"
rm -rf "$PKG_DIR"
