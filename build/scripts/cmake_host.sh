#!/usr/bin/env bash
# cmake_host.sh — Configure + build host static lib via CMake (Linux/macOS).
# AetherEngine-iOS · Clean-room. No Xcode required.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${ROOT}/build/out/cmake-host"
mkdir -p "${OUT}"
cmake -S "${ROOT}/build" -B "${OUT}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DAETHER_BUILD_IOS=OFF \
  -DAETHER_BUILD_TESTS=ON
cmake --build "${OUT}" -- -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
echo "Host CMake build OK → ${OUT}"
ls -la "${OUT}"/libaether_engine.a 2>/dev/null || find "${OUT}" -name 'libaether_engine.a'
