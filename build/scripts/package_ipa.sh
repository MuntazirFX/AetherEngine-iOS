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

# ---------- Artifact notes (what appears after a successful run) ----------
# On disk under build/out/:
#   AetherEngine.ipa              — unsigned IPA zip (Payload/ layout)
#   ipa-stage/Payload/            — staging dir used while zipping
#   ipa-stage/Payload/AetherEngine.app/  — copied .app bundle contents:
#       Info.plist, AetherEngine (arm64 Mach-O), Assets.car / storyboards,
#       Frameworks/ (if any), PkgInfo, _CodeSignature stripped, no mobileprovision
#   DerivedData/.../AetherEngine.app — produced earlier by build_ios.sh (input)
# Sideload tools expect the .ipa; the staging Payload/ tree is intermediate.

# ---------- GitHub Actions artifact upload notes ----------
# When build-arm64.yml runs on workflow_dispatch (macos-14):
#   1. Job build_ipa packages this IPA via package_ipa.sh
#   2. Step "Upload IPA artifact" uses actions/upload-artifact@v4
#      name: AetherEngine-<version>
#      path: build/out/AetherEngine.ipa
#      retention-days: 30
#   3. Download from the Actions run page → Artifacts → AetherEngine-<version>
#      or: gh run download <run-id> -n AetherEngine-<version>
#   4. Optional: publish_release=true also attaches the IPA to a GitHub Release
# This Linux/CI host does not build the IPA; the artifact steps above are macOS-only.
