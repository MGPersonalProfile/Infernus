#pragma once
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include <cmath>

class MovementSystem {
public:
  void update(Registry &registry, float deltaTime) {
    auto entities = registry.view<Transform2D, Velocity>();

    for (Entity entity : entities) {
      auto &transform = registry.getComponent<Transform2D>(entity);
      auto &velocity = registry.getComponent<Velocity>(entity);

      transform.x += velocity.vx * deltaTime;
      transform.y += velocity.vy * deltaTime;

      // Apply friction (decelerates velocity toward zero)
      if (velocity.friction > 0.0f) {
        float decay = 1.0f - velocity.friction * deltaTime;
        if (decay < 0.0f)
          decay = 0.0f;
        velocity.vx *= decay;
        velocity.vy *= decay;
      }
    }
  }
};
