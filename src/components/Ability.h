#pragma once
#include "../core/ECS.h"
#include <string>
#include <vector>

// Types of ability effects
enum class AbilityEffect {
  DAMAGE_BOOST,    // Increase base damage
  SPEED_BOOST,     // Increase movement speed
  HEAL,            // Restore HP
  SHIELD,          // Temporary invulnerability
  THORNS,          // Reflect damage on hit
  LIFESTEAL,       // Heal on dealing damage
  STAMINA_REGEN,   // Boost stamina regen rate
  FIRE_TRAIL,      // Leave damaging trail
  CRIT_CHANCE,     // Chance to deal double damage
  MAX_HP_UP,       // Increase max HP permanently
  DASH_DAMAGE,     // Dash deals damage
  ATTACK_SPEED_UP, // Reduce windup time
};

// Rarity affects color and power
enum class AbilityRarity { COMMON, RARE, EPIC };

// Single ability definition (data)
struct AbilityData {
  std::string id;
  std::string name;
  std::string description;
  AbilityEffect effect;
  AbilityRarity rarity = AbilityRarity::COMMON;

  float value = 0.0f;     // Primary effect value
  float duration = 0.0f;  // 0 = passive/permanent, >0 = timed buff
  float cooldown = 0.0f;  // 0 = passive, >0 = active ability

  // Tags for synergy system (Phase 6)
  std::vector<std::string> tags;
};

// Active buff state (for timed abilities)
struct ActiveBuff {
  AbilityEffect effect;
  float value;
  float remainingTime;
};

// Component: attached to the player to track acquired abilities
struct AbilityHolder : public Component {
  std::vector<AbilityData> abilities;    // All acquired abilities
  std::vector<ActiveBuff> activeBuffs;   // Currently active timed buffs

  // Cached stat modifiers (recomputed when abilities change)
  float damageMultiplier = 1.0f;
  float speedMultiplier = 1.0f;
  float staminaRegenMultiplier = 1.0f;
  float windupMultiplier = 1.0f;
  float critChance = 0.0f;
  float lifestealFraction = 0.0f;
  float thornsDamage = 0.0f;
  int bonusMaxHP = 0;
  bool dashDamage = false;

  // Recalculate all passive stat bonuses
  void recalculate() {
    damageMultiplier = 1.0f;
    speedMultiplier = 1.0f;
    staminaRegenMultiplier = 1.0f;
    windupMultiplier = 1.0f;
    critChance = 0.0f;
    lifestealFraction = 0.0f;
    thornsDamage = 0.0f;
    bonusMaxHP = 0;
    dashDamage = false;

    for (auto &ab : abilities) {
      if (ab.duration > 0.0f)
        continue; // timed abilities apply via activeBuffs
      applyEffect(ab.effect, ab.value);
    }
    for (auto &buff : activeBuffs) {
      applyEffect(buff.effect, buff.value);
    }
  }

  bool hasAbility(const std::string &id) const {
    for (auto &a : abilities)
      if (a.id == id)
        return true;
    return false;
  }

private:
  void applyEffect(AbilityEffect effect, float value) {
    switch (effect) {
    case AbilityEffect::DAMAGE_BOOST:
      damageMultiplier += value;
      break;
    case AbilityEffect::SPEED_BOOST:
      speedMultiplier += value;
      break;
    case AbilityEffect::STAMINA_REGEN:
      staminaRegenMultiplier += value;
      break;
    case AbilityEffect::CRIT_CHANCE:
      critChance += value;
      break;
    case AbilityEffect::LIFESTEAL:
      lifestealFraction += value;
      break;
    case AbilityEffect::THORNS:
      thornsDamage += value;
      break;
    case AbilityEffect::MAX_HP_UP:
      bonusMaxHP += (int)value;
      break;
    case AbilityEffect::ATTACK_SPEED_UP:
      windupMultiplier -= value;
      break;
    case AbilityEffect::DASH_DAMAGE:
      dashDamage = true;
      break;
    default:
      break;
    }
  }
};
