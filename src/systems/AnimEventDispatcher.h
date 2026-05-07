#pragma once
// AnimEventDispatcher — loads animation_events/events.json (Antigravity's
// expanded schema) and fires concrete game effects on frame transitions.
//
// Schema (file = assets/data/animation_events/events.json):
//   { "<group>": { "<clip>": { "events": [ {frame, action, params:{...}} ] } } }
//   group ∈ { "player", "melee", "ranged", "tank", "assassin", "bomber",
//             "minotaur", ... }; clip ∈ { "attack", "run", "death", ... }
//
// Lookup key resolution (from AnimClip.texturePath):
//   "assets/sprites/player/knight_attack.png" -> stem "knight_attack"
//   The dispatcher tries: stem ("knight_attack") first, then suffix-only
//   ("attack"). This lets one schema entry serve all knight/warrior/rogue.

#include "../audio/AudioManager.h"
#include "../components/AnimState.h"
#include "../systems/CameraSystem.h"
#include "../systems/PartikelEmitters.h"
#include "../systems/ScreenEffects.h"
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
inline CameraSystem*  &camPtr()    { static CameraSystem*  p = nullptr; return p; }
inline ScreenEffects* &fxPtr()     { static ScreenEffects* p = nullptr; return p; }
}

// Wire the dispatcher to live engine subsystems. Call once at Game::init.
inline void wire(CameraSystem *cam, ScreenEffects *fx) {
  detail::camPtr() = cam;
  detail::fxPtr() = fx;
}

// Load events.json. Iterates 2 levels deep: groups -> clips -> events.
inline void load(const std::string &path) {
  detail::table().clear();
  nlohmann::json data;
  if (!JsonLoader::load(path, data)) return;
  int total = 0;
  for (auto groupIt = data.begin(); groupIt != data.end(); ++groupIt) {
    const std::string gkey = groupIt.key();
    if (!gkey.empty() && gkey[0] == '_') continue; // skip _schema, _description
    if (!groupIt.value().is_object()) continue;
    for (auto clipIt = groupIt.value().begin(); clipIt != groupIt.value().end(); ++clipIt) {
      const std::string ckey = clipIt.key();
      if (!ckey.empty() && ckey[0] == '_') continue;
      if (!clipIt.value().is_object()) continue;
      auto evField = clipIt.value().find("events");
      if (evField == clipIt.value().end() || !evField->is_array()) continue;
      std::vector<AnimEvent> list;
      for (auto &e : *evField) {
        AnimEvent ev;
        ev.frame  = e.value("frame", 0);
        ev.action = e.value("action", std::string{});
        if (e.contains("params")) ev.paramsJson = e["params"].dump();
        list.push_back(ev);
      }
      // Index two ways: full key "knight_attack" if group is a class name,
      // and bare clip key "attack" so any knight_attack/warrior_attack hits it.
      detail::table()[gkey + "_" + ckey] = list;
      // Don't overwrite a more specific entry if it already exists at bare key.
      if (detail::table().find(ckey) == detail::table().end()) {
        detail::table()[ckey] = list;
      }
      total += (int)list.size();
    }
  }
  TraceLog(LOG_INFO, "ANIM_EVENTS: Loaded %d events across %d entries",
           total, (int)detail::table().size());
}

// Lookup events for a clip key. Tries full path stem first, then bare suffix.
inline const std::vector<AnimEvent> *eventsFor(const std::string &clipKey) {
  auto &t = detail::table();
  auto it = t.find(clipKey);
  if (it != t.end()) return &it->second;
  // Strip class prefix: "knight_attack" -> "attack"
  size_t under = clipKey.find('_');
  if (under != std::string::npos) {
    auto it2 = t.find(clipKey.substr(under + 1));
    if (it2 != t.end()) return &it2->second;
  }
  return nullptr;
}

// "assets/sprites/player/knight_run.png" -> "knight_run"
inline std::string keyFromPath(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  size_t dot   = path.find_last_of('.');
  size_t start = (slash == std::string::npos) ? 0 : slash + 1;
  size_t end   = (dot == std::string::npos || dot < start) ? path.size() : dot;
  return path.substr(start, end - start);
}

// Fire one event at world position (x, y). Reads params on demand.
inline void dispatch(const AnimEvent &ev, float x, float y) {
  nlohmann::json p;
  if (!ev.paramsJson.empty()) {
    try { p = nlohmann::json::parse(ev.paramsJson); }
    catch (...) { /* malformed — silently skip params */ }
  }

  if (ev.action == "play_sfx") {
    std::string sound = p.value("sound", std::string{});
    if (!sound.empty()) AudioManager::getInstance().playSFX(sound);
  } else if (ev.action == "spawn_particles") {
    std::string type = p.value("type", std::string{});
    int count = p.value("count", 6);
    float ox = p.value("offsetX", 0.0f);
    float oy = p.value("offsetY", 0.0f);
    float px = x + ox, py = y + oy;
    if      (type == "blood" || type == "blood_splash" || type == "blood_burst") PartikelEmitters::spawnBlood(px, py, count);
    else if (type == "dash_dust" || type == "dust")  PartikelEmitters::spawnDashDust(px, py, 1.0f);
    else if (type == "fire_burst" || type == "fire_gather" || type == "fire_explosion" || type == "fire_trail") PartikelEmitters::spawnFireTrail(px, py);
    else if (type == "shockwave" || type == "slam_shockwave") PartikelEmitters::spawnSlamShockwave(px, py);
    else if (type == "spark_burst" || type == "soul_release" || type == "soul_wisps") PartikelEmitters::spawnBlood(px, py, count); // fallback to blood-style sparkle
    // Unknown types silently skip — Antigravity can extend later.
  } else if (ev.action == "screen_shake") {
    if (auto *cam = detail::camPtr()) {
      float intensity = p.value("intensity", 4.0f);
      float duration  = p.value("duration", 0.2f);
      cam->addShake(intensity, duration);
    }
  } else if (ev.action == "add_hitstop") {
    if (auto *fx = detail::fxPtr()) {
      float duration = p.value("duration", 0.06f);
      fx->addHitstop(duration);
    }
  }
  // Future: activate_hitbox, deactivate_hitbox, apply_velocity, set_invulnerable
  // need entity context — would require a different signature.
}

} // namespace AnimEventDispatcher
