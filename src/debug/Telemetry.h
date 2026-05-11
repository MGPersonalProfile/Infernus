#pragma once
// Telemetry — emits per-tick game state JSONL for headless QA scripts.
// Activated when env var INFERNUS_TEST=1 (sets Game::enableTelemetry = true
// in main.cpp). Output goes to telemetry.jsonl in the cwd.
//
// Reader: tools/test_gameplay.py polls the file and asserts game invariants
// (HP > 0, position changes on input, enemies spawn, etc.).
//
// Format: one JSON object per line, fields stable across versions:
//   { "t": time_s, "hp": int, "stamina": int, "x": float, "y": float,
//     "enemies": int, "state": "PLAYING"|"GAME_OVER"|"VICTORY"|... }

#include "../components/Health.h"
#include "../components/Stamina.h"
#include "../components/Transform.h"
#include "../core/ECS.h"
#include "../core/GameState.h"
#include <cstdio>
#include <string>

namespace Telemetry {

namespace detail {
inline FILE *&fileHandle() { static FILE *h = nullptr; return h; }
inline float &accumTimer()  { static float t = 0.0f;   return t; }
inline float &elapsed()     { static float t = 0.0f;   return t; }
// UI overlay/state string for the snapshot. Set by the Game each frame so
// the headless QA scripts can verify menu/overlay transitions (G.1 tutorial,
// C.2 TAB consolidation) without reading the framebuffer.
inline std::string &uiStateRef() { static std::string s = "NONE"; return s; }
// Active input-buffer count (D.6). Set by the Game each frame from
// InputManager::activeBufferCount(). Lets QA verify queued presses survive
// animation lockouts.
inline int &inputBufferRef() { static int n = 0; return n; }
}

// Game sets these once per frame. tick() includes the current values in the
// next snapshot. Safe to call when telemetry is inactive — they're cheap.
inline void setUIState(const char *s)    { detail::uiStateRef() = s ? s : "NONE"; }
inline void setInputBufferSize(int n)    { detail::inputBufferRef() = n; }

// Open output file. Truncates on first call so each test run is fresh.
inline void open(const char *path = "telemetry.jsonl") {
  if (detail::fileHandle()) return;
  detail::fileHandle() = fopen(path, "w");
}

inline void close() {
  if (detail::fileHandle()) {
    fclose(detail::fileHandle());
    detail::fileHandle() = nullptr;
  }
}

// Maps GameState enum to short string. Keep stable for the python reader.
inline const char *stateLabel(GameState s) {
  switch (s) {
    case GameState::MAIN_MENU:        return "MAIN_MENU";
    case GameState::CHARACTER_SELECT: return "CHARACTER_SELECT";
    case GameState::PLAYING:          return "PLAYING";
    case GameState::PAUSED:           return "PAUSED";
    case GameState::GAME_OVER:        return "GAME_OVER";
    case GameState::VICTORY:          return "VICTORY";
    case GameState::ABILITY_SELECT:   return "ABILITY_SELECT";
    case GameState::BOSS_INTRO:       return "BOSS_INTRO";
    case GameState::INVENTORY:        return "INVENTORY";
    case GameState::INFO:             return "INFO";
    case GameState::ITEM_SWAP:        return "ITEM_SWAP";
    case GameState::MAP_SELECT:       return "MAP_SELECT";
    case GameState::SHOP:             return "SHOP";
    case GameState::REST:             return "REST";
    case GameState::OPTIONS:          return "OPTIONS";
    default:                          return "UNKNOWN";
  }
}

// Emit a discrete event line. Use for non-periodic happenings (hit confirmed,
// ability fired, room entered, boss phase change, player death). Cheap; no
// throttle. The Python reader can grep `"event":` to filter.
//
// Example: Telemetry::event("hit_dealt", "{\"dmg\":25,\"type\":\"fire\"}");
inline void event(const char *name, const char *jsonPayload = nullptr) {
  if (!detail::fileHandle()) open();
  if (!detail::fileHandle()) return;
  if (jsonPayload && jsonPayload[0]) {
    fprintf(detail::fileHandle(),
            "{\"t\":%.2f,\"event\":\"%s\",\"payload\":%s}\n",
            detail::elapsed(), name, jsonPayload);
  } else {
    fprintf(detail::fileHandle(),
            "{\"t\":%.2f,\"event\":\"%s\"}\n",
            detail::elapsed(), name);
  }
  fflush(detail::fileHandle());
}

// Returns true if telemetry is currently active (file is open). Lets call
// sites cheaply skip building event payloads when nothing's listening.
inline bool isActive() { return detail::fileHandle() != nullptr; }

// Emit a snapshot at most every `interval` seconds (default 200ms = 5 Hz).
// Cheap enough not to affect frame timing measurably.
inline void tick(Registry &registry, Entity playerEntity, GameState state,
                 int enemiesAlive, float deltaTime, float interval = 0.2f) {
  if (!detail::fileHandle()) open();
  detail::accumTimer() += deltaTime;
  detail::elapsed()    += deltaTime;
  if (detail::accumTimer() < interval) return;
  detail::accumTimer() = 0.0f;

  if (!registry.isAlive(playerEntity) ||
      !registry.hasComponent<Health>(playerEntity) ||
      !registry.hasComponent<Transform2D>(playerEntity)) {
    return;
  }
  auto &h = registry.getComponent<Health>(playerEntity);
  auto &t = registry.getComponent<Transform2D>(playerEntity);
  int stam = 0;
  if (registry.hasComponent<Stamina>(playerEntity)) {
    stam = (int)registry.getComponent<Stamina>(playerEntity).currentStamina;
  }

  fprintf(detail::fileHandle(),
          "{\"t\":%.2f,\"hp\":%d,\"stamina\":%d,\"x\":%.1f,\"y\":%.1f,"
          "\"enemies\":%d,\"state\":\"%s\","
          "\"ui\":\"%s\",\"input_buffer\":%d}\n",
          detail::elapsed(), h.currentHP, stam, t.x, t.y,
          enemiesAlive, stateLabel(state),
          detail::uiStateRef().c_str(), detail::inputBufferRef());
  fflush(detail::fileHandle()); // ensure reader sees lines promptly
}

} // namespace Telemetry
