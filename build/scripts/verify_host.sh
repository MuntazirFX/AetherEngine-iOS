#!/usr/bin/env bash
# verify_host.sh — Host-side compile + smoke test for AetherEngine C sources.
# Runs on Linux/macOS without Xcode. AetherEngine-iOS · Clean-room.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}OK${NC}  $*"; }
fail() { echo -e "${RED}FAIL${NC} $*" >&2; exit 1; }
info() { echo -e "${YELLOW}==>${NC} $*"; }

OBJ_DIR="${ROOT}/build/out/host-obj"
LIB_DIR="${ROOT}/build/out/host-lib"
BIN_DIR="${ROOT}/build/out/host-bin"
rm -rf "${OBJ_DIR}"
mkdir -p "${OBJ_DIR}" "${LIB_DIR}" "${BIN_DIR}"

CC="${CC:-cc}"
CFLAGS=(
  -std=c11 -Wall -Wextra -Wno-unused-parameter
  -O0 -g
  -Iengine/core -Iengine/console -Iengine/command -Iengine/thread
  -Iengine/game -Iengine/game/weapons -Iengine/game/monsters
  -Iengine/game/dll -Iengine/game/ai -Iengine/physics
  -Iengine/save -Iengine/net -Iengine/input -Iengine/config
  -Iengine/fs -Iengine/audio -Iengine/render -Iengine/bsp
  -Iengine/player -Iengine/texture -Iengine/model -Iengine/entity
  -Iengine/client/hud -Iengine/client/menu -Iengine/vgui
)

info "1/5 Verify required paths"
REQUIRED=(
  engine/core/AetherCore.c
  engine/core/AetherEngine.c
  engine/game/AetherGameManager.c
  engine/game/AetherManifest.c
  engine/bsp/AetherBSP.c
  ios/AetherApp/EngineBridge.c
  ios/AetherApp/EngineBridge.h
  ios/AetherApp/AetherApp.swift
  ios/AetherApp/MetalRenderer.swift
  ios/AetherApp/AetherAudioIOS.swift
  build/CMakeLists.txt
  build/project.yml
  tests/host_smoke.c
)
for f in "${REQUIRED[@]}"; do
  [ -f "$f" ] || fail "missing $f"
  ok "$f"
done

info "2/5 Verify game manifests"
for m in valve bshift gearbox cstrike czero; do
  [ -f "engine/game/manifests/$m.json" ] || fail "missing manifest $m.json"
  ok "manifest $m.json"
done

info "3/5 Compile all engine C sources"
COUNT=0
FAIL=0
LIST_FILE="${OBJ_DIR}/sources.list"
find engine -name '*.c' | sort > "${LIST_FILE}"
while IFS= read -r f; do
  COUNT=$((COUNT + 1))
  base="$(echo "$f" | tr '/' '_')"
  out="${OBJ_DIR}/${base%.c}.o"
  if ! "${CC}" "${CFLAGS[@]}" -c "$f" -o "$out"; then
    echo "compile failed: $f" >&2
    FAIL=1
  fi
done < "${LIST_FILE}"
[ "$COUNT" -ge 80 ] || fail "expected >=80 engine .c files, found $COUNT"
ok "found $COUNT engine .c files"
[ "$FAIL" -eq 0 ] || fail "one or more C sources failed to compile"
ok "compiled $COUNT object files"

info "4/5 Archive static library"
# shellcheck disable=SC2086
ar rcs "${LIB_DIR}/libaether_engine.a" ${OBJ_DIR}/*.o
ok "libaether_engine.a"

info "5/5 Link and run host smoke test"
"${CC}" "${CFLAGS[@]}" \
  tests/host_smoke.c \
  "${LIB_DIR}/libaether_engine.a" \
  -lm \
  -o "${BIN_DIR}/host_smoke"
"${BIN_DIR}/host_smoke"
ok "host smoke passed"

info "All host verification checks passed."
