#pragma once
#include "../core/ECS.h"
#include <string>
#include <unordered_map>

enum class AnimState {
  IDLE,
  RUN,
  ATTACK_LIGHT,
  ATTACK_HEAVY,
  DASH,
  HIT,
  DEATH,
  SPECIAL
};

struct AnimData {
  std::string spritePath;
  int frames = 2;
  float speed = 0.15f;
  float frameWidth = 48.0f;
  float frameHeight = 56.0f;
  bool loop = true;
};

// Hashes AnimState for use in unordered_map
struct AnimStateHash {
  std::size_t operator()(AnimState s) const {
    return std::hash<int>()(static_cast<int>(s));
  }
};

struct AnimationController : public Component {
  AnimState currentState = AnimState::IDLE;
  AnimState previousState = AnimState::IDLE;

  std::unordered_map<AnimState, AnimData, AnimStateHash> animations;

  void addAnim(AnimState state, const std::string &path, int frames, float speed,
               float w, float h, bool loop = true) {
    animations[state] = {path, frames, speed, w, h, loop};
  }

  AnimationController() = default;
};
