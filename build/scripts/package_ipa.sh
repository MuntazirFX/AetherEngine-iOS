#!/usr/bin/env bash
# package_ipa.sh
# Packages the built .app into an unsigned .ipa (Payload/ zip).
# Works with sideloading tools (AltStore, Sideloadly, TrollStore, etc.).
# AetherEngine-iOS · Clean-room.

set -euo pipefail

GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m"
log()  { echo -e "${GREEN}[ipa]${NC} $*"; }
fail() { echo -e "${RED}[ipa]${NC} $*" >&2; exit 1; }

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT_DIR="${ROOT}/build/out"
STAGE_DIR="${OUT_DIR}/ipa-stage"
IPA_PATH="${OUT_DIR}/AetherEngine.ipa"

# ---------- 1. Find built .app ----------
APP_PATH="$(find "${OUT_DIR}/DerivedData" -name 'AetherEngine.app' -type d | head -n1)"
[ -n "${APP_PATH}" ] || fail "AetherEngine.app not found — run build_ios.sh first"

log "App bundle: ${APP_PATH}"

# ---------- 2. Prepare staging dir ----------
rm -rf "${STAGE_DIR}" "${IPA_PATH}"
mkdir -p "${STAGE_DIR}/Payload"

# ---------- 3. Copy .app into Payload/ ----------
cp -R "${APP_PATH}" "${STAGE_DIR}/Payload/"

# ---------- 4. Strip any embedded signature (unsigned IPA) ----------
APP_IN_STAGE="${STAGE_DIR}/Payload/$(basename "${APP_PATH}")"
if [ -d "${APP_IN_STAGE}/_CodeSignature" ]; then
    rm -rf "${APP_IN_STAGE}/_CodeSignature"
fi
rm -f "${APP_IN_STAGE}/embedded.mobileprovision"

# ---------- 5. Zip it up ----------
log "Zipping Payload -> AetherEngine.ipa"
cd "${STAGE_DIR}"
zip -qry "${IPA_PATH}" Payload

# ---------- 6. Report ----------
IPA_SIZE=$(du -sh "${IPA_PATH}" | cut -f1)
log "IPA created: ${IPA_PATH} (${IPA_SIZE})"
log "Install via: AltStore / Sideloadly / TrollStore / ideviceinstaller"
log "Done ✔"
