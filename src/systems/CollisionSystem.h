#pragma once
#include "../components/Collider.h"
#include "../components/Projectile.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "raylib.h"
#include <vector>

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

  // Resolve collisions between moving entities and solid walls.
  // Uses split-axis resolution: resolve X overlap first, re-sync, then Y.
  // This prevents corner-snagging where diagonal movement sticks on tile edges.
  void resolveWallCollisions(Registry &registry) {
    // Cache wall entities once (anything solid without Velocity)
    if (wallCacheDirty) rebuildWallCache(registry);

    auto movers = registry.view<Transform2D, Collider, Velocity>();

    for (Entity mover : movers) {
      auto &mc = registry.getComponent<Collider>(mover);
      if (mc.isTrigger) continue;

      auto &mt = registry.getComponent<Transform2D>(mover);
      auto &mv = registry.getComponent<Velocity>(mover);

      bool isProjectile = registry.hasComponent<Projectile>(mover);

      // --- X axis ---
      for (Entity wall : wallCache) {
        if (!registry.isAlive(wall)) continue;
        auto &wc = registry.getComponent<Collider>(wall);
        if (!CheckCollisionRecs(mc.rect, wc.rect)) continue;

        if (isProjectile) {
          registry.destroyEntity(mover);
          goto next_mover;
        }

        Rectangle overlap = GetCollisionRec(mc.rect, wc.rect);
        float mCenterX = mc.rect.x + mc.rect.width * 0.5f;
        float wCenterX = wc.rect.x + wc.rect.width * 0.5f;
        float pushX = (mCenterX < wCenterX) ? -overlap.width : overlap.width;
        mt.x += pushX;
        mv.vx = 0.0f;
        mc.rect.x = mt.x + mc.offsetX;
      }

      // --- Y axis ---
      mc.rect.y = mt.y + mc.offsetY; // re-sync after X push
      for (Entity wall : wallCache) {
        if (!registry.isAlive(wall)) continue;
        auto &wc = registry.getComponent<Collider>(wall);
        if (!CheckCollisionRecs(mc.rect, wc.rect)) continue;

        if (isProjectile) {
          registry.destroyEntity(mover);
          goto next_mover;
        }

        Rectangle overlap = GetCollisionRec(mc.rect, wc.rect);
        float mCenterY = mc.rect.y + mc.rect.height * 0.5f;
        float wCenterY = wc.rect.y + wc.rect.height * 0.5f;
        float pushY = (mCenterY < wCenterY) ? -overlap.height : overlap.height;
        mt.y += pushY;
        mv.vy = 0.0f;
        mc.rect.y = mt.y + mc.offsetY;
      }

      next_mover:;
    }
  }

  // Keep solid entities inside the play area (room bounds)
  void enforceBoundaries(Registry &registry, float roomWidth,
                         float roomHeight) {
    auto entities = registry.view<Transform2D, Collider, Velocity>();
    for (Entity entity : entities) {
      auto &transform = registry.getComponent<Transform2D>(entity);
      auto &collider = registry.getComponent<Collider>(entity);
      auto &velocity = registry.getComponent<Velocity>(entity);
      if (collider.isTrigger) continue;

      // Destroy projectiles that leave room bounds
      if (registry.hasComponent<Projectile>(entity)) {
        if (collider.rect.x + collider.rect.width < 0 ||
            collider.rect.x > roomWidth ||
            collider.rect.y + collider.rect.height < 0 ||
            collider.rect.y > roomHeight) {
          registry.destroyEntity(entity);
          continue;
        }
      }

      if (collider.rect.x < 0.0f) {
        transform.x = -collider.offsetX;
        velocity.vx = 0.0f;
      }
      if (collider.rect.x + collider.rect.width > roomWidth) {
        transform.x = roomWidth - collider.rect.width - collider.offsetX;
        velocity.vx = 0.0f;
      }
      if (collider.rect.y < 0.0f) {
        transform.y = -collider.offsetY;
        velocity.vy = 0.0f;
      }
      if (collider.rect.y + collider.rect.height > roomHeight) {
        transform.y = roomHeight - collider.rect.height - collider.offsetY;
        velocity.vy = 0.0f;
      }
    }
  }

  // Call when the room changes to force a wall cache rebuild
  void invalidateWallCache() { wallCacheDirty = true; }

private:
  std::vector<Entity> wallCache;
  bool wallCacheDirty = true;

  void rebuildWallCache(Registry &registry) {
    wallCache.clear();
    auto all = registry.view<Collider>();
    for (Entity e : all) {
      auto &c = registry.getComponent<Collider>(e);
      if (c.isTrigger) continue;
      if (registry.hasComponent<Velocity>(e)) continue; // skip movers
      wallCache.push_back(e);
    }
    wallCacheDirty = false;
  }
};
