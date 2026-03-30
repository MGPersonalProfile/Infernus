#pragma once
#include "../core/ECS.h"

// Represents a temporary entity that will be automatically destroyed
// after timeRemaining reaches 0. Used by hitboxes, particles, loot orbs, etc.
struct Lifetime : public Component {
  float maxTime = 1.0f;       // Original total lifetime (for interpolation)
  float timeRemaining = 1.0f; // Counts down to 0

  Lifetime() = default;
  Lifetime(float time) : maxTime(time), timeRemaining(time) {}

  // Returns 0.0 at start, 1.0 at end of life (useful for fade-outs)
  float progress() const {
    return (maxTime > 0.0f) ? 1.0f - (timeRemaining / maxTime) : 1.0f;
  }
};
