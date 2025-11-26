#!/bin/bash
#
# // SPDX-License-Identifier: MPL-2.0
# // Copyright 2025 Simon Balarabe
# // Project: ReactorMQ — https://github.com/Naragato/reactormq
#

set -e

echo "Building with ThreadSanitizer enabled..."
mkdir -p build_tsan
cd build_tsan

cmake -DENABLE_TSAN=ON -DCMAKE_BUILD_TYPE=Debug ..

cmake --build . -j $(nproc)

echo "Running Unit Tests..."
./tests/unit_tests

echo "Running Integration Tests..."
./tests/integration_tests

echo "TSan test run complete."
