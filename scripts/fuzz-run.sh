#!/bin/sh

#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="$ROOT_DIR/cmake-build-fuzz"

FUZZER_NAME=${1:-fuzz_mqtt_codec}
CORPUS_DIR=${2:-"$BUILD_DIR/tests/fuzz_corpus/$FUZZER_NAME"}

cd "$BUILD_DIR"

BIN_PATH="tests/fuzz/$FUZZER_NAME"

if [ ! -x "$BIN_PATH" ]; then
  echo "[fuzz-run] Fuzzer binary not found or not executable: $BIN_PATH" >&2
  echo "           Make sure you have run scripts/fuzz-build.sh first." >&2
  exit 1
fi

mkdir -p "$CORPUS_DIR"

echo "[fuzz-run] Running $FUZZER_NAME with -max_total_time=30 and corpus: $CORPUS_DIR" >&2
"$BIN_PATH" -max_total_time=30 "$CORPUS_DIR"

echo "[fuzz-run] Running $FUZZER_NAME with -runs=100000 (no corpus)" >&2
"$BIN_PATH" -runs=100000

echo "[fuzz-run] Done." >&2
