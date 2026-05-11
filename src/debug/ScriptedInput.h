#pragma once
// ScriptedInput — replays a timeline of input actions for headless smoke
// tests. Lets QA scripts simulate combat sequences without pyautogui.
//
// Activation:
//   build/INFERNUS.exe --headless --script combat_basic --duration 10
//   (or env INFERNUS_SCRIPT=combat_basic)
//
// Script format: assets/test_scripts/<name>.json
//   [
//     { "t": 0.5,  "action": "MOVE_RIGHT", "duration": 1.0 },
//     { "t": 1.5,  "action": "ATTACK_LIGHT" },
//     { "t": 2.0,  "action": "DASH" },
//     { "t": 2.5,  "action": "ABILITY_Q" }
//   ]
//
// Actions match InputAction enum names. `t` is seconds since script start.
// `duration` (optional, only for held-down inputs like MOVE_*) keeps the
// action active for that many seconds. Tap actions (ATTACK_*, DASH, ABILITY_*)
// fire as a one-frame "pressed" event.

#include "../input/InputManager.h"
#include "../utils/JsonLoader.h"
#include <json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace ScriptedInput {

struct ScriptedAction {
  float startTime = 0.0f;
  float duration = 0.0f;     // 0 = single-frame tap
  InputAction action = InputAction::MOVE_UP;
  bool fired = false;        // for taps: prevents re-fire
};

namespace detail {
inline std::vector<ScriptedAction> &script() { static std::vector<ScriptedAction> s; return s; }
inline float &elapsed() { static float t = 0.0f; return t; }
inline bool &active() { static bool a = false; return a; }

// Tap-actions that should be reported only on the frame they fire (isActionPressed).
// MOVE_* actions are continuous (isActionDown for `duration` seconds).
inline bool isTapAction(InputAction a) {
  switch (a) {
    case InputAction::ATTACK_LIGHT:
    case InputAction::ATTACK_HEAVY:
    case InputAction::DASH:
    case InputAction::PARRY:
    case InputAction::ABILITY_Q:
    case InputAction::ABILITY_E:
    case InputAction::INTERACT:
    case InputAction::OPEN_INVENTORY:
    case InputAction::OPEN_INFO:
    case InputAction::OPEN_ABILITIES:
      return true;
    default:
      return false;
  }
}
}

inline InputAction parseActionName(const std::string &name) {
  if (name == "MOVE_UP")        return InputAction::MOVE_UP;
  if (name == "MOVE_DOWN")      return InputAction::MOVE_DOWN;
  if (name == "MOVE_LEFT")      return InputAction::MOVE_LEFT;
  if (name == "MOVE_RIGHT")     return InputAction::MOVE_RIGHT;
  if (name == "ATTACK_LIGHT")   return InputAction::ATTACK_LIGHT;
  if (name == "ATTACK_HEAVY")   return InputAction::ATTACK_HEAVY;
  if (name == "DASH")           return InputAction::DASH;
  if (name == "PARRY")          return InputAction::PARRY;
  if (name == "ABILITY_Q")      return InputAction::ABILITY_Q;
  if (name == "ABILITY_E")      return InputAction::ABILITY_E;
  if (name == "INTERACT")       return InputAction::INTERACT;
  if (name == "OPEN_INVENTORY") return InputAction::OPEN_INVENTORY;
  if (name == "OPEN_INFO")      return InputAction::OPEN_INFO;
  if (name == "OPEN_ABILITIES") return InputAction::OPEN_ABILITIES;
  return InputAction::MOVE_UP; // unrecognized -> harmless default
}

// Load script from JSON. Returns true on success.
inline bool load(const std::string &path) {
  detail::script().clear();
  detail::elapsed() = 0.0f;
  detail::active() = false;

  nlohmann::json data;
  if (!JsonLoader::load(path, data)) return false;
  if (!data.is_array()) {
    TraceLog(LOG_WARNING, "SCRIPTED_INPUT: %s root must be an array", path.c_str());
    return false;
  }
  for (auto &entry : data) {
    ScriptedAction a;
    a.startTime = entry.value("t", 0.0f);
    a.duration  = entry.value("duration", 0.0f);
    a.action    = parseActionName(entry.value("action", std::string{"MOVE_UP"}));
    detail::script().push_back(a);
  }
  detail::active() = !detail::script().empty();
  TraceLog(LOG_INFO, "SCRIPTED_INPUT: Loaded %d actions from %s",
           (int)detail::script().size(), path.c_str());
  return true;
}

inline bool isActive() { return detail::active(); }

// Advance script clock. Call once per frame from Game::update.
inline void tick(float deltaTime) {
  if (!detail::active()) return;
  detail::elapsed() += deltaTime;
}

// Is the action currently held down? (continuous MOVE_* via duration)
inline bool isActionDown(InputAction action) {
  if (!detail::active()) return false;
  float now = detail::elapsed();
  for (auto &a : detail::script()) {
    if (a.action != action) continue;
    if (a.duration <= 0.0f) continue; // tap-only entries don't hold
    if (now >= a.startTime && now < a.startTime + a.duration) return true;
  }
  return false;
}

// Was the action fired this frame? Fires on the first frame the action
// becomes active and is then suppressed (via `fired`) so it only
// pulses once per scheduled entry — works for both tap actions and
// the leading edge of HELD actions (e.g. MOVE_RIGHT inside a menu).
inline bool isActionPressed(InputAction action) {
  if (!detail::active()) return false;
  float now = detail::elapsed();
  for (auto &a : detail::script()) {
    if (a.action != action) continue;
    if (a.fired) continue;
    if (now >= a.startTime) {
      a.fired = true;
      return true;
    }
  }
  return false;
}

// Analog move vector derived from MOVE_LEFT/RIGHT/UP/DOWN held states.
// Returns {0,0} if no movement actions are active.
inline InputManager::MoveVector getMoveAxis() {
  InputManager::MoveVector v;
  if (!detail::active()) return v;
  if (isActionDown(InputAction::MOVE_LEFT))  v.x -= 1.0f;
  if (isActionDown(InputAction::MOVE_RIGHT)) v.x += 1.0f;
  if (isActionDown(InputAction::MOVE_UP))    v.y -= 1.0f;
  if (isActionDown(InputAction::MOVE_DOWN))  v.y += 1.0f;
  return v;
}

} // namespace ScriptedInput
