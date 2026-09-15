/**
 * android_keyboard.cpp - physical keyboard -> virtual gamepad mapper.
 *
 * Layout (scancodes, so it is layout-independent: WASD works on AZERTY too):
 *
 *   Left stick .... W A S D          Right stick ... I J K L
 *   D-Pad ......... Arrow keys
 *   A ............. Space            B ............. Left Shift
 *   X ............. F                Y ............. R
 *   LB / RB ....... Q / E            LT / RT ....... C / V (digital: full pull)
 *   L3 / R3 ....... Tab / Right Shift
 *   Start ......... Enter            Back .......... Backspace
 *   Guide ......... Escape
 *
 * Keys drive the shared virtual joystick through the thread-safe writers in
 * android_gamepad.h, so no JNI round-trip happens per keypress. Unmapped keys
 * (volume, media, ...) are ignored, and events are never consumed, so the
 * rest of SDL keeps working normally.
 */

#include "android_keyboard.h"

#include "android_gamepad.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_stdinc.h>

#include <jni.h>

#include <mutex>

#include <rex/logging.h>

namespace rexport::keyboard {

namespace {

// SDL_GAMEPAD_BUTTON_* indices (must match the virtual pad layout in
// android_gamepad.cpp: buttons 0..15 in SDL order, axes LX LY RX RY LT RT).
constexpr int kBtnA = 0;
constexpr int kBtnB = 1;
constexpr int kBtnX = 2;
constexpr int kBtnY = 3;
constexpr int kBtnBack = 4;
constexpr int kBtnGuide = 5;
constexpr int kBtnStart = 6;
constexpr int kBtnL3 = 7;
constexpr int kBtnR3 = 8;
constexpr int kBtnLB = 9;
constexpr int kBtnRB = 10;
constexpr int kBtnDpadUp = 11;
constexpr int kBtnDpadDown = 12;
constexpr int kBtnDpadLeft = 13;
constexpr int kBtnDpadRight = 14;
constexpr int kTrackedButtons = 16;

struct StickState {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
};

std::mutex g_mutex;
bool g_installed = false;
// Keyboard-held inputs only (touch-overlay state lives on the Java side).
StickState g_sticks[2];
bool g_triggers[2] = {false, false};
bool g_buttons[kTrackedButtons] = {false};

// All helpers below run with g_mutex held. They call into the gamepad writers
// (which take the gamepad mutex); the gamepad never calls back here, so the
// lock order is fixed and cannot deadlock.

void PushStickLocked(int stick) {
  const StickState& s = g_sticks[stick];
  const float x = (s.right ? 1.0f : 0.0f) - (s.left ? 1.0f : 0.0f);
  const float y = (s.down ? 1.0f : 0.0f) - (s.up ? 1.0f : 0.0f);
  gamepad::VirtualSetStick(stick, x, y);
}

void SetButtonLocked(int button, bool down) {
  if (button < 0 || button >= kTrackedButtons) return;
  if (g_buttons[button] == down) return;
  g_buttons[button] = down;
  gamepad::VirtualSetButton(button, down);
}

void SetTriggerLocked(int trigger, bool down) {
  if (trigger < 0 || trigger > 1) return;
  if (g_triggers[trigger] == down) return;
  g_triggers[trigger] = down;
  gamepad::VirtualSetTrigger(trigger, down ? 1.0f : 0.0f);
}

bool SDLCALL EventWatch(void* /*userdata*/, SDL_Event* event) {
  if (event->type == SDL_EVENT_KEYBOARD_ADDED || event->type == SDL_EVENT_KEYBOARD_REMOVED) {
    REXLOG_INFO("keyboard: {} (id {})",
                event->type == SDL_EVENT_KEYBOARD_ADDED ? "connected" : "disconnected",
                static_cast<unsigned>(event->kdevice.which));
    return true;
  }
  if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP) {
    return true;
  }
  if (event->key.repeat) {
    return true;
  }
  const bool down = event->type == SDL_EVENT_KEY_DOWN;
  std::lock_guard<std::mutex> guard(g_mutex);
  switch (event->key.scancode) {
    // Left stick: WASD.
    case SDL_SCANCODE_W: g_sticks[0].up = down; PushStickLocked(0); break;
    case SDL_SCANCODE_S: g_sticks[0].down = down; PushStickLocked(0); break;
    case SDL_SCANCODE_A: g_sticks[0].left = down; PushStickLocked(0); break;
    case SDL_SCANCODE_D: g_sticks[0].right = down; PushStickLocked(0); break;
    // Right stick: IJKL.
    case SDL_SCANCODE_I: g_sticks[1].up = down; PushStickLocked(1); break;
    case SDL_SCANCODE_K: g_sticks[1].down = down; PushStickLocked(1); break;
    case SDL_SCANCODE_J: g_sticks[1].left = down; PushStickLocked(1); break;
    case SDL_SCANCODE_L: g_sticks[1].right = down; PushStickLocked(1); break;
    // D-Pad: arrows.
    case SDL_SCANCODE_UP: SetButtonLocked(kBtnDpadUp, down); break;
    case SDL_SCANCODE_DOWN: SetButtonLocked(kBtnDpadDown, down); break;
    case SDL_SCANCODE_LEFT: SetButtonLocked(kBtnDpadLeft, down); break;
    case SDL_SCANCODE_RIGHT: SetButtonLocked(kBtnDpadRight, down); break;
    // Face buttons.
    case SDL_SCANCODE_SPACE: SetButtonLocked(kBtnA, down); break;
    case SDL_SCANCODE_LSHIFT: SetButtonLocked(kBtnB, down); break;
    case SDL_SCANCODE_F: SetButtonLocked(kBtnX, down); break;
    case SDL_SCANCODE_R: SetButtonLocked(kBtnY, down); break;
    // Shoulders / triggers.
    case SDL_SCANCODE_Q: SetButtonLocked(kBtnLB, down); break;
    case SDL_SCANCODE_E: SetButtonLocked(kBtnRB, down); break;
    case SDL_SCANCODE_C: SetTriggerLocked(0, down); break;
    case SDL_SCANCODE_V: SetTriggerLocked(1, down); break;
    // Stick clicks.
    case SDL_SCANCODE_TAB: SetButtonLocked(kBtnL3, down); break;
    case SDL_SCANCODE_RSHIFT: SetButtonLocked(kBtnR3, down); break;
    // Menu buttons.
    case SDL_SCANCODE_RETURN: SetButtonLocked(kBtnStart, down); break;
    case SDL_SCANCODE_BACKSPACE: SetButtonLocked(kBtnBack, down); break;
    case SDL_SCANCODE_ESCAPE: SetButtonLocked(kBtnGuide, down); break;
    default: break;
  }
  return true;
}

}  // namespace

void InstallMapper() {
  std::lock_guard<std::mutex> guard(g_mutex);
  if (g_installed) {
    return;
  }
  g_installed = true;
  int count = 0;
  SDL_KeyboardID* ids = SDL_GetKeyboards(&count);
  REXLOG_INFO("keyboard: mapper installed ({} keyboard(s) present)", count);
  SDL_free(ids);
  if (!SDL_AddEventWatch(EventWatch, nullptr)) {
    REXLOG_ERROR("keyboard: SDL_AddEventWatch failed: {}", SDL_GetError());
  }
}

void ResetState() {
  std::lock_guard<std::mutex> guard(g_mutex);
  for (int i = 0; i < kTrackedButtons; ++i) {
    if (g_buttons[i]) {
      g_buttons[i] = false;
      gamepad::VirtualSetButton(i, false);
    }
  }
  for (int t = 0; t < 2; ++t) {
    if (g_triggers[t]) {
      g_triggers[t] = false;
      gamepad::VirtualSetTrigger(t, 0.0f);
    }
  }
  for (int s = 0; s < 2; ++s) {
    g_sticks[s] = StickState{};
    gamepad::VirtualSetStick(s, 0.0f, 0.0f);
  }
}

}  // namespace rexport::keyboard

extern "C" JNIEXPORT void JNICALL
Java_com_rexauto_port_MainActivity_nativeKeyboardReset(JNIEnv* /*env*/, jclass /*clazz*/) {
  rexport::keyboard::ResetState();
}
