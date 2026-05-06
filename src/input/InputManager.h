#pragma once
#include "raylib.h"
#include <iostream>
#include <unordered_map>

// Action mappings
enum class InputAction {
  MOVE_UP,
  MOVE_DOWN,
  MOVE_LEFT,
  MOVE_RIGHT,
  ATTACK_LIGHT,
  ATTACK_HEAVY,
  DASH,
  INTERACT,
  SPECIAL_ATTACK,
  PARRY,
  ABILITY_Q,           // Active ability slot 1 (Q)
  ABILITY_E,           // Active ability slot 2 (E)
  OPEN_INVENTORY,
  OPEN_INFO,
  OPEN_ABILITIES
};

class InputManager {
public:
  void init() {
    // Default Key Bindings
    keyBinds[InputAction::MOVE_UP] = KEY_W;
    keyBinds[InputAction::MOVE_DOWN] = KEY_S;
    keyBinds[InputAction::MOVE_LEFT] = KEY_A;
    keyBinds[InputAction::MOVE_RIGHT] = KEY_D;

    keyBinds[InputAction::ATTACK_LIGHT] = KEY_J;
    keyBinds[InputAction::ATTACK_HEAVY] = KEY_K;
    keyBinds[InputAction::DASH] = KEY_SPACE;
    keyBinds[InputAction::INTERACT] = KEY_R;          // moved from E (E now ability slot 2)
    keyBinds[InputAction::SPECIAL_ATTACK] = KEY_L;
    keyBinds[InputAction::PARRY] = KEY_F;
    keyBinds[InputAction::ABILITY_Q] = KEY_Q;         // active ability slot 1
    keyBinds[InputAction::ABILITY_E] = KEY_E;         // active ability slot 2
    keyBinds[InputAction::OPEN_INVENTORY] = KEY_I;
    keyBinds[InputAction::OPEN_INFO] = KEY_TAB;
    keyBinds[InputAction::OPEN_ABILITIES] = KEY_H;

    // Default Gamepad Bindings (Preparing Interface)
    padBinds[InputAction::MOVE_UP] = GAMEPAD_BUTTON_LEFT_FACE_UP;
    padBinds[InputAction::MOVE_DOWN] = GAMEPAD_BUTTON_LEFT_FACE_DOWN;
    padBinds[InputAction::MOVE_LEFT] = GAMEPAD_BUTTON_LEFT_FACE_LEFT;
    padBinds[InputAction::MOVE_RIGHT] = GAMEPAD_BUTTON_LEFT_FACE_RIGHT;

    padBinds[InputAction::ATTACK_LIGHT] =
        GAMEPAD_BUTTON_RIGHT_FACE_LEFT; // X on Xbox
    padBinds[InputAction::ATTACK_HEAVY] =
        GAMEPAD_BUTTON_RIGHT_FACE_UP;                             // Y on Xbox
    padBinds[InputAction::DASH] = GAMEPAD_BUTTON_RIGHT_FACE_DOWN; // A on Xbox
    padBinds[InputAction::INTERACT] =
        GAMEPAD_BUTTON_RIGHT_FACE_RIGHT; // B on Xbox
    padBinds[InputAction::SPECIAL_ATTACK] =
        GAMEPAD_BUTTON_LEFT_TRIGGER_1; // LB on Xbox
    padBinds[InputAction::PARRY] =
        GAMEPAD_BUTTON_RIGHT_TRIGGER_1; // RB on Xbox
    // Active abilities on triggers (Xbox-style)
    padBinds[InputAction::ABILITY_Q] =
        GAMEPAD_BUTTON_LEFT_TRIGGER_2;  // LT on Xbox
    padBinds[InputAction::ABILITY_E] =
        GAMEPAD_BUTTON_RIGHT_TRIGGER_2; // RT on Xbox
  }

  // Check if key/button for action is currently held down
  bool isActionDown(InputAction action) const {
    bool down = false;

    // Check Keyboard
    if (keyBinds.find(action) != keyBinds.end()) {
      down = IsKeyDown(keyBinds.at(action));
    }

    // Check Gamepad (if any gamepad is available, just check gamepad 0 for
    // prototyping)
    if (!down && IsGamepadAvailable(0)) {
      if (padBinds.find(action) != padBinds.end()) {
        down = IsGamepadButtonDown(0, padBinds.at(action));
      }
    }

    return down;
  }

  // Check if key/button for action was just pressed this frame
  bool isActionPressed(InputAction action) const {
    bool pressed = false;

    // Check Keyboard
    if (keyBinds.find(action) != keyBinds.end()) {
      pressed = IsKeyPressed(keyBinds.at(action));
    }

    // Check Gamepad
    if (!pressed && IsGamepadAvailable(0)) {
      if (padBinds.find(action) != padBinds.end()) {
        pressed = IsGamepadButtonPressed(0, padBinds.at(action));
      }
    }

    return pressed;
  }

private:
  std::unordered_map<InputAction, int> keyBinds;
  std::unordered_map<InputAction, int> padBinds; // Placholder gamepad mappings
};
