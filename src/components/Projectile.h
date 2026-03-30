#pragma once
#include "../core/ECS.h"

// Marker component for projectile entities.
// Damage and knockback are handled by the Combat component.
// Movement direction/speed are handled by the Velocity component.
struct Projectile : public Component {
  int damage = 5;
  Entity owner = NULL_ENTITY;
  float speed = 300.0f;
  float dirX = 1.0f;
  float dirY = 0.0f;

  Projectile() = default;
  Projectile(int dmg, Entity ownerEntity, float spd, float dx, float dy)
      : damage(dmg), owner(ownerEntity), speed(spd), dirX(dx), dirY(dy) {}
};
