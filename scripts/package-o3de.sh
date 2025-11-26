#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
OUT_DIR="$ROOT_DIR/dist"
PKG_NAME="reactormq-o3de"
PKG_DIR="$OUT_DIR/$PKG_NAME"

rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/Code/Source" "$PKG_DIR/Code/Include"

cp -R "$ROOT_DIR/src/." "$PKG_DIR/Code/Source/"
cp -R "$ROOT_DIR/include/." "$PKG_DIR/Code/Include/"

mkdir -p "$OUT_DIR"
cd "$OUT_DIR"
tar -czf "$PKG_NAME.tar.gz" "$PKG_NAME"
cd "$ROOT_DIR"
rm -rf "$PKG_DIR"
