#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMINIMATCH_BUILD_TESTS=ON
cmake --build build -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu)"
ctest --test-dir build --output-on-failure
