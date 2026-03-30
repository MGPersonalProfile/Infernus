#pragma once
#include "../core/ECS.h"

// ----------------------------------------------------------------------------
// Stamina Component
// ----------------------------------------------------------------------------
// Manages an entity's physical energy for actions like attacking or dodging.
struct Stamina : public Component {
  float maxStamina = 100.0f;
  float currentStamina = 100.0f;

  // How much stamina is regenerated per second when allowed
  float regenRate = 20.0f;

  // Delay in seconds before stamina starts regenerating after consumption
  float regenDelay = 1.0f;

  // Current timer for the delay. Starts at regenDelay when logic triggers it,
  // counts down to 0.
  float cooldownTimer = 0.0f;

  Stamina() = default;
  Stamina(float max, float regen, float delay)
      : maxStamina(max), currentStamina(max), regenRate(regen),
        regenDelay(delay) {}

  bool hasEnough(float amount) const { return currentStamina >= amount; }
  bool canRegen() const { return cooldownTimer <= 0.0f; }
};
