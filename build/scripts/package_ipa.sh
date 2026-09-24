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

# ---------- Flags (unsigned IPA dry-run polish) ----------
#   --dry-run / -n     Print plan; do not require .app / do not zip (Linux-safe)
#   --notes-only       Write DRY_RUN_NOTES.txt checklist then exit 0
#   --help / -h        Show usage
#   --sign-check       Note that this path is unsigned (no codesign invoked)
DRY_RUN=0
NOTES_ONLY=0
SIGN_CHECK=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dry-run|-n) DRY_RUN=1; shift ;;
    --notes-only) NOTES_ONLY=1; shift ;;
    --sign-check) SIGN_CHECK=1; shift ;;
    --help|-h)
      cat <<'USAGE'
package_ipa.sh — unsigned IPA packager (macOS) + dry-run notes (any host)

Usage:
  ./build/scripts/package_ipa.sh              # package DerivedData .app → .ipa (macOS)
  ./build/scripts/package_ipa.sh --dry-run    # print plan; no zip (Linux/CI safe)
  ./build/scripts/package_ipa.sh --notes-only # write build/out/DRY_RUN_NOTES.txt
  ./build/scripts/package_ipa.sh --sign-check # confirm unsigned path (no codesign)

Flags may combine: --dry-run --notes-only --sign-check
USAGE
      exit 0
      ;;
    *) fail "unknown flag: $1 (try --help)" ;;
  esac
done

write_dry_run_notes() {
  mkdir -p "${OUT_DIR}"
  NOTES="${OUT_DIR}/DRY_RUN_NOTES.txt"
  {
    echo "AetherEngine-iOS unsigned IPA dry-run notes"
    echo "host=$(uname -s 2>/dev/null || echo unknown)"
    echo "script=package_ipa.sh"
    echo "unsigned=1"
    echo "codesign=never (Payload zip only; strip _CodeSignature)"
    echo "requires=macos+xcode for real IPA"
    echo "workflow=build-arm64.yml workflow_dispatch"
    echo "artifact=upload-artifact@v4 retention=30 compression-level=9"
    echo "local_macos=./build/scripts/build_ios.sh && ./build/scripts/package_ipa.sh"
    echo "dry_run_flag=--dry-run"
    echo "notes_only_flag=--notes-only"
    echo "sign_check_flag=--sign-check"
    echo "sideload=AltStore|Sideloadly|TrollStore|ideviceinstaller"
    echo "DEVICE_SIDELOAD_CHECKLIST=AppleConfigurator|XcodeDevices|ideviceinstaller|AltStore"
    echo "device_sideload_a=Apple Configurator 2 → Add Apps → IPA"
    echo "device_sideload_b=Xcode Devices and Simulators → Installed Apps +"
    echo "device_sideload_c=ideviceinstaller -i build/out/AetherEngine.ipa"
    echo "dispatch_dry_run=gh workflow run 'Build AetherEngine IPA' -f version=v0.0.0-dry -f publish_release=false -f dry_run_validate=true"
    echo "dispatch_input=dry_run_validate"
    echo "workflow_file=.github/workflows/build-arm64.yml"
  } > "${NOTES}"
  log "Wrote ${NOTES}"
}

if [[ "${NOTES_ONLY}" -eq 1 ]]; then
  write_dry_run_notes
  log "notes-only complete (no IPA built on this host)"
  exit 0
fi

if [[ "${DRY_RUN}" -eq 1 ]]; then
  log "DRY-RUN: unsigned IPA package plan (no .app required)"
  log "  1. build_ios.sh → build/out/DerivedData/.../AetherEngine.app"
  log "  2. package_ipa.sh → Payload/ zip → ${IPA_PATH}"
  log "  3. strip _CodeSignature + embedded.mobileprovision"
  log "  4. upload-artifact@v4 (Actions macos-14) or sideload locally"
  if [[ "${SIGN_CHECK}" -eq 1 ]]; then
    log "  sign-check: codesign NOT invoked; IPA remains unsigned"
  fi
  write_dry_run_notes
  log "DRY-RUN done ✔ (IPA unbuilt here — expected on Linux/CI)"
  exit 0
fi

if [[ "${SIGN_CHECK}" -eq 1 ]]; then
  log "sign-check: this script never calls codesign; IPA is unsigned Payload zip"
fi

# ---------- 1. Find built .app ----------
APP_PATH="$(find "${OUT_DIR}/DerivedData" -name 'AetherEngine.app' -type d | head -n1)"
[ -n "${APP_PATH}" ] || fail "AetherEngine.app not found — run build_ios.sh first (or use --dry-run)"

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

# Write companion artifact notes for Actions upload (batch16)
if [[ -f "${IPA_PATH}" ]]; then
  NOTES="${OUT_DIR}/ARTIFACT_NOTES.txt"
  {
    echo "AetherEngine-iOS unsigned IPA artifact"
    echo "ipa=$(basename "${IPA_PATH}")"
    echo "size=${IPA_SIZE}"
    echo "sha256=$(shasum -a 256 "${IPA_PATH}" | awk '{print $1}')"
    echo "retention_days_default=30"
    echo "upload=actions/upload-artifact@v4"
    echo "compression_level=9"
    echo "built_by=package_ipa.sh"
  } > "${NOTES}"
  log "Wrote ${NOTES}"
fi

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

# ---------- macOS Actions runner dry-run notes (further) ----------
# Runner: macos-14 (see .github/workflows/build-arm64.yml build_ipa job)
# Xcode: image default (xcode-select -p); pin via DeMille/setup-xcode if needed
# Dry-run without signing:
#   1. workflow_dispatch → build_ipa
#   2. build_ios.sh → DerivedData/.../AetherEngine.app (arm64)
#   3. package_ipa.sh → unsigned Payload zip (this script)
#   4. upload-artifact@v4 → AetherEngine-<version> (30d)
#   5. Optional softprops/action-gh-release when publish_release=true
# Local macOS dry-run (no Actions):
#   ./build/scripts/build_ios.sh && ./build/scripts/package_ipa.sh
#   ls -lh build/out/AetherEngine.ipa
# Linux/CI host: cannot produce IPA; verify_host.sh remains the gate here.
# Sideload: AltStore / Sideloadly / TrollStore / ideviceinstaller — no Apple ID
# signing baked into this unsigned IPA path.


# ==========================================================================
# UNSIGNED IPA DRY-RUN (Actions macos-14) — clear checklist
# ==========================================================================
# This Linux/CI host NEVER builds the IPA. Use GitHub Actions macos-14:
#
# A) From GitHub UI
#    1. Actions → "Build AetherEngine IPA" → Run workflow
#    2. Inputs: version (e.g. v0.1.15), publish_release=false for dry-run
#    3. Wait for job "Unsigned IPA (dispatch only)" on runner macos-14
#    4. Download artifact AetherEngine-<version> (retention 30 days)
#
# B) From CLI (gh)
#    gh workflow run build-arm64.yml -f version=v0.1.15 -f publish_release=false
#    gh run list --workflow=build-arm64.yml --limit 3
#    gh run download <run-id> -n AetherEngine-v0.1.15
#
# C) Local macOS (same scripts Actions uses)
#    ./build/scripts/build_ios.sh && ./build/scripts/package_ipa.sh
#    test -f build/out/AetherEngine.ipa && ls -lh build/out/AetherEngine.ipa
#
# D) Sideload unsigned Payload zip
#    AltStore / Sideloadly / TrollStore / ideviceinstaller
#    No Apple Developer signing is baked into this path.
#
# PR/push events only run verify_host on macos-14 (no IPA). IPA is
# workflow_dispatch-only — see .github/workflows/build-arm64.yml.


# ---------- IPA artifact automation (batch16) ----------
# Actions upload improvements (see .github/workflows/build-arm64.yml):
#   - actions/upload-artifact@v4
#   - retention-days: 30 (override via workflow input artifact_retention_days)
#   - compression-level: 9 (smaller artifact upload)
#   - if-no-files-found: error
#   - Companion ARTIFACT_NOTES.txt uploaded beside the IPA (sha256, size, commit, retention)
# Download:
#   gh run download <run-id> -n AetherEngine-<version>
#   # contains AetherEngine.ipa + ARTIFACT_NOTES.txt
# Retention note: GitHub free/pro default artifact retention is 90d max; we pin 30d
# unless workflow_dispatch input raises it (1..90). Expired artifacts are not recoverable.




# ---------- Unsigned IPA dry-run polish (batch17) ----------
# Flags (work on Linux/CI without Xcode):
#   --dry-run / -n     Print packaging plan; write DRY_RUN_NOTES.txt; exit 0
#   --notes-only       Only write DRY_RUN_NOTES.txt
#   --sign-check       Explicitly confirm no codesign is invoked
#   --help / -h        Usage
# Real IPA still requires macOS + Xcode (build_ios.sh) then this script without --dry-run.
# Example Linux gate:
#   bash build/scripts/package_ipa.sh --dry-run --sign-check
#   test -f build/out/DRY_RUN_NOTES.txt


# ---------- Device IPA sideload checklist (batch19) ----------
# After you have build/out/AetherEngine.ipa (macOS/Xcode or Actions artifact):
#
# A) Apple Configurator 2 (macOS App Store)
#    1. Connect iPhone/iPad via USB; trust the computer
#    2. Select the device → Add → Apps → choose AetherEngine.ipa
#    3. Or: drag-drop the IPA onto the device in Configurator
#    4. On device: Settings → General → VPN & Device Management → trust developer
#    Note: unsigned IPAs may need a free/paid Apple ID resign via Configurator
#          "Prepare" / pairing, or resign with a development cert first.
#
# B) Xcode → Window → Devices and Simulators
#    1. Select connected device
#    2. Installed Apps → + → pick AetherEngine.app (unzip IPA → Payload/)
#    3. Or Product → Destination → device, then Run (signed debug build)
#    4. Prefer a development-signed .app for day-to-day; unsigned IPA is for
#       sideload tools / TestFlight-less distribution experiments.
#
# C) ideviceinstaller (libimobiledevice; Homebrew: brew install libimobiledevice ideviceinstaller)
#    idevice_id -l
#    ideviceinstaller -i build/out/AetherEngine.ipa
#    ideviceinstaller -l | grep -i aether
#    ideviceinstaller -U <bundle-id>   # uninstall
#
# D) Also: AltStore / Sideloadly / TrollStore (see earlier sideload notes)
#
# Linux/CI host: cannot sideload; this checklist is documentation only.
# DEVICE_SIDELOAD_CHECKLIST=AppleConfigurator|XcodeDevices|ideviceinstaller|AltStore

# ---------- Device-run notes (batch20): first launch / entitlements / Documents game dir ----------
# After sideload (see DEVICE_SIDELOAD_CHECKLIST above), first launch on device:
#
# FIRST LAUNCH
#   1. Tap AetherEngine; if "Untrusted Developer", Settings → General → VPN & Device Management
#      → trust the signing identity, then relaunch.
#   2. Grant Local Network if prompted (multiplayer UDP listen/connect smokes).
#   3. On first launch the app creates Documents/AetherEngine/ (or Documents/aether/) for
#      writable game data — maps, saves, aether.cfg. No retail HL assets are bundled.
#
# ENTITLEMENTS (expected for development / sideload builds)
#   - application-identifier / team-id (from resign / Xcode signing)
#   - get-task-allow=true for debug (Xcode Devices); false for distribution sideload
#   - com.apple.security.application-groups optional; sandbox still allows app Documents
#   - No special Game Center / iCloud entitlement required for the demo path
#   - Microphone only if voice chat cue path is enabled later
#
# DOCUMENTS GAME DIR LAYOUT (under app container Documents/)
#   Documents/AetherEngine/
#     aether.cfg          — settings/cvars persisted from Options
#     valve/              — user-provided Half-Life game dir (optional)
#     cstrike/            — user-provided CS 1.6 dir (optional)
#     bshift/ gearbox/ czero/ — other mod dirs when present
#     maps/ or */maps/    — .bsp maps the FS VFS mounts via setup_game
#     saves/              — host save stubs
#   Without user game data the engine falls back to the synthetic BSP demo room.
#
# DEVICE_RUN_NOTES=first_launch|entitlements|Documents/AetherEngine|game_dir|synthetic_fallback
# Linux/CI host: cannot launch on device; these notes document the on-device path only.
