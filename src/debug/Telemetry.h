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
}

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
          "\"enemies\":%d,\"state\":\"%s\"}\n",
          detail::elapsed(), h.currentHP, stam, t.x, t.y,
          enemiesAlive, stateLabel(state));
  fflush(detail::fileHandle()); // ensure reader sees lines promptly
}

} // namespace Telemetry
