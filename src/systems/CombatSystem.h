// CombatSystem — Attack State Machine, Hit Detection, and VFX Spawning
//
// Handles the full attack lifecycle:
//   1. Lifetime management for transient entities (hitboxes, particles)
//   2. Attack state machine: NONE -> WINDUP -> ACTIVE -> RECOVERY -> NONE
//   3. Hitbox vs Hurtbox collision detection (AABB-based)
//   4. Damage application, knockback, i-frames, and VFX (shake + particles)

#pragma once

#include "../core/ECS.h"

class CameraSystem;    // forward declaration
class ScreenEffects;   // forward declaration

class CombatSystem {
public:
  void update(Registry &registry, CameraSystem &cameraSystem,
              ScreenEffects &screenEffects, float deltaTime);

private:
  void processLifetimes(Registry &registry, float deltaTime);
  void processAttackStates(Registry &registry, float deltaTime);
  void processHitDetection(Registry &registry, CameraSystem &cameraSystem,
                           ScreenEffects &screenEffects);

  void spawnHitbox(Registry &registry, Entity owner);
  void spawnHitParticles(Registry &registry, float x, float y);
  void spawnSlashArc(Registry &registry, float x, float y);
};
