#pragma once
#include "../components/Combat.h"  // DamageType
#include "../core/ECS.h"

// ----------------------------------------------------------------------------
// Elemental Resistances — multiplier per DamageType (0.0 = immune, 1.0 = normal, >1.0 = weak)
// ----------------------------------------------------------------------------
struct Resistances {
  float physical  = 1.0f;
  float fire      = 1.0f;
  float ice       = 1.0f;
  float lightning  = 1.0f;
  float toxic     = 1.0f;

  float get(DamageType type) const {
    switch (type) {
    case DamageType::PHYSICAL:  return physical;
    case DamageType::FIRE:      return fire;
    case DamageType::ICE:       return ice;
    case DamageType::LIGHTNING: return lightning;
    case DamageType::TOXIC:     return toxic;
    }
    return 1.0f;
  }
};

// ----------------------------------------------------------------------------
// Health Component
// ----------------------------------------------------------------------------
// Manages an entity's vitality and invulnerability frames.
struct Health : public Component {
  int maxHP = 100;
  int currentHP = 100;

  // Elemental resistances
  Resistances resistances;

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
