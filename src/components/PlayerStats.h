#pragma once
#include "../core/ECS.h"
#include <string>

// Unified stats aggregation — single source of truth for all player stats.
// Recalculated when abilities, items, or synergies change.
struct PlayerStats : public Component {
  // Base stats (from character JSON, immutable during run)
  int baseMaxHP = 100;
  int baseDamage = 15;
  float baseSpeed = 250.0f;
  float baseMaxStamina = 100.0f;
  float baseStaminaRegen = 20.0f;

  // Computed final stats
  int finalMaxHP = 100;
  int finalDamage = 15;
  float finalSpeed = 250.0f;
  float finalMaxStamina = 100.0f;
  float finalStaminaRegen = 20.0f;
  float finalCritChance = 0.0f;
  float finalLifesteal = 0.0f;
  float finalThorns = 0.0f;
  float finalWindupMultiplier = 1.0f;
  bool dashDamage = false;

  // Attack pattern modifiers (from items)
  int extraHitboxes = 0;
  bool projectileAttack = false;
  bool areaAttack = false;
  bool chainAttack = false;
  float fireTrailChance = 0.0f;
  int chainBounces = 0;

  // Character class ID for reference
  std::string classId = "warrior";

  PlayerStats() = default;
};
