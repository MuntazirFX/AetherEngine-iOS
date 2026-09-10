#!/usr/bin/env bash
# build_ios.sh
# Builds the AetherEngine iOS app (unsigned .app bundle + libaether_engine.a).
# Intended to run on macOS (or GitHub Actions macOS runner).
# AetherEngine-iOS · Clean-room.

set -euo pipefail

# ---------- Colors ----------
GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
NC="\033[0m"

log()  { echo -e "${GREEN}[build]${NC} $*"; }
warn() { echo -e "${YELLOW}[warn]${NC} $*"; }
fail() { echo -e "${RED}[fail]${NC} $*" >&2; exit 1; }

# ---------- Paths ----------
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${ROOT}/build"
OUT_DIR="${ROOT}/build/out"
ENGINE_DIR="${ROOT}/engine"

log "Repository root : ${ROOT}"
log "Build directory : ${BUILD_DIR}"
log "Output directory: ${OUT_DIR}"

mkdir -p "${OUT_DIR}"

# ---------- 1. Ensure tools ----------
command -v cmake >/dev/null 2>&1 || fail "cmake not found (brew install cmake)"
command -v xcodebuild >/dev/null 2>&1 || fail "xcodebuild not found (Xcode required)"
command -v xcodegen >/dev/null 2>&1 || warn "xcodegen not found; will try to install via brew"

if ! command -v xcodegen >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
        log "Installing xcodegen via Homebrew..."
        brew install xcodegen
    else
        fail "xcodegen missing and Homebrew unavailable"
    fi
fi

# ---------- 2. Build C engine static library ----------
log "Configuring CMake for iOS ARM64..."
cmake -S "${BUILD_DIR}" -B "${OUT_DIR}/cmake-ios" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAETHER_BUILD_IOS=ON \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_SYSROOT=iphoneos \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0

log "Building engine static library..."
cmake --build "${OUT_DIR}/cmake-ios" --config Release -- -j"$(sysctl -n hw.ncpu)"

LIB_PATH="$(find "${OUT_DIR}/cmake-ios" -name 'libaether_engine.a' | head -n1)"
[ -n "${LIB_PATH}" ] || fail "libaether_engine.a not found"
log "Engine library: ${LIB_PATH}"

# ---------- 3. Generate Xcode project ----------
log "Running xcodegen..."
cd "${BUILD_DIR}"
xcodegen generate --spec project.yml --project "${OUT_DIR}/AetherApp.xcodeproj"

# ---------- 4. Build the iOS app ----------
log "Building iOS app (unsigned)..."
xcodebuild \
    -project "${OUT_DIR}/AetherApp.xcodeproj" \
    -scheme AetherApp \
    -configuration Release \
    -sdk iphoneos \
    -arch arm64 \
    -derivedDataPath "${OUT_DIR}/DerivedData" \
    CODE_SIGNING_ALLOWED=NO \
    CODE_SIGN_IDENTITY="" \
    CODE_SIGNING_REQUIRED=NO \
    build

APP_PATH="$(find "${OUT_DIR}/DerivedData" -name 'AetherEngine.app' -type d | head -n1)"
[ -n "${APP_PATH}" ] || fail "AetherEngine.app not found"
log "iOS app bundle: ${APP_PATH}"

# ---------- 5. Report ----------
APP_SIZE=$(du -sh "${APP_PATH}" | cut -f1)
log "App bundle size: ${APP_SIZE}"
log "Build complete ✔"
