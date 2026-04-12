#include "AISystem.h"
#include "../debug/Profiler.h"
#include "../components/Combat.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../entities/ProjectileFactory.h"
#include "../utils/Constants.h"
#include <cmath>

// =============================================================================
// Public
// =============================================================================

void AISystem::update(Registry &registry, float deltaTime) {
  INFERNUS_ZONE_N("AISystem");
  auto aiView = registry.view<AIBehavior, Transform2D, Velocity>();

  for (Entity entity : aiView) {
    auto &ai = registry.getComponent<AIBehavior>(entity);
    auto &transform = registry.getComponent<Transform2D>(entity);
    auto &velocity = registry.getComponent<Velocity>(entity);

    if (!registry.hasComponent<Transform2D>(ai.targetEntity))
      continue;

    auto &playerTransform =
        registry.getComponent<Transform2D>(ai.targetEntity);

    float dx = playerTransform.x - transform.x;
    float dy = playerTransform.y - transform.y;
    float distToPlayer = sqrtf(dx * dx + dy * dy);

    float dirX = 0.0f, dirY = 0.0f;
    if (distToPlayer > 0.0f) {
      dirX = dx / distToPlayer;
      dirY = dy / distToPlayer;
    }

    if (ai.attackCooldown > 0.0f)
      ai.attackCooldown -= deltaTime;

    switch (ai.currentState) {
    case AIState::IDLE:
      handleIdle(ai, velocity, distToPlayer, deltaTime);
      break;
    case AIState::PATROL:
      handlePatrol(ai, transform, velocity, distToPlayer, deltaTime);
      break;
    case AIState::CHASE:
      handleChase(registry, entity, ai, velocity, dirX, dirY, distToPlayer);
      break;
    case AIState::ATTACK:
      handleAttack(registry, entity, ai, transform, velocity, dirX, dirY,
                   deltaTime);
      break;
    case AIState::STAGGER:
      handleStagger(ai, velocity, deltaTime);
      break;
    case AIState::DEATH:
      velocity.vx = 0.0f;
      velocity.vy = 0.0f;
      break;
    }

    // Flip sprite to face player or movement direction
    if (registry.hasComponent<Sprite>(entity)) {
      auto &sprite = registry.getComponent<Sprite>(entity);
      if (ai.currentState == AIState::CHASE ||
          ai.currentState == AIState::ATTACK) {
        sprite.flipX = (dx < 0.0f);
      } else if (ai.currentState == AIState::PATROL) {
        sprite.flipX = (ai.patrolDirection < 0.0f);
      }
    }
  }
}

// =============================================================================
// State Handlers
// =============================================================================

void AISystem::handleIdle(AIBehavior &ai, Velocity &velocity,
                          float distToPlayer, float deltaTime) {
  velocity.vx = 0.0f;
  velocity.vy = 0.0f;
  ai.stateTimer += deltaTime;

  if (distToPlayer <= ai.aggroRange) {
    ai.currentState = AIState::CHASE;
    ai.stateTimer = 0.0f;
    return;
  }

  if (ai.stateTimer >= Constants::IDLE_DURATION) {
    ai.currentState = AIState::PATROL;
    ai.stateTimer = 0.0f;
    ai.patrolTimer = 0.0f;
  }
}

void AISystem::handlePatrol(AIBehavior &ai, Transform2D & /*transform*/,
                            Velocity &velocity, float distToPlayer,
                            float deltaTime) {
  velocity.vx = ai.patrolSpeed * ai.patrolDirection;
  velocity.vy = 0.0f;

  ai.patrolTimer += deltaTime;
  if (ai.patrolTimer >= ai.patrolDuration) {
    ai.patrolDirection *= -1.0f;
    ai.patrolTimer = 0.0f;
  }

  if (distToPlayer <= ai.aggroRange) {
    ai.currentState = AIState::CHASE;
    ai.stateTimer = 0.0f;
  }
}

void AISystem::handleChase(Registry &registry, Entity entity, AIBehavior &ai,
                           Velocity &velocity, float dirX, float dirY,
                           float distToPlayer) {
  // Ranged enemies retreat if player gets too close
  if (ai.enemyType == EnemyType::RANGED && ai.retreatRange > 0.0f &&
      distToPlayer < ai.retreatRange) {
    velocity.vx = -dirX * ai.chaseSpeed;
    velocity.vy = -dirY * ai.chaseSpeed;
  } else if (distToPlayer > ai.attackRange) {
    velocity.vx = dirX * ai.chaseSpeed;
    velocity.vy = dirY * ai.chaseSpeed;
  } else {
    // In attack range
    velocity.vx = 0.0f;
    velocity.vy = 0.0f;

    if (ai.attackCooldown <= 0.0f && registry.hasComponent<Combat>(entity)) {
      auto &combat = registry.getComponent<Combat>(entity);
      combat.currentState = AttackState::WINDUP;
      combat.stateTimer = ai.attackWindup;
      ai.currentState = AIState::ATTACK;
      ai.stateTimer = 0.0f;
    }
  }

  // Leash: return to patrol if player runs far enough away
  if (distToPlayer > ai.aggroRange * Constants::AGGRO_LEASH_MULTIPLIER) {
    ai.currentState = AIState::PATROL;
    ai.stateTimer = 0.0f;
    ai.patrolTimer = 0.0f;
    velocity.vx = 0.0f;
    velocity.vy = 0.0f;
  }
}

void AISystem::handleAttack(Registry &registry, Entity entity, AIBehavior &ai,
                            Transform2D &transform, Velocity &velocity,
                            float dirX, float dirY, float deltaTime) {
  velocity.vx = 0.0f;
  velocity.vy = 0.0f;
  ai.stateTimer += deltaTime;

  if (!registry.hasComponent<Combat>(entity))
    return;

  auto &combat = registry.getComponent<Combat>(entity);

  // Attack sequence complete — return to chase
  if (combat.currentState == AttackState::NONE) {
    ai.currentState = AIState::CHASE;
    ai.stateTimer = 0.0f;
    ai.attackCooldown = ai.attackCooldownMax;
  }

  // Ranged enemies: spawn projectile when entering ACTIVE state
  if (ai.enemyType == EnemyType::RANGED &&
      combat.currentState == AttackState::ACTIVE &&
      ai.stateTimer < deltaTime * 2.0f) {
    ProjectileFactory::create(registry, entity, transform.x,
                              transform.y + 15.0f, dirX, dirY,
                              ai.projectileSpeed, combat.baseDamage,
                              combat.knockbackForce, ai.projectileLifetime);
  }
}

void AISystem::handleStagger(AIBehavior &ai, Velocity &velocity,
                             float deltaTime) {
  velocity.vx = 0.0f;
  velocity.vy = 0.0f;
  ai.stateTimer += deltaTime;

  if (ai.stateTimer >= Constants::STAGGER_DURATION) {
    ai.currentState = AIState::CHASE;
    ai.stateTimer = 0.0f;
  }
}
