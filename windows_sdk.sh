#!/bin/bash
# windows_sdk.sh -- clone ReXGlue SDK v0.10.0 (the tag the port was generated
# against), init submodules, apply the desktop patch subset.
#
#   ./windows_sdk.sh [dest=windows/sdk/rexglue-sdk] [phase=all]
#
# Phases (CI runs them as separate steps so a failure points at the phase):
#   clone       fresh clone of the SDK tag (LF worktree, long paths allowed)
#   submodules  init/update third-party submodules (shallow)
#   patches     apply the desktop patch subset (base + perf + fps-cap)
#   all         clone + submodules + patches (default; skips if already done)
#
# Patch set (must stay in this order): the base fixes, the shared performance
# work (interrupt serialization, fault-log throttling, mobile-class memory
# diagnostics - all of it applies to desktop too), and the vblank FPS cap.
# The Android-only patches are deliberately NOT applied here.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
SDK_DIR="${1:-$ROOT/windows/sdk/rexglue-sdk}"
PHASE="${2:-all}"
SDK_REPO="https://github.com/rexglue/rexglue-sdk.git"
SDK_TAG="v0.10.0"
PATCHES="$ROOT/android/patches"

# Report the failing command as a CI annotation (readable via the API too).
on_err() { echo "::error::windows_sdk.sh[$PHASE] failed at: $BASH_COMMAND"; }
trap on_err ERR
if [ "${CI:-}" = "true" ]; then set -x; fi

do_clone() {
  rm -rf "$SDK_DIR"; mkdir -p "$(dirname "$SDK_DIR")"
  echo "[windows_sdk] cloning $SDK_REPO@$SDK_TAG"
  # LF worktree so the .patch files apply on Windows checkouts too;
  # longpaths because the tree under D:\a\... can exceed MAX_PATH.
  git -c core.autocrlf=false -c core.longpaths=true clone --depth 1 \
    --branch "$SDK_TAG" "$SDK_REPO" "$SDK_DIR"
}

do_submodules() {
  echo "[windows_sdk] submodules"
  git -C "$SDK_DIR" -c core.longpaths=true submodule update --init --depth 1
}

do_patches() {
  for p in rexglue-sdk-v0.10.0.patch rexglue-sdk-v0.10.0-android-perf.patch rexglue-sdk-v0.10.0-fps-cap.patch; do
    if grep -q $'\r' "$PATCHES/$p"; then
      echo "::error::windows_sdk.sh[patches] $p has CRLF line endings"
    fi
    echo "[windows_sdk] applying $p"
    git -C "$SDK_DIR" -c core.autocrlf=false -c core.longpaths=true apply \
      "$PATCHES/$p"
  done
  touch "$SDK_DIR/.patches-applied"
}

case "$PHASE" in
  clone) do_clone;;
  submodules) do_submodules;;
  patches) do_patches;;
  all)
    if [ -f "$SDK_DIR/.patches-applied" ]; then
      echo "[windows_sdk] SDK already set up at $SDK_DIR"
    else
      do_clone; do_submodules; do_patches
    fi;;
  *) echo "unknown phase: $PHASE (want clone|submodules|patches|all)" >&2; exit 2;;
esac
echo "[windows_sdk] done ($PHASE)"
