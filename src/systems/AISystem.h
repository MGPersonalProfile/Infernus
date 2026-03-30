// AISystem — Enemy AI State Machine
//
// Processes all entities with AIBehavior + Transform every frame.
// State machine: IDLE -> PATROL -> CHASE -> ATTACK -> (back to CHASE)
//                Any state + damage -> STAGGER -> CHASE
//                HP <= 0 -> DEATH
//
// Enemy types (MELEE, RANGED, TANK) have distinct behavior variants.

#pragma once

#include "../components/AIBehavior.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"

class AISystem {
public:
  void update(Registry &registry, float deltaTime);

private:
  void handleIdle(AIBehavior &ai, Velocity &velocity, float distToPlayer,
                  float deltaTime);
  void handlePatrol(AIBehavior &ai, Transform2D &transform, Velocity &velocity,
                    float distToPlayer, float deltaTime);
  void handleChase(Registry &registry, Entity entity, AIBehavior &ai,
                   Velocity &velocity, float dirX, float dirY,
                   float distToPlayer);
  void handleAttack(Registry &registry, Entity entity, AIBehavior &ai,
                    Transform2D &transform, Velocity &velocity, float dirX,
                    float dirY, float deltaTime);
  void handleStagger(AIBehavior &ai, Velocity &velocity, float deltaTime);
};
