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
#include "../world/RoomGenerator.h"
#include <unordered_map>
#include <utility>
#include <vector>

class AISystem {
public:
  void update(Registry &registry, float deltaTime);

  // Set the active room for pathfinding. Call when a new room spawns.
  void setRoom(const RoomTemplate *room) { currentRoom = room; }

private:
  // Path cache per entity: list of next tile waypoints + ttl
  struct PathInfo {
    std::vector<std::pair<int, int>> tiles;
    int currentWaypoint = 0;
    float ttl = 0.0f;
  };
  std::unordered_map<Entity, PathInfo> pathCache;
  const RoomTemplate *currentRoom = nullptr;

  void handleIdle(AIBehavior &ai, Velocity &velocity, float distToPlayer,
                  float deltaTime);
  void handlePatrol(AIBehavior &ai, Transform2D &transform, Velocity &velocity,
                    float distToPlayer, float deltaTime);
  void handleChase(Registry &registry, Entity entity, AIBehavior &ai,
                   Transform2D &transform, Velocity &velocity, float dirX,
                   float dirY, float distToPlayer, float deltaTime);
  void handleAttack(Registry &registry, Entity entity, AIBehavior &ai,
                    Transform2D &transform, Velocity &velocity, float dirX,
                    float dirY, float deltaTime);
  void handleStagger(AIBehavior &ai, Velocity &velocity, float deltaTime);
};
