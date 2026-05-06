#pragma once
#include "../core/ECS.h"
#include <string>
#include <vector>

// Types of active ability effects (executed via Q/E key press, not passive).
// Each effect has parameters specific to its type read from JSON.
enum class ActiveAbilityType {
  PROJECTILE,        // Fire spear, etc — launch a damaging projectile
  SHIELD,            // Temp invulnerability for `duration` seconds
  TELEPORT,          // Short blink in facing direction by `distance` px
  AOE_KNOCKBACK,     // Push enemies in radius + brief stun
  LIFESTEAL_BURST,   // Next N hits heal X% of damage (consumed on use)
};

struct ActiveAbilityData {
  std::string id;            // matches assets/data/active_abilities.json key
  std::string name;
  std::string description;
  std::string iconPath;      // assets/sprites/abilities/<id>.png
  ActiveAbilityType type;

  float cooldown = 4.0f;     // seconds before usable again
  float staminaCost = 30.0f;
  float duration = 0.0f;     // for SHIELD (invuln seconds), LIFESTEAL_BURST (charges)

  // Type-specific parameters (semantic varies by type)
  float param1 = 0.0f;       // PROJECTILE: damage; TELEPORT: distance; AOE: radius
  float param2 = 0.0f;       // PROJECTILE: speed;  AOE: knockback force
  std::string damageType = "physical"; // for PROJECTILE / AOE
};

// Component attached to player. Two slots: Q and E.
// Player picks which 2 actives to equip via menu (future) or starts with defaults.
struct ActiveAbilities : public Component {
  ActiveAbilityData slotQ;
  ActiveAbilityData slotE;
  bool hasQ = false;
  bool hasE = false;

  float cooldownQ = 0.0f;    // seconds remaining
  float cooldownE = 0.0f;

  // For LIFESTEAL_BURST style abilities: number of pending charges
  int chargesQ = 0;
  int chargesE = 0;

  ActiveAbilities() = default;

  void tick(float dt) {
    if (cooldownQ > 0.0f) cooldownQ = (cooldownQ - dt < 0.0f) ? 0.0f : cooldownQ - dt;
    if (cooldownE > 0.0f) cooldownE = (cooldownE - dt < 0.0f) ? 0.0f : cooldownE - dt;
  }

  bool readyQ() const { return hasQ && cooldownQ <= 0.0f; }
  bool readyE() const { return hasE && cooldownE <= 0.0f; }
};
