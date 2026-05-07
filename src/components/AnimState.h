#pragma once
#include "../core/ECS.h"
#include "../core/ResourceManager.h"
#include "raylib.h"
#include <string>
#include <unordered_map>

enum class AnimStateType { IDLE, RUN, ATTACK, DEATH, CHARGE, SLAM };

// AnimEvent — fired when currentFrame transitions to `frame`. Schema follows
// assets/data/animation_events/events.json (Antigravity-edited).
// Actions and their params (extracted by AnimEventDispatcher::dispatch):
//   "play_sfx":        { sound: string, volume?: float }
//   "spawn_particles": { type: string, count?: int, offsetX?, offsetY?: float }
//   "screen_shake":    { intensity: float, duration: float }
//   "add_hitstop":     { duration: float }
//   "apply_velocity":  { vx: float, vy: float }
// Params are JSON to allow data-driven action growth without C++ schema churn.
struct AnimEvent {
  int frame = 0;
  std::string action;
  // Pretty-printed JSON of the params object. Stored as string to keep this
  // header self-contained (no <json.hpp> dependency in components/). The
  // dispatcher parses on demand — fine for at-most-N-events-per-frame fires.
  std::string paramsJson;
};

struct AnimClip {
  std::string texturePath;
  int frames = 1;
  float frameSpeed = 0.15f;
  float frameWidth = 0.0f;
  float frameHeight = 0.0f;
  bool loop = true;
  std::vector<AnimEvent> events;
};

// Manages multiple animation clips (idle, run, attack, etc.) and switches
// between them. Works alongside Animation + Sprite components — this component
// only stores the clip data and current state; AnimationSystem does the ticking.
struct AnimState : public Component {
  std::unordered_map<AnimStateType, AnimClip> clips;
  AnimStateType current = AnimStateType::IDLE;
  AnimStateType previous = AnimStateType::IDLE;
  bool dirty = true; // true when state changed, needs texture swap

  AnimState() = default;

  void addClip(AnimStateType type, const std::string &path, int frames,
               float speed, float w, float h, bool loop = true) {
    clips[type] = {path, frames, speed, w, h, loop};
  }

  void setState(AnimStateType newState) {
    if (newState == current) return;
    if (clips.find(newState) == clips.end()) return; // no clip for this state
    previous = current;
    current = newState;
    dirty = true;
  }

  bool hasClip(AnimStateType type) const {
    return clips.find(type) != clips.end();
  }
};
