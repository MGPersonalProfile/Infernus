#include "BossAISystem.h"
#include "../audio/AudioManager.h"
#include "../components/BossPhase.h"
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Lifetime.h"
#include "../components/Particle.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ResourceManager.h"
#include "../systems/CameraSystem.h"
#include "../utils/Constants.h"
#include "raylib.h"
#include <cmath>

void BossAISystem::update(Registry &registry, CameraSystem &cameraSystem,
                          float deltaTime, Entity playerEntity) {
  auto bossView = registry.view<BossPhase, Transform2D, Health, Combat>();

  for (Entity boss : bossView) {
    auto &bp = registry.getComponent<BossPhase>(boss);
    auto &transform = registry.getComponent<Transform2D>(boss);
    auto &health = registry.getComponent<Health>(boss);
    (void)transform; // used indirectly by sub-functions

    if (health.isDead())
      continue;

    // --- Phase transition ---
    if (bp.transitioning) {
      bp.transitionTimer -= deltaTime;
      if (registry.hasComponent<Velocity>(boss)) {
        auto &vel = registry.getComponent<Velocity>(boss);
        vel.vx = 0.0f;
        vel.vy = 0.0f;
      }
      if (bp.transitionTimer <= 0.0f) {
        bp.transitioning = false;
        bp.patternActive = false;
        bp.isCharging = false;
      }
      continue;
    }

    checkPhaseTransition(registry, boss, cameraSystem);

    // --- Pattern execution ---
    if (!bp.patternActive) {
      bp.patternTimer -= deltaTime;
      if (bp.patternTimer <= 0.0f) {
        selectPattern(registry, boss);
      }
      // Face the player while idle
      if (registry.hasComponent<Transform2D>(playerEntity) &&
          registry.hasComponent<Sprite>(boss)) {
        auto &pt = registry.getComponent<Transform2D>(playerEntity);
        registry.getComponent<Sprite>(boss).flipX = (pt.x < transform.x);
      }
      return;
    }

    switch (bp.currentPattern) {
    case BossPattern::CHARGE:
    case BossPattern::ENRAGED_CHARGE:
      executeCharge(registry, boss, cameraSystem, deltaTime, playerEntity);
      break;
    case BossPattern::GROUND_SLAM:
      executeGroundSlam(registry, boss, cameraSystem, deltaTime);
      break;
    case BossPattern::STOMP:
      executeStomp(registry, boss, cameraSystem, deltaTime);
      break;
    case BossPattern::COMBO:
      // Combo: rapid sequence of ground slams
      executeGroundSlam(registry, boss, cameraSystem, deltaTime);
      break;
    }
  }
}

void BossAISystem::selectPattern(Registry &registry, Entity boss) {
  auto &bp = registry.getComponent<BossPhase>(boss);
  auto &phase = bp.current();

  // Pick a random pattern from the current phase's available patterns
  int idx = GetRandomValue(0, (int)phase.patterns.size() - 1);
  const std::string &pat = phase.patterns[idx];

  if (pat == "charge")
    bp.currentPattern = BossPattern::CHARGE;
  else if (pat == "ground_slam")
    bp.currentPattern = BossPattern::GROUND_SLAM;
  else if (pat == "stomp")
    bp.currentPattern = BossPattern::STOMP;
  else if (pat == "combo")
    bp.currentPattern = BossPattern::COMBO;
  else if (pat == "enraged_charge")
    bp.currentPattern = BossPattern::ENRAGED_CHARGE;
  else
    bp.currentPattern = BossPattern::CHARGE;

  bp.patternActive = true;
  bp.patternStep = 0;
  bp.patternTimer = 0.0f;
  bp.isCharging = false;
  bp.comboCount = 0;

  // Boss attack SFX
  if (bp.currentPattern == BossPattern::CHARGE || bp.currentPattern == BossPattern::ENRAGED_CHARGE)
    AudioManager::getInstance().playSFX("boss_charge");
  else if (bp.currentPattern == BossPattern::GROUND_SLAM)
    AudioManager::getInstance().playSFX("boss_slam");
  else
    AudioManager::getInstance().playSFX("boss_roar");

  // Apply phase multipliers to combat
  auto &combat = registry.getComponent<Combat>(boss);
  combat.stateTimer = phase.attackWindup;
  combat.currentState = AttackState::WINDUP;
}

// =============================================================================
// CHARGE — Telegraph, then rush toward player position
// =============================================================================
void BossAISystem::executeCharge(Registry &registry, Entity boss,
                                 CameraSystem &cam, float deltaTime,
                                 Entity player) {
  auto &bp = registry.getComponent<BossPhase>(boss);
  auto &transform = registry.getComponent<Transform2D>(boss);
  auto &combat = registry.getComponent<Combat>(boss);
  auto &vel = registry.getComponent<Velocity>(boss);

  float speed =
      bp.chargeSpeed * bp.current().speedMultiplier;
  if (bp.currentPattern == BossPattern::ENRAGED_CHARGE)
    speed *= 1.5f;

  // Step 0: Windup (telegraph)
  if (bp.patternStep == 0) {
    vel.vx = 0.0f;
    vel.vy = 0.0f;
    combat.stateTimer -= deltaTime;

    if (combat.stateTimer <= 0.0f) {
      // Lock direction toward player
      if (registry.hasComponent<Transform2D>(player)) {
        auto &pt = registry.getComponent<Transform2D>(player);
        float dx = pt.x - transform.x;
        float dy = pt.y - transform.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > 0.0f) {
          bp.chargeDirX = dx / dist;
          bp.chargeDirY = dy / dist;
        }
      }
      bp.isCharging = true;
      bp.chargeTimer = 0.6f; // charge duration
      bp.patternStep = 1;
      combat.currentState = AttackState::ACTIVE;
    }
    return;
  }

  // Step 1: Charging — spawn damage hitbox at boss position
  if (bp.patternStep == 1) {
    vel.vx = bp.chargeDirX * speed;
    vel.vy = bp.chargeDirY * speed;
    bp.chargeTimer -= deltaTime;

    // Spawn charge hitbox (moves with boss)
    {
      int chargeDmg = (int)(combat.baseDamage * bp.current().damageMultiplier);
      Entity chargeHit = registry.createEntity();
      registry.addComponent<Transform2D>(chargeHit, transform.x - 30.0f,
                                         transform.y - 30.0f);
      registry.addComponent<Collider>(chargeHit, 60.0f, 60.0f);
      registry.addComponent<Combat>(chargeHit, chargeDmg, 300.0f, boss);
      registry.addComponent<Lifetime>(chargeHit, deltaTime * 2.0f);
    }

    // Flip sprite
    if (registry.hasComponent<Sprite>(boss))
      registry.getComponent<Sprite>(boss).flipX = (vel.vx < 0.0f);

    if (bp.chargeTimer <= 0.0f) {
      // Charge done — recovery
      vel.vx = 0.0f;
      vel.vy = 0.0f;
      bp.isCharging = false;
      bp.patternStep = 2;
      bp.patternTimer = 0.4f; // recovery time
      combat.currentState = AttackState::RECOVERY;
      cam.addShake(8.0f, 0.3f);
    }
    return;
  }

  // Step 2: Recovery
  if (bp.patternStep == 2) {
    bp.patternTimer -= deltaTime;
    if (bp.patternTimer <= 0.0f) {
      bp.patternActive = false;
      bp.patternTimer = bp.current().attackCooldown;
      combat.currentState = AttackState::NONE;
    }
  }
}

// =============================================================================
// GROUND SLAM — AoE damage around the boss
// =============================================================================
void BossAISystem::executeGroundSlam(Registry &registry, Entity boss,
                                     CameraSystem &cam, float deltaTime) {
  auto &bp = registry.getComponent<BossPhase>(boss);
  auto &transform = registry.getComponent<Transform2D>(boss);
  auto &combat = registry.getComponent<Combat>(boss);
  auto &vel = registry.getComponent<Velocity>(boss);
  vel.vx = 0.0f;
  vel.vy = 0.0f;

  // Step 0: Windup
  if (bp.patternStep == 0) {
    combat.stateTimer -= deltaTime;
    if (combat.stateTimer <= 0.0f) {
      bp.patternStep = 1;
      bp.patternTimer = 0.15f;
      combat.currentState = AttackState::ACTIVE;

      int dmg = (int)(combat.baseDamage * bp.current().damageMultiplier);
      spawnShockwave(registry, transform.x, transform.y, 80.0f, dmg, boss);
      cam.addShake(12.0f, 0.4f);
    }
    return;
  }

  // Step 1: Active
  if (bp.patternStep == 1) {
    bp.patternTimer -= deltaTime;
    if (bp.patternTimer <= 0.0f) {
      bp.patternStep = 2;
      bp.patternTimer = 0.5f;
      combat.currentState = AttackState::RECOVERY;

      // Combo: repeat slam up to 3 times
      if (bp.currentPattern == BossPattern::COMBO && bp.comboCount < 3) {
        bp.comboCount++;
        bp.patternStep = 0;
        combat.stateTimer = 0.2f;
        combat.currentState = AttackState::WINDUP;
      }
    }
    return;
  }

  // Step 2: Recovery
  bp.patternTimer -= deltaTime;
  if (bp.patternTimer <= 0.0f) {
    bp.patternActive = false;
    bp.patternTimer = bp.current().attackCooldown;
    combat.currentState = AttackState::NONE;
  }
}

// =============================================================================
// STOMP — Outward shockwave (larger radius)
// =============================================================================
void BossAISystem::executeStomp(Registry &registry, Entity boss,
                                CameraSystem &cam, float deltaTime) {
  auto &bp = registry.getComponent<BossPhase>(boss);
  auto &transform = registry.getComponent<Transform2D>(boss);
  auto &combat = registry.getComponent<Combat>(boss);
  auto &vel = registry.getComponent<Velocity>(boss);
  vel.vx = 0.0f;
  vel.vy = 0.0f;

  if (bp.patternStep == 0) {
    combat.stateTimer -= deltaTime;
    if (combat.stateTimer <= 0.0f) {
      bp.patternStep = 1;
      bp.patternTimer = 0.2f;
      combat.currentState = AttackState::ACTIVE;

      int dmg = (int)(combat.baseDamage * bp.current().damageMultiplier * 0.7f);
      spawnShockwave(registry, transform.x, transform.y, 150.0f, dmg, boss);
      cam.addShake(15.0f, 0.5f);
    }
    return;
  }

  if (bp.patternStep == 1) {
    bp.patternTimer -= deltaTime;
    if (bp.patternTimer <= 0.0f) {
      bp.patternStep = 2;
      bp.patternTimer = 0.8f;
      combat.currentState = AttackState::RECOVERY;
    }
    return;
  }

  bp.patternTimer -= deltaTime;
  if (bp.patternTimer <= 0.0f) {
    bp.patternActive = false;
    bp.patternTimer = bp.current().attackCooldown;
    combat.currentState = AttackState::NONE;
  }
}

// =============================================================================
// Phase Transitions
// =============================================================================
void BossAISystem::checkPhaseTransition(Registry &registry, Entity boss,
                                        CameraSystem &cam) {
  auto &bp = registry.getComponent<BossPhase>(boss);
  auto &health = registry.getComponent<Health>(boss);

  float ratio = (float)health.currentHP / (float)health.maxHP;
  if (bp.shouldTransition(ratio)) {
    bp.currentPhase++;
    bp.transitioning = true;
    bp.transitionTimer = 1.0f;
    bp.patternActive = false;
    bp.isCharging = false;

    // Visual + audio feedback
    cam.addShake(20.0f, 0.8f);
    health.invulnerabilityTimer = 1.0f;
    AudioManager::getInstance().playSFX("phase_transition");

    if (bp.currentPhase >= bp.totalPhases - 1)
      bp.enraged = true;

    // Spawn transition particles
    auto &t = registry.getComponent<Transform2D>(boss);
    auto &res = ResourceManager::getInstance();
    Texture2D tex = res.getTexture("assets/sprites/fx/ember.png");
    for (int i = 0; i < 30; i++) {
      Entity p = registry.createEntity();
      registry.addComponent<Transform2D>(p, t.x + 40.0f, t.y + 40.0f);
      registry.addComponent<Velocity>(p, (float)GetRandomValue(-500, 500),
                                      (float)GetRandomValue(-500, 500));
      registry.addComponent<Lifetime>(p, 0.6f);
      registry.addComponent<Sprite>(p, tex, Rectangle{0, 0, 4, 4}, 12);
      registry.addComponent<Particle>(p, ORANGE, Color{255, 0, 0, 0}, 2.0f,
                                      0.1f);
    }
  }
}

// =============================================================================
// Shockwave Hitbox
// =============================================================================
void BossAISystem::spawnShockwave(Registry &registry, float x, float y,
                                  float radius, int damage, Entity owner) {
  Entity sw = registry.createEntity();
  registry.addComponent<Transform2D>(sw, x - radius / 2.0f,
                                     y - radius / 2.0f);
  registry.addComponent<Collider>(sw, radius, radius);
  registry.addComponent<Combat>(sw, damage, 300.0f, owner);
  registry.addComponent<Lifetime>(sw, 0.15f);

  // Visual: use ember particle scaled up
  auto &res = ResourceManager::getInstance();
  Texture2D tex = res.getTexture("assets/sprites/fx/ember.png");
  auto &sprite = registry.addComponent<Sprite>(sw, tex,
                                Rectangle{0, 0, 4, 4}, 2);
  (void)sprite;
  // Scale the shockwave entity so the small texture fills the area
  auto &swTransform = registry.getComponent<Transform2D>(sw);
  swTransform.scale = radius / 4.0f;
}

void BossAISystem::setBossVel(Registry &registry, Entity boss, float vx,
                              float vy) {
  if (registry.hasComponent<Velocity>(boss)) {
    auto &vel = registry.getComponent<Velocity>(boss);
    vel.vx = vx;
    vel.vy = vy;
  }
}
