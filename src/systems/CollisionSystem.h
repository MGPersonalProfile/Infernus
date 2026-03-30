#pragma once
#include "../components/Collider.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "raylib.h"
#include <cmath>

class CollisionSystem {
public:
  // Sync collider rects to transform positions
  void update(Registry &registry) {
    auto entities = registry.view<Transform2D, Collider>();

    for (Entity entity : entities) {
      auto &transform = registry.getComponent<Transform2D>(entity);
      auto &collider = registry.getComponent<Collider>(entity);
      collider.rect.x = transform.x + collider.offsetX;
      collider.rect.y = transform.y + collider.offsetY;
    }
  }

  // Resolve collisions between moving entities and solid walls
  void resolveWallCollisions(Registry &registry) {
    // Collect wall colliders (static, non-trigger)
    auto allColliders = registry.view<Transform2D, Collider>();

    // For each moving entity, check against walls
    auto movers = registry.view<Transform2D, Collider, Velocity>();

    for (Entity mover : movers) {
      auto &mc = registry.getComponent<Collider>(mover);
      if (mc.isTrigger)
        continue; // triggers don't collide

      auto &mt = registry.getComponent<Transform2D>(mover);
      auto &mv = registry.getComponent<Velocity>(mover);

      for (Entity other : allColliders) {
        if (other == mover)
          continue;

        auto &oc = registry.getComponent<Collider>(other);
        if (oc.isTrigger)
          continue; // skip triggers
        // Skip other movers (only resolve against static walls)
        if (registry.hasComponent<Velocity>(other))
          continue;

        // AABB overlap check
        if (!CheckCollisionRecs(mc.rect, oc.rect))
          continue;

        // Get overlap rectangle
        Rectangle overlap = GetCollisionRec(mc.rect, oc.rect);

        // Push mover out along the smallest overlap axis
        if (overlap.width < overlap.height) {
          // Horizontal push
          float pushDir =
              (mc.rect.x + mc.rect.width / 2.0f <
               oc.rect.x + oc.rect.width / 2.0f)
                  ? -overlap.width
                  : overlap.width;
          mt.x += pushDir;
          mv.vx = 0.0f;
        } else {
          // Vertical push
          float pushDir =
              (mc.rect.y + mc.rect.height / 2.0f <
               oc.rect.y + oc.rect.height / 2.0f)
                  ? -overlap.height
                  : overlap.height;
          mt.y += pushDir;
          mv.vy = 0.0f;
        }

        // Re-sync collider after push
        mc.rect.x = mt.x + mc.offsetX;
        mc.rect.y = mt.y + mc.offsetY;
      }
    }
  }

  // Keep solid entities inside the play area
  void enforceBoundaries(Registry &registry, float screenWidth,
                         float screenHeight) {
    auto entities = registry.view<Transform2D, Collider, Velocity>();

    for (Entity entity : entities) {
      auto &transform = registry.getComponent<Transform2D>(entity);
      auto &collider = registry.getComponent<Collider>(entity);
      auto &velocity = registry.getComponent<Velocity>(entity);

      if (collider.isTrigger)
        continue;

      if (collider.rect.x < 0.0f) {
        transform.x = -collider.offsetX;
        velocity.vx = 0.0f;
      }
      if (collider.rect.x + collider.rect.width > screenWidth) {
        transform.x = screenWidth - collider.rect.width - collider.offsetX;
        velocity.vx = 0.0f;
      }
      if (collider.rect.y < 0.0f) {
        transform.y = -collider.offsetY;
        velocity.vy = 0.0f;
      }
      if (collider.rect.y + collider.rect.height > screenHeight) {
        transform.y = screenHeight - collider.rect.height - collider.offsetY;
        velocity.vy = 0.0f;
      }
    }
  }
};
