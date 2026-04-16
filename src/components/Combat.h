#pragma once
#include "../core/ECS.h"

enum class DamageType { PHYSICAL, FIRE, ICE, LIGHTNING, TOXIC };

enum class AttackState { NONE, WINDUP, ACTIVE, RECOVERY, PARRY_ACTIVE, PARRY_RECOVERY };
enum class AttackType { LIGHT, HEAVY };

// Gives an entity the ability to deal damage.
// Attached to players, enemies, and transient hitbox entities.
struct Combat : public Component {
  int baseDamage = 10;
  DamageType damageType = DamageType::PHYSICAL;
  float knockbackForce = 150.0f;

  AttackState currentState = AttackState::NONE;
  float stateTimer = 0.0f;

  AttackType lastAttackType = AttackType::LIGHT;

  // Combo tracking
  int comboCount = 0;          // 0-2 = light hits, 3 = finisher pending
  float comboTimer = 0.0f;     // time left to chain next hit
  bool isFinisher = false;     // true when current attack is a combo finisher

  // Owner entity (used by transient hitboxes to avoid self-damage)
  Entity owner = NULL_ENTITY;

  Combat() = default;
  Combat(int damage, float knockback)
      : baseDamage(damage), knockbackForce(knockback) {}
  Combat(int damage, float knockback, Entity ownerEntity)
      : baseDamage(damage), knockbackForce(knockback), owner(ownerEntity) {}
};
