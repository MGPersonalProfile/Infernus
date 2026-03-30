// AIBehavior — Enemy AI State Machine Component
//
// State Machine:
//   IDLE -> PATROL -> CHASE -> ATTACK -> (back to CHASE)
//   Any state + receive damage -> STAGGER -> CHASE
//   HP <= 0 -> DEATH

#pragma once
#include "../core/ECS.h"

enum class AIState { IDLE, PATROL, CHASE, ATTACK, STAGGER, DEATH };

enum class EnemyType { MELEE, RANGED, TANK };

struct AIBehavior : public Component {
  // --- State ---
  AIState currentState = AIState::IDLE;
  float stateTimer = 0.0f;

  // --- Enemy Archetype ---
  EnemyType enemyType = EnemyType::MELEE;

  // --- Detection Ranges ---
  float aggroRange = 200.0f;
  float attackRange = 50.0f;

  // --- Movement Speeds ---
  float patrolSpeed = 60.0f;
  float chaseSpeed = 120.0f;

  // --- Patrol ---
  float patrolDirection = 1.0f;
  float patrolTimer = 0.0f;
  float patrolDuration = 2.0f;

  // --- Ranged-Specific ---
  float retreatRange = 0.0f;
  float projectileSpeed = 300.0f;
  float projectileLifetime = 2.0f;
  float attackCooldown = 0.0f;
  float attackCooldownMax = 1.5f;

  // --- Attack ---
  float attackWindup = 0.4f;

  // --- Target ---
  Entity targetEntity = NULL_ENTITY;

  AIBehavior() = default;
};
