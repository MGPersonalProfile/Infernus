#pragma once
#include "../components/PhysicsBody.h"
#include "../components/Particle.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "raylib.h"
#include <cmath>

// Applies physics-based movement (acceleration, friction, mass) to entities
// with PhysicsBody, and spawns dust/impact particles on speed changes.
class PhysicsVFXSystem {
public:
  void update(Registry &registry, float deltaTime) {
    auto entities = registry.view<Transform2D, PhysicsBody, Velocity>();

    for (Entity e : entities) {
      auto &body = registry.getComponent<PhysicsBody>(e);
      auto &vel = registry.getComponent<Velocity>(e);

      // Apply friction (deceleration proportional to friction coefficient)
      float speed = std::sqrt(vel.vx * vel.vx + vel.vy * vel.vy);

      if (speed > 0.01f) {
        float frictionForce = body.friction * deltaTime;
        float newSpeed = speed - frictionForce;
        if (newSpeed < 0.0f) newSpeed = 0.0f;
        float scale = newSpeed / speed;
        vel.vx *= scale;
        vel.vy *= scale;
      }

      // Clamp to maxSpeed (adjusted by mass — heavier = slower accel)
      float effectiveMax = body.maxSpeed / body.mass;
      speed = std::sqrt(vel.vx * vel.vx + vel.vy * vel.vy);
      if (speed > effectiveMax) {
        float scale = effectiveMax / speed;
        vel.vx *= scale;
        vel.vy *= scale;
      }

      // Spawn dust particles on sudden direction changes or high speed
      if (speed > effectiveMax * 0.8f) {
        auto &t = registry.getComponent<Transform2D>(e);
        spawnDustParticle(registry, t.x, t.y + 20.0f);
      }
    }
  }

private:
  float dustTimer = 0.0f;

  void spawnDustParticle(Registry &registry, float x, float y) {
    dustTimer += GetFrameTime();
    if (dustTimer < 0.15f) return;
    dustTimer = 0.0f;

    Entity p = registry.createEntity();
    registry.addComponent<Transform2D>(p, x + GetRandomValue(-4, 4),
                                       y + GetRandomValue(-2, 2));
    registry.addComponent<Velocity>(p, (float)GetRandomValue(-20, 20),
                                    (float)GetRandomValue(-30, -10));
    Particle part;
    part.startColor = Color{160, 140, 120, 180};
    part.endColor = Color{80, 70, 60, 0};
    registry.addComponent<Particle>(p, part);
    registry.addComponent<Lifetime>(p, 0.3f);
  }
};
