#pragma once
// AnimEventDispatcher — loads anim_events.json at startup and provides a
// lookup `eventsFor(clipKey)` and a `dispatch(action, param, x, y)` that
// fires concrete game effects (particles + SFX). Decouples AnimationSystem
// from the rest of the game.

#include "../audio/AudioManager.h"
#include "../components/AnimState.h"
#include "../systems/PartikelEmitters.h"
#include "../utils/JsonLoader.h"
#include <json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace AnimEventDispatcher {

namespace detail {
inline std::unordered_map<std::string, std::vector<AnimEvent>> &table() {
  static std::unordered_map<std::string, std::vector<AnimEvent>> t;
  return t;
}
}

// Load all events from JSON (call once at startup).
inline void load(const std::string &path) {
  detail::table().clear();
  nlohmann::json data;
  if (!JsonLoader::load(path, data)) return;
  int total = 0;
  for (auto it = data.begin(); it != data.end(); ++it) {
    const std::string key = it.key();
    if (key.size() > 0 && key[0] == '_') continue; // skip _comment fields
    if (!it.value().is_array()) continue;
    std::vector<AnimEvent> list;
    for (auto &e : it.value()) {
      AnimEvent ev;
      ev.frame  = e.value("frame", 0);
      ev.action = e.value("action", std::string{});
      ev.param  = e.value("param", std::string{});
      list.push_back(ev);
    }
    detail::table()[key] = list;
    total += (int)list.size();
  }
  TraceLog(LOG_INFO, "ANIM_EVENTS: Loaded %d events for %d clips",
           total, (int)detail::table().size());
}

// Lookup events for a clip key. The key is derived from the texture basename
// (stem of texturePath without extension).
inline const std::vector<AnimEvent> *eventsFor(const std::string &clipKey) {
  auto it = detail::table().find(clipKey);
  return (it == detail::table().end()) ? nullptr : &it->second;
}

// Convert an AnimClip's texturePath to a clip key.
// "assets/sprites/player/knight_run.png" -> "knight_run"
inline std::string keyFromPath(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  size_t dot   = path.find_last_of('.');
  size_t start = (slash == std::string::npos) ? 0 : slash + 1;
  size_t end   = (dot == std::string::npos || dot < start) ? path.size() : dot;
  return path.substr(start, end - start);
}

// Fire one event's effect at world position (x, y).
inline void dispatch(const AnimEvent &ev, float x, float y) {
  if (ev.action == "spawn_particles") {
    if      (ev.param == "blood")     PartikelEmitters::spawnBlood(x, y, 8);
    else if (ev.param == "dust")      PartikelEmitters::spawnDashDust(x, y, 1.0f);
    else if (ev.param == "fire")      PartikelEmitters::spawnFireTrail(x, y);
    else if (ev.param == "shockwave") PartikelEmitters::spawnSlamShockwave(x, y);
  } else if (ev.action == "play_sfx") {
    AudioManager::getInstance().playSFX(ev.param);
  }
  // Future actions: "screen_shake", "spawn_hitbox", etc.
}

} // namespace AnimEventDispatcher
