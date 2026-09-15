/**
 * android_gamepad.h - SDL3 virtual gamepad for the on-screen touch overlay.
 *
 * @added  deivid22srk, 2026 - Android port
 */

#pragma once

namespace rexport::gamepad {

/**
 * Attaches the virtual gamepad (idempotent, thread-safe). MUST be called
 * only after the runtime app's OnInitialize() returned - see the timing
 * comment in android_gamepad.cpp.
 *
 * @return true when the virtual pad is live.
 */
bool EnsureVirtualPadAttached();

/**
 * Shared virtual-pad writers, used by the JNI touch overlay and the physical
 * keyboard mapper alike. Thread-safe; no-ops (returning false) until the pad
 * is attached or when an index is out of range.
 */
bool VirtualSetButton(int button, bool down);
/** x/y in [-1, 1]; +y is DOWN (SDL gamepad convention). */
bool VirtualSetStick(int stick, float x, float y);
/** value in [0, 1]. */
bool VirtualSetTrigger(int trigger, float value);

}  // namespace rexport::gamepad
