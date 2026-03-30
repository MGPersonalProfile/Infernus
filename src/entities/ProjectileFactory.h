// ProjectileFactory — Creates projectile entities
//
// Spawns a projectile with Transform, Velocity, Sprite, Collider,
// Projectile, Combat, and Lifetime components.

#pragma once
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Lifetime.h"
#include "../components/Projectile.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "../core/ResourceManager.h"

class ProjectileFactory {
public:
  static Entity create(Registry &registry, Entity owner, float x, float y,
                       float dirX, float dirY, float speed, int damage,
                       float knockback, float lifetime) {
    Entity proj = registry.createEntity();

    registry.addComponent<Transform2D>(proj, x, y);
    registry.addComponent<Velocity>(proj, dirX * speed, dirY * speed);

    // Fireball sprite (12x12 per frame)
    Texture2D tex =
        ResourceManager::getInstance().getTexture("assets/sprites/fx/fireball.png");
    registry.addComponent<Sprite>(proj, tex, Rectangle{0, 0, 12, 12}, 8);

    registry.addComponent<Collider>(proj, 12.0f, 12.0f, false);
    registry.addComponent<Combat>(proj, damage, knockback, owner);
    registry.addComponent<Lifetime>(proj, lifetime);
    registry.addComponent<Projectile>(proj, damage, owner, speed, dirX, dirY);

    return proj;
  }
};
