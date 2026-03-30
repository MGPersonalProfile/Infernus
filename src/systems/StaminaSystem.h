#pragma once
#include "../components/Stamina.h"
#include "../core/ECS.h"

class StaminaSystem {
public:
  void update(Registry &registry, float deltaTime) {
    auto view = registry.view<Stamina>();

    for (Entity entity : view) {
      auto &stam = registry.getComponent<Stamina>(entity);

      if (stam.cooldownTimer > 0.0f) {
        stam.cooldownTimer -= deltaTime;
        if (stam.cooldownTimer < 0.0f)
          stam.cooldownTimer = 0.0f;
      }

      if (stam.canRegen() && stam.currentStamina < stam.maxStamina) {
        stam.currentStamina += stam.regenRate * deltaTime;
        if (stam.currentStamina > stam.maxStamina)
          stam.currentStamina = stam.maxStamina;
      }
    }
  }
};
