#pragma once
#include "../core/ECS.h"

// ----------------------------------------------------------------------------
// Health Component
// ----------------------------------------------------------------------------
// Manages an entity's vitality and invulnerability frames.
struct Health : public Component {
  int maxHP = 100;
  int currentHP = 100;

  // Invulnerability timer in seconds (i-frames).
  // If > 0.0f, the entity cannot take damage.
  float invulnerabilityTimer = 0.0f;

  // Visual feedback timer (e.g., flash red when hit)
  float hitFlashTimer = 0.0f;

  Health() = default;
  Health(int maxHP) : maxHP(maxHP), currentHP(maxHP) {}

  bool isDead() const { return currentHP <= 0; }
  bool isInvulnerable() const { return invulnerabilityTimer > 0.0f; }
};
