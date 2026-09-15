/**
 * android_keyboard.h - physical keyboard -> virtual gamepad mapper.
 *
 * USB-OTG / Bluetooth keyboards (and Chromebooks) show up in SDL3 as keyboard
 * devices, but the game only listens to gamepad input (XInput). This mapper
 * installs an SDL event watch that translates key presses into the same
 * virtual gamepad the touch overlay drives, so a keyboard "just works" with
 * zero configuration. The layout is listed in the launcher's "Keyboard
 * controls" dialog and in BUILD_ANDROID.md.
 */

#pragma once

namespace rexport::keyboard {

// Installs the SDL event watch. Safe to call before the virtual pad exists
// (key events are dropped until the pad is attached); call after
// EnsureVirtualPadAttached() in the normal startup path.
void InstallMapper();

// Releases every keyboard-held button and recenters keyboard-driven sticks.
// Called when the activity pauses so keys held across a pause can't stick.
// Touch-overlay state is untouched (only keyboard-held inputs are released).
void ResetState();

}  // namespace rexport::keyboard
