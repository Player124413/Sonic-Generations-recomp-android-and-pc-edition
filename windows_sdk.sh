#!/bin/bash
# windows_sdk.sh -- clone ReXGlue SDK v0.10.0 (the tag the port was generated
# against), init submodules, apply the desktop patch subset.
#
#   ./windows_sdk.sh [dest=windows/sdk/rexglue-sdk]
#
# Patch set (must stay in this order): the base fixes, the shared performance
# work (interrupt serialization, fault-log throttling, mobile-class memory
# diagnostics - all of it applies to desktop too), and the vblank FPS cap.
# The Android-only patches are deliberately NOT applied here.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
SDK_DIR="${1:-$ROOT/windows/sdk/rexglue-sdk}"
SDK_REPO="https://github.com/rexglue/rexglue-sdk.git"
SDK_TAG="v0.10.0"
PATCHES="$ROOT/android/patches"

if [ -f "$SDK_DIR/.patches-applied" ]; then
  echo "[windows_sdk] SDK already set up at $SDK_DIR"
else
  rm -rf "$SDK_DIR"; mkdir -p "$(dirname "$SDK_DIR")"
  echo "[windows_sdk] cloning $SDK_REPO@$SDK_TAG"
  # LF worktree so the .patch files apply on Windows checkouts too.
  git -c core.autocrlf=false clone --depth 1 --branch "$SDK_TAG" "$SDK_REPO" "$SDK_DIR"
  git -C "$SDK_DIR" submodule update --init --depth 1
  for p in rexglue-sdk-v0.10.0.patch rexglue-sdk-v0.10.0-android-perf.patch rexglue-sdk-v0.10.0-fps-cap.patch; do
    echo "[windows_sdk] applying $p"
    git -C "$SDK_DIR" -c core.autocrlf=false apply "$PATCHES/$p"
  done
  touch "$SDK_DIR/.patches-applied"
fi
echo "[windows_sdk] done"
