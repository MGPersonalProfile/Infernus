// HealthSystem — Health Management, Death Processing, and Loot Drops
//
// Per-frame processing:
//   1. Counts down invulnerability timer (i-frames)
//   2. Counts down hit flash timer (visual feedback)
//   3. Triggers stagger on AI enemies when damaged
//   4. Handles death: spawns loot orbs, death particles, marks entity for
//      destruction

#pragma once

#include "../core/ECS.h"

struct Loot; // forward declaration

class HealthSystem {
public:
  void update(Registry &registry, float deltaTime);

private:
  void spawnLootOrb(Registry &registry, float x, float y, const Loot &loot);
  void spawnItemPickup(Registry &registry, float x, float y);
  void spawnDeathParticles(Registry &registry, float x, float y);
};
