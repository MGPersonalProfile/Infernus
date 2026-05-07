#pragma once
#include "../core/ECS.h"
#include "../core/ResourceManager.h"
#include "raylib.h"
#include <string>
#include <unordered_map>

enum class AnimStateType { IDLE, RUN, ATTACK, DEATH, CHARGE, SLAM };

// AnimEvent — fired when the current frame of a clip transitions to `frame`.
// Data-driven via assets/data/anim_events.json. Actions:
//   "spawn_particles" — param = "blood" | "dust" | "fire" | "shockwave"
//   "play_sfx"        — param = sfx name (e.g. "footstep", "swing")
struct AnimEvent {
  int frame = 0;
  std::string action;
  std::string param;
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
