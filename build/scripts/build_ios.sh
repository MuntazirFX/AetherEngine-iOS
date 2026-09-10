#!/usr/bin/env bash
# build_ios.sh
# Builds the AetherEngine iOS app (unsigned .app bundle).
# AetherEngine-iOS · Clean-room.

set -euo pipefail

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
NC="\033[0m"

log()  { echo -e "${GREEN}[build]${NC} $*"; }
warn() { echo -e "${YELLOW}[warn]${NC} $*"; }
fail() { echo -e "${RED}[fail]${NC} $*" >&2; exit 1; }

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${ROOT}/build"
OUT_DIR="${ROOT}/build/out"

log "Repository root : ${ROOT}"
log "Build directory : ${BUILD_DIR}"
log "Output directory: ${OUT_DIR}"

mkdir -p "${OUT_DIR}"

command -v cmake >/dev/null 2>&1 || fail "cmake not found"
command -v xcodebuild >/dev/null 2>&1 || fail "xcodebuild not found"
if ! command -v xcodegen >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
        brew install xcodegen
    else
        fail "xcodegen missing and Homebrew unavailable"
    fi
fi

# ---------- 1. Get iOS SDK Path ----------
log "Fetching iOS SDK path..."
IOS_SDK_PATH="$(xcrun --sdk iphoneos --show-sdk-path)"
[ -d "$IOS_SDK_PATH" ] || fail "iPhoneOS SDK not found"
log "iOS SDK: $IOS_SDK_PATH"

# ---------- 2. Build C engine static library (via CMake) ----------
log "Configuring CMake for iOS ARM64..."
cmake -S "${BUILD_DIR}" -B "${OUT_DIR}/cmake-ios" \
    -DCMAKE_BUILD_TYPE=Release \
    -DAETHER_BUILD_IOS=ON \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_SYSROOT="${IOS_SDK_PATH}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0

log "Building engine static library..."
cmake --build "${OUT_DIR}/cmake-ios" --config Release -- -j"$(sysctl -n hw.ncpu)"

LIB_PATH="$(find "${OUT_DIR}/cmake-ios" -name 'libaether_engine.a' | head -n1)"
[ -n "${LIB_PATH}" ] || fail "libaether_engine.a not found"
log "Engine library: ${LIB_PATH}"

# Copy the lib to a known location so project.yml can find it
cp "${LIB_PATH}" "${OUT_DIR}/cmake-ios/libaether_engine.a"
log "Copied library to: ${OUT_DIR}/cmake-ios/libaether_engine.a"

# ---------- 3. Generate Xcode project ----------
log "Running xcodegen..."
cd "${BUILD_DIR}"
rm -rf AetherApp.xcodeproj
xcodegen generate --spec project.yml --project .

# ---------- 3b. Force Xcode 15 compatible format ----------
log "Patching project format for Xcode 15 compatibility..."
PBXPROJ_FILE="AetherApp.xcodeproj/project.pbxproj"
if [ -f "${PBXPROJ_FILE}" ]; then
    sed -i '' -E 's/objectVersion = [0-9]+;/objectVersion = 56;/g' "${PBXPROJ_FILE}"
fi

# ---------- 4. Build the iOS app ----------
log "Building iOS app (unsigned)..."
xcodebuild \
    -project "AetherApp.xcodeproj" \
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

APP_SIZE=$(du -sh "${APP_PATH}" | cut -f1)
log "App bundle size: ${APP_SIZE}"
log "Build complete ✔"
