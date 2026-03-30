#include "../src/components/Health.h"
#include "../src/components/Stamina.h"
#include "../src/core/ECS.h"
#include "../src/systems/HealthSystem.h"
#include "../src/systems/StaminaSystem.h"
#include "raylib.h"
#include <cassert>
#include <iostream>

void testHealthAndIFrames() {
  Registry registry;
  HealthSystem healthSystem;

  Entity player = registry.createEntity();
  registry.addComponent<Health>(player, 100);

  auto &health = registry.getComponent<Health>(player);
  assert(health.currentHP == 100);

  // Simulate damage
  health.currentHP -= 20;
  health.invulnerabilityTimer = 0.5f;
  assert(health.currentHP == 80);
  assert(health.isInvulnerable());

  // Partial tick — still invulnerable
  healthSystem.update(registry, 0.3f);
  registry.flushDestroyed();
  assert(health.isInvulnerable());

  // Full tick — i-frames expired
  healthSystem.update(registry, 0.3f);
  registry.flushDestroyed();
  assert(!health.isInvulnerable());

  std::cout << "[OK] Health & I-Frames\n";
}

void testDeath() {
  Registry registry;
  HealthSystem healthSystem;

  Entity e = registry.createEntity();
  registry.addComponent<Health>(e, 100);

  registry.getComponent<Health>(e).currentHP = 0;
  healthSystem.update(registry, 0.016f);
  registry.flushDestroyed();

  assert(!registry.isAlive(e) && "Entity should be destroyed at 0 HP");

  std::cout << "[OK] Death destruction\n";
}

void testStaminaRegen() {
  Registry registry;
  StaminaSystem staminaSystem;

  Entity e = registry.createEntity();
  registry.addComponent<Stamina>(e, 100.0f, 20.0f, 1.0f);

  auto &stam = registry.getComponent<Stamina>(e);
  stam.currentStamina -= 30.0f;
  stam.cooldownTimer = stam.regenDelay;
  assert(stam.currentStamina == 70.0f);

  // During cooldown — no regen
  staminaSystem.update(registry, 0.5f);
  assert(stam.currentStamina == 70.0f);

  // After cooldown — should regen
  staminaSystem.update(registry, 0.6f);
  assert(stam.currentStamina > 70.0f);

  // Fill up
  staminaSystem.update(registry, 5.0f);
  assert(stam.currentStamina == 100.0f);

  std::cout << "[OK] Stamina regen\n";
}

int main() {
  // HealthSystem spawns particles on death, which requires Raylib init
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(1, 1, "test");

  std::cout << "--- Combat Systems Unit Tests ---\n";
  testHealthAndIFrames();
  testDeath();
  testStaminaRegen();
  std::cout << "--- All Combat Tests Passed ---\n";

  CloseWindow();
  return 0;
}
