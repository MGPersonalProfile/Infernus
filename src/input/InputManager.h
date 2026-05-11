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
    // SPECIAL_ATTACK retired: was redundant with Q/E active ability system.
    // The class "ultimate" still lives at executeSpecialAttack() but is no
    // longer player-triggered. Keeping enum for backward-compat / future re-add.
    keyBinds[InputAction::PARRY] = KEY_F;
    keyBinds[InputAction::ABILITY_Q] = KEY_Q;         // active ability slot 1
    keyBinds[InputAction::ABILITY_E] = KEY_E;         // active ability slot 2
    // C.2 consolidation: TAB is now the single key for all info menus.
    // OPEN_INVENTORY (was KEY_I) and OPEN_ABILITIES (was KEY_H) retired —
    // both views are reachable via tabs inside the INFO menu (TAB to open,
    // then arrow keys to switch tabs). Enum retained for backward-compat.
    keyBinds[InputAction::OPEN_INFO] = KEY_TAB;

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
    // SPECIAL_ATTACK gamepad bind retired (was LB) — see keyBinds note.
    padBinds[InputAction::PARRY] =
        GAMEPAD_BUTTON_RIGHT_TRIGGER_1; // RB on Xbox
    // Active abilities on triggers (Xbox-style)
    padBinds[InputAction::ABILITY_Q] =
        GAMEPAD_BUTTON_LEFT_TRIGGER_2;  // LT on Xbox
    padBinds[InputAction::ABILITY_E] =
        GAMEPAD_BUTTON_RIGHT_TRIGGER_2; // RT on Xbox
  }

  // Headless mode (no raylib window): all input queries return false. Set by
  // Game::init when headlessMode is on. Without this, IsKeyPressed/IsKeyDown
  // crash because raylib requires an InitWindow'd context.
  void setHeadless(bool h) { headless = h; }

  // Function pointers for ScriptedInput delegation. Set by Game::init when a
  // --script is loaded. Lets us avoid a circular header dep on ScriptedInput.h.
  using QueryFn = bool(*)(InputAction);
  void setScriptedQueries(QueryFn down, QueryFn pressed) {
    scriptedDown = down;
    scriptedPressed = pressed;
  }

  // Check if key/button for action is currently held down
  bool isActionDown(InputAction action) const {
    if (scriptedDown) return scriptedDown(action);
    if (headless) return false;
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
    if (scriptedPressed) return scriptedPressed(action);
    if (headless) return false;
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

  // Analog movement vector. Returns gamepad left-stick if gamepad connected
  // (with deadzone), else returns digital direction from WASD/arrow keys.
  // X/Y in [-1.0, 1.0]. Magnitude can be < 1.0 for analog.
  struct MoveVector { float x = 0.0f; float y = 0.0f; };
  MoveVector getMoveAxis() const {
    MoveVector v;
    if (scriptedDown) {
      // Derive analog vector from MOVE_* held states from the script.
      if (scriptedDown(InputAction::MOVE_LEFT))  v.x -= 1.0f;
      if (scriptedDown(InputAction::MOVE_RIGHT)) v.x += 1.0f;
      if (scriptedDown(InputAction::MOVE_UP))    v.y -= 1.0f;
      if (scriptedDown(InputAction::MOVE_DOWN))  v.y += 1.0f;
      return v;
    }
    if (headless) return v;
    if (IsGamepadAvailable(0)) {
      float ax = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
      float ay = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
      // Radial deadzone (0.2) so the stick doesn't drift on rest
      float mag = ax * ax + ay * ay;
      if (mag > 0.04f) { v.x = ax; v.y = ay; return v; }
    }
    // Keyboard fallback (digital)
    if (IsKeyDown(keyBinds.at(InputAction::MOVE_LEFT))) v.x -= 1.0f;
    if (IsKeyDown(keyBinds.at(InputAction::MOVE_RIGHT))) v.x += 1.0f;
    if (IsKeyDown(keyBinds.at(InputAction::MOVE_UP))) v.y -= 1.0f;
    if (IsKeyDown(keyBinds.at(InputAction::MOVE_DOWN))) v.y += 1.0f;
    return v;
  }

  bool isGamepadActive() const { return !headless && IsGamepadAvailable(0); }

  // === D.6: Input buffer (0.2s) =============================================
  // When the player presses an attack/dodge mid-animation, the press stays
  // "alive" for BUFFER_WINDOW seconds. Combat code peeks with
  // isBufferedPressed() and calls consumeBuffer() once the action fires —
  // so a press isn't lost if stamina is briefly insufficient or the player
  // is in a brief lockout.
  static constexpr float BUFFER_WINDOW = 0.2f;

  // Call once per frame, before any consumeBuffer / isBufferedPressed
  // queries. Top up the timer for any buffered action that was just pressed,
  // and decay everything else.
  void tickInputBuffer(float dt) {
    for (auto &kv : bufferTimers) kv.second += dt;
    static const InputAction tracked[] = {
        InputAction::ATTACK_LIGHT, InputAction::ATTACK_HEAVY,
        InputAction::DASH,         InputAction::PARRY,
        InputAction::ABILITY_Q,    InputAction::ABILITY_E,
    };
    for (auto a : tracked) {
      if (isActionPressed(a)) bufferTimers[a] = 0.0f;
    }
  }

  bool isBufferedPressed(InputAction action) const {
    auto it = bufferTimers.find(action);
    return it != bufferTimers.end() && it->second < BUFFER_WINDOW;
  }

  void consumeBuffer(InputAction action) {
    bufferTimers[action] = BUFFER_WINDOW + 1.0f; // mark past-window
  }

  // Number of actions currently buffered (timer still within BUFFER_WINDOW).
  // Exposed so telemetry can verify D.6 is actually queueing presses across
  // animation lockouts during headless QA runs.
  int activeBufferCount() const {
    int n = 0;
    for (const auto &kv : bufferTimers)
      if (kv.second < BUFFER_WINDOW) n++;
    return n;
  }

  // Menu navigation helpers. Route raw IsKeyPressed(KEY_LEFT/RIGHT/D/A) calls
  // in menu handlers through these so ScriptedInput can drive menu transitions
  // in headless QA (it intercepts InputAction queries, not raylib raw keys).
  bool menuNextPressed() const {
    if (scriptedPressed) return scriptedPressed(InputAction::MOVE_RIGHT);
    if (headless) return false;
    return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
  }
  bool menuPrevPressed() const {
    if (scriptedPressed) return scriptedPressed(InputAction::MOVE_LEFT);
    if (headless) return false;
    return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
  }

private:
  std::unordered_map<InputAction, int> keyBinds;
  std::unordered_map<InputAction, int> padBinds; // Placholder gamepad mappings
  bool headless = false; // when true, all input queries return idle
  // ScriptedInput hooks: when set, override raylib queries entirely. Used in
  // headless test runs to feed deterministic action timelines.
  QueryFn scriptedDown = nullptr;
  QueryFn scriptedPressed = nullptr;
  std::unordered_map<InputAction, float> bufferTimers; // D.6 ring buffer
};
