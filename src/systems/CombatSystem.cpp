#include "CombatSystem.h"
#include "../debug/Profiler.h"
#include "../audio/AudioManager.h"
#include "../components/AIBehavior.h"
#include "../components/Animation.h"
#include "../components/BossPhase.h"
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Lifetime.h"
#include "../components/Particle.h"
#include "../components/ActiveAbility.h"
#include "../components/PlayerStats.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ResourceManager.h"
#include "../scripting/LuaEngine.h"
#include "../systems/CameraSystem.h"
#include "../systems/PartikelEmitters.h"
#include "../systems/ScreenEffects.h"
#include "../ui/UIRenderer.h"
#include "../utils/Constants.h"
#include <cmath>

// =============================================================================
// Public
// =============================================================================

void CombatSystem::update(Registry &registry, CameraSystem &cameraSystem,
                          ScreenEffects &screenEffects, float deltaTime) {
  INFERNUS_ZONE_N("CombatSystem");
  processLifetimes(registry, deltaTime);
  processAttackStates(registry, deltaTime);
  processHitDetection(registry, cameraSystem, screenEffects);
}

// =============================================================================
// Phase 1 — Lifetime Expiration
// =============================================================================

void CombatSystem::processLifetimes(Registry &registry, float deltaTime) {
  auto lifetimeView = registry.view<Lifetime>();
  for (Entity entity : lifetimeView) {
    auto &lifetime = registry.getComponent<Lifetime>(entity);
    lifetime.timeRemaining -= deltaTime;
    if (lifetime.timeRemaining <= 0.0f) {
      registry.markForDestruction(entity);
    }
  }
}

// =============================================================================
// Phase 2 — Attack State Machine
// =============================================================================

void CombatSystem::processAttackStates(Registry &registry, float deltaTime) {
  auto combatView = registry.view<Combat, Transform2D>();
  for (Entity entity : combatView) {
    auto &combat = registry.getComponent<Combat>(entity);

    if (combat.currentState == AttackState::NONE)
      continue;

    combat.stateTimer -= deltaTime;

    if (combat.currentState == AttackState::WINDUP &&
        combat.stateTimer <= 0.0f) {
      combat.currentState = AttackState::ACTIVE;
      combat.stateTimer =
          LuaEngine::getFeel("hitbox_active_time", Constants::HITBOX_ACTIVE_TIME);
      // Apply windup multiplier to recovery too if player
      if (registry.hasComponent<PlayerStats>(entity)) {
        auto &ps = registry.getComponent<PlayerStats>(entity);
        combat.stateTimer *= ps.finalWindupMultiplier;
      }
      spawnHitbox(registry, entity);

    } else if (combat.currentState == AttackState::ACTIVE &&
               combat.stateTimer <= 0.0f) {
      combat.currentState = AttackState::RECOVERY;
      combat.stateTimer =
          LuaEngine::getFeel("attack_recovery_time", Constants::ATTACK_RECOVERY_TIME);

    } else if (combat.currentState == AttackState::RECOVERY &&
               combat.stateTimer <= 0.0f) {
      combat.currentState = AttackState::NONE;
    } else if (combat.currentState == AttackState::PARRY_ACTIVE &&
               combat.stateTimer <= 0.0f) {
      // Parry window expired without blocking — punishment recovery
      combat.currentState = AttackState::PARRY_RECOVERY;
      combat.stateTimer =
          LuaEngine::getFeel("parry_recovery", Constants::PARRY_RECOVERY);
    } else if (combat.currentState == AttackState::PARRY_RECOVERY &&
               combat.stateTimer <= 0.0f) {
      combat.currentState = AttackState::NONE;
    }
  }
}

// =============================================================================
// Phase 3 — Hit Detection (Hitbox vs Hurtbox)
// =============================================================================

static bool checkAABB(const Transform2D & /*t1*/, const Collider &c1,
                      const Transform2D & /*t2*/, const Collider &c2) {
  // Use synced rects (includes offsets) rather than raw transform positions
  return CheckCollisionRecs(c1.rect, c2.rect);
}

void CombatSystem::processHitDetection(Registry &registry,
                                       CameraSystem &cameraSystem,
                                       ScreenEffects &screenEffects) {
  auto hitboxView = registry.view<Combat, Collider, Transform2D>();
  auto hurtboxView = registry.view<Health, Collider, Transform2D>();

  for (Entity attacker : hitboxView) {
    // Only process transient hitbox entities (identified by Lifetime)
    if (!registry.hasComponent<Lifetime>(attacker))
      continue;

    auto &aCombat = registry.getComponent<Combat>(attacker);
    auto &aTransform = registry.getComponent<Transform2D>(attacker);
    auto &aCollider = registry.getComponent<Collider>(attacker);

    for (Entity victim : hurtboxView) {
      if (attacker == victim)
        continue;
      if (aCombat.owner == victim)
        continue;

      auto &vHealth = registry.getComponent<Health>(victim);
      if (vHealth.isInvulnerable() || vHealth.isDead())
        continue;

      auto &vTransform = registry.getComponent<Transform2D>(victim);
      auto &vCollider = registry.getComponent<Collider>(victim);

      if (!checkAABB(aTransform, aCollider, vTransform, vCollider))
        continue;

      // === PARRY CHECK ===
      if (registry.hasComponent<Combat>(victim)) {
        auto &vCombat = registry.getComponent<Combat>(victim);
        if (vCombat.currentState == AttackState::PARRY_ACTIVE) {
          // Successful parry! Negate damage, stagger attacker, reward player
          vCombat.currentState = AttackState::NONE; // cancel parry state
          vCombat.stateTimer = 0.0f;

          // Stagger the attack owner (not the hitbox entity)
          Entity attackOwner = aCombat.owner;
          if (registry.isAlive(attackOwner) &&
              registry.hasComponent<AIBehavior>(attackOwner)) {
            auto &ai = registry.getComponent<AIBehavior>(attackOwner);
            ai.currentState = AIState::STAGGER;
            ai.stateTimer = 0.0f;
            ai.staggerDuration = Constants::PARRY_STAGGER_TIME;
          }
          // Also stagger boss if applicable
          if (registry.isAlive(attackOwner) &&
              registry.hasComponent<BossPhase>(attackOwner)) {
            auto &bp = registry.getComponent<BossPhase>(attackOwner);
            bp.patternActive = false;
            bp.transitioning = true;
            bp.transitionTimer = Constants::PARRY_STAGGER_TIME;
            if (registry.hasComponent<Velocity>(attackOwner)) {
              auto &bv = registry.getComponent<Velocity>(attackOwner);
              bv.vx = 0.0f;
              bv.vy = 0.0f;
            }
          }

          // VFX: parry flash + big shake (Lua-tunable via feel.lua)
          cameraSystem.addShake(LuaEngine::getFeel("shake_parry_intensity", 12.0f), 0.3f);
          screenEffects.addHitstop(LuaEngine::getFeel("hitstop_parry", 0.1f));
          screenEffects.addFlash(Color{255, 255, 200, 120}, 0.15f);
          AudioManager::getInstance().playSFX("boss_slam"); // impactful sound
          spawnHitParticles(registry, vTransform.x + 20.0f, vTransform.y + 20.0f);

          // Destroy the hitbox so it can't hit again
          registry.markForDestruction(attacker);
          continue;
        }
      }

      // === HIT CONFIRMED ===
      int damage = aCombat.baseDamage;
      bool isCrit = false;

      // Apply crit and lifesteal if attacker is player (owner has PlayerStats)
      Entity attackOwner = aCombat.owner;
      if (registry.isAlive(attackOwner) &&
          registry.hasComponent<PlayerStats>(attackOwner)) {
        auto &ps = registry.getComponent<PlayerStats>(attackOwner);

        // Crit chance
        if (ps.finalCritChance > 0.0f) {
          float roll = (float)GetRandomValue(0, 100) / 100.0f;
          if (roll <= ps.finalCritChance) {
            damage = (int)(damage * 2.0f);
            isCrit = true;
          }
        }

        // Lifesteal
        if (ps.finalLifesteal > 0.0f &&
            registry.hasComponent<Health>(attackOwner)) {
          int healAmt = (int)(damage * ps.finalLifesteal);
          if (healAmt > 0) {
            auto &ownerHP = registry.getComponent<Health>(attackOwner);
            ownerHP.currentHP = std::min(ownerHP.currentHP + healAmt, ownerHP.maxHP);
          }
        }

        // Active "drenar_alma" charges: heal 50% of damage, decrement
        if (registry.hasComponent<ActiveAbilities>(attackOwner) &&
            registry.hasComponent<Health>(attackOwner)) {
          auto &actives = registry.getComponent<ActiveAbilities>(attackOwner);
          int heal = damage / 2;
          if (actives.chargesQ > 0) {
            actives.chargesQ--;
            auto &ownerHP = registry.getComponent<Health>(attackOwner);
            ownerHP.currentHP = std::min(ownerHP.currentHP + heal, ownerHP.maxHP);
          } else if (actives.chargesE > 0) {
            actives.chargesE--;
            auto &ownerHP = registry.getComponent<Health>(attackOwner);
            ownerHP.currentHP = std::min(ownerHP.currentHP + heal, ownerHP.maxHP);
          }
        }
      }

      // Apply thorns if victim is player (has PlayerStats) and attacker is enemy
      if (registry.isAlive(attackOwner) &&
          registry.hasComponent<PlayerStats>(victim)) {
        auto &ps = registry.getComponent<PlayerStats>(victim);
        if (ps.finalThorns > 0.0f &&
            registry.hasComponent<Health>(attackOwner)) {
          registry.getComponent<Health>(attackOwner).currentHP -=
              (int)ps.finalThorns;
        }
      }

      // Apply elemental resistance
      float resist = vHealth.resistances.get(aCombat.damageType);
      damage = (int)(damage * resist);
      if (damage < 1) damage = 1; // always deal at least 1

      vHealth.currentHP -= damage;
      vHealth.invulnerabilityTimer = LuaEngine::getFeel("hit_iframes", Constants::HIT_IFRAMES);
      vHealth.hitFlashTimer        = LuaEngine::getFeel("hit_flash_time", Constants::HIT_FLASH_TIME);

      // Knockback — push victim away from attacker
      if (registry.hasComponent<Velocity>(victim)) {
        auto &vVel = registry.getComponent<Velocity>(victim);
        float dx = vTransform.x - aTransform.x;
        float dy = vTransform.y - aTransform.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 1.0f) { dx = 1.0f; dy = 0.0f; dist = 1.0f; }
        vVel.vx += aCombat.knockbackForce * (dx / dist);
        vVel.vy += aCombat.knockbackForce * (dy / dist);
      }

      // SFX — distinguish player vs enemy hit, and damage type for enemy hits.
      // Damage-type variants are silent if the WAV doesn't exist (AudioManager
      // lazy-loads + skips missing files). Antigravity can drop in
      // hit_fire.wav, hit_ice.wav etc. without code changes.
      if (registry.hasComponent<AIBehavior>(victim)) {
        const char *typedSfx = nullptr;
        switch (aCombat.damageType) {
          case DamageType::FIRE:      typedSfx = "hit_fire"; break;
          case DamageType::ICE:       typedSfx = "hit_ice"; break;
          case DamageType::LIGHTNING: typedSfx = "hit_lightning"; break;
          case DamageType::TOXIC:     typedSfx = "hit_toxic"; break;
          default: break;
        }
        if (typedSfx) AudioManager::getInstance().playSFX(typedSfx);
        AudioManager::getInstance().playSFX("hit_enemy"); // base layer always
      } else {
        AudioManager::getInstance().playSFX("hit_player");
      }

      // Damage number (colored by element)
      UIRenderer::spawnDamageNumber(registry, vTransform.x + 10.0f,
                                    vTransform.y, damage, isCrit,
                                    aCombat.damageType);

      // VFX (Lua-tunable via feel.lua)
      float shakeAmount = isCrit
          ? LuaEngine::getFeel("shake_crit_intensity", 10.0f)
          : LuaEngine::getFeel("shake_normal_intensity", 5.0f);
      cameraSystem.addShake(shakeAmount, isCrit ? 0.3f : 0.2f);
      spawnHitParticles(registry, vTransform.x + 20.0f, vTransform.y + 20.0f);
      spawnSlashArc(registry, vTransform.x, vTransform.y);

      // libpartikel burst — high-volume blood splatter on top of ECS particles
      PartikelEmitters::spawnBlood(vTransform.x + 16.0f, vTransform.y + 24.0f,
                                   isCrit ? 18 : 10);

      // Hitstop on heavy/crit hits
      if (isCrit || aCombat.baseDamage >= 20)
        screenEffects.addHitstop(LuaEngine::getFeel("hitstop_normal", 0.06f));
    }
  }
}

// =============================================================================
// Spawning helpers
// =============================================================================

void CombatSystem::spawnHitbox(Registry &registry, Entity owner) {
  auto &ownerCombat = registry.getComponent<Combat>(owner);
  auto &ownerTransform = registry.getComponent<Transform2D>(owner);

  // Use facing direction for 4-directional attacks
  float fx = ownerTransform.facingX;
  float fy = ownerTransform.facingY;

  // Determine dominant axis for hitbox placement
  float offsetDist = 40.0f;
  float hitW = Constants::HITBOX_WIDTH;
  float hitH = Constants::HITBOX_HEIGHT;

  if (std::abs(fy) > std::abs(fx)) {
    hitW = Constants::HITBOX_HEIGHT;
    hitH = Constants::HITBOX_WIDTH;
    fx = 0.0f;
    fy = (fy > 0.0f) ? 1.0f : -1.0f;
  } else {
    fx = (fx >= 0.0f) ? 1.0f : -1.0f;
    fy = 0.0f;
  }

  float hx = ownerTransform.x + fx * offsetDist;
  float hy = ownerTransform.y + fy * offsetDist;

  // Heavy attacks deal 1.8x damage and 1.5x knockback
  int hitDamage = ownerCombat.baseDamage;
  float hitKnockback = ownerCombat.knockbackForce;
  if (ownerCombat.lastAttackType == AttackType::HEAVY) {
    hitDamage = (int)(hitDamage * 1.8f);
    hitKnockback *= 1.5f;
  }
  // Combo finisher: 2.5x damage, 2x knockback
  if (ownerCombat.isFinisher) {
    hitDamage = (int)(ownerCombat.baseDamage * 2.5f);
    hitKnockback = ownerCombat.knockbackForce * 2.0f;
    ownerCombat.isFinisher = false;
  }

  Entity hitbox = registry.createEntity();
  registry.addComponent<Transform2D>(hitbox, hx, hy);
  registry.addComponent<Collider>(hitbox, hitW, hitH);
  auto &hitCombat = registry.addComponent<Combat>(hitbox, hitDamage, hitKnockback, owner);
  hitCombat.damageType = ownerCombat.damageType;
  registry.addComponent<Lifetime>(hitbox,
      LuaEngine::getFeel("hitbox_active_time", Constants::HITBOX_ACTIVE_TIME));

  // Attack pattern modifiers from PlayerStats (items)
  if (registry.hasComponent<PlayerStats>(owner)) {
    auto &stats = registry.getComponent<PlayerStats>(owner);

    // Multi-hit: spawn extra hitboxes offset perpendicular to attack
    if (stats.extraHitboxes > 0) {
      float perpX = -fy, perpY = fx;
      for (int i = 0; i < stats.extraHitboxes; i++) {
        float sign = (i % 2 == 0) ? 1.0f : -1.0f;
        float dist = 30.0f * ((i / 2) + 1);
        Entity extra = registry.createEntity();
        registry.addComponent<Transform2D>(extra, hx + perpX * sign * dist,
                                           hy + perpY * sign * dist);
        registry.addComponent<Collider>(extra, hitW * 0.8f, hitH * 0.8f);
        registry.addComponent<Combat>(extra, ownerCombat.baseDamage / 2,
                                      ownerCombat.knockbackForce * 0.5f, owner);
        registry.addComponent<Lifetime>(extra, Constants::HITBOX_ACTIVE_TIME);
      }
    }

    // Projectile: launch a projectile in facing direction
    if (stats.projectileAttack) {
      Entity proj = registry.createEntity();
      registry.addComponent<Transform2D>(proj, ownerTransform.x, ownerTransform.y);
      registry.addComponent<Velocity>(proj, fx * 400.0f, fy * 400.0f);
      registry.addComponent<Collider>(proj, 12.0f, 12.0f, true);
      registry.addComponent<Combat>(proj, ownerCombat.baseDamage,
                                    80.0f, owner);
      registry.addComponent<Lifetime>(proj, 1.5f);
    }

    // Area attack: spawn a larger AoE around the player
    if (stats.areaAttack) {
      Entity aoe = registry.createEntity();
      registry.addComponent<Transform2D>(aoe, ownerTransform.x - 50.0f,
                                         ownerTransform.y - 50.0f);
      registry.addComponent<Collider>(aoe, 100.0f, 100.0f);
      registry.addComponent<Combat>(aoe, ownerCombat.baseDamage / 2,
                                    100.0f, owner);
      registry.addComponent<Lifetime>(aoe, 0.1f);
    }

    // Chain attack: spawn additional projectiles in random directions
    if (stats.chainAttack && stats.chainBounces > 0) {
      for (int i = 0; i < stats.chainBounces; i++) {
        float angle = (float)GetRandomValue(0, 360) * (3.14159f / 180.0f);
        Entity chain = registry.createEntity();
        registry.addComponent<Transform2D>(chain, hx, hy);
        registry.addComponent<Velocity>(chain, cosf(angle) * 300.0f,
                                        sinf(angle) * 300.0f);
        registry.addComponent<Collider>(chain, 10.0f, 10.0f, true);
        registry.addComponent<Combat>(chain, ownerCombat.baseDamage / 3,
                                      60.0f, owner);
        registry.addComponent<Lifetime>(chain, 0.6f);
        auto &res = ResourceManager::getInstance();
        Texture2D chainTex = res.getTexture("assets/sprites/fx/hit_particle.png");
        registry.addComponent<Sprite>(chain, chainTex, Rectangle{0, 0, 4, 4}, 10);
        registry.addComponent<Particle>(chain, Color{100, 180, 255, 200},
                                        Color{50, 100, 255, 0}, 0.8f, 0.3f);
      }
    }

    // Fire trail: chance to leave a fire patch behind
    if (stats.fireTrailChance > 0.0f) {
      float roll = (float)GetRandomValue(0, 100) / 100.0f;
      if (roll <= stats.fireTrailChance) {
        Entity fire = registry.createEntity();
        registry.addComponent<Transform2D>(fire, ownerTransform.x - 16.0f,
                                           ownerTransform.y - 16.0f);
        registry.addComponent<Collider>(fire, 32.0f, 32.0f, true);
        int fireDmg = std::max(5, hitDamage / 4);
        registry.addComponent<Combat>(fire, fireDmg, 0.0f, owner);
        registry.addComponent<Lifetime>(fire, 2.0f);
        auto &res = ResourceManager::getInstance();
        Texture2D fireTex = res.getTexture("assets/sprites/fx/hit_particle.png");
        registry.addComponent<Sprite>(fire, fireTex, Rectangle{0, 0, 4, 4}, 1);
        registry.addComponent<Particle>(fire, Color{255, 100, 0, 200},
                                        Color{255, 50, 0, 0}, 2.0f, 0.5f);
      }
    }
  }
}

void CombatSystem::spawnHitParticles(Registry &registry, float x, float y) {
  auto &res = ResourceManager::getInstance();
  Texture2D bloodTex = res.getTexture("assets/sprites/particles/blood_drop.png");

  int pMin = (int)LuaEngine::getFeel("hit_particles_min", (float)Constants::HIT_PARTICLES_MIN);
  int pMax = (int)LuaEngine::getFeel("hit_particles_max", (float)Constants::HIT_PARTICLES_MAX);
  int count = GetRandomValue(pMin, pMax);
  float pLife = LuaEngine::getFeel("hit_particle_lifetime", Constants::HIT_PARTICLE_LIFETIME);
  for (int i = 0; i < count; i++) {
    Entity p = registry.createEntity();
    registry.addComponent<Transform2D>(p, x, y);
    registry.addComponent<Velocity>(p, (float)GetRandomValue(-200, 200),
                                    (float)GetRandomValue(-300, 50));
    registry.addComponent<Lifetime>(p, pLife);
    // blood_drop.png: 24x8, 3 frames of 8x8 — pick random frame
    int frame = GetRandomValue(0, 2);
    registry.addComponent<Sprite>(p, bloodTex,
                                  Rectangle{(float)(frame * 8), 0, 8, 8}, 0);
    registry.addComponent<Particle>(p, Color{220, 30, 30, 255},
                                    Color{100, 0, 0, 0}, 1.0f, 0.2f);
  }
}

void CombatSystem::spawnSlashArc(Registry &registry, float x, float y) {
  auto &res = ResourceManager::getInstance();
  Texture2D tex = res.getTexture("assets/sprites/fx/slash_arc.png");

  Entity arc = registry.createEntity();
  registry.addComponent<Transform2D>(arc, x - 8.0f, y - 8.0f);
  registry.addComponent<Sprite>(arc, tex, Rectangle{0, 0, 32, 32}, 11);
  registry.addComponent<Lifetime>(arc, 0.15f);
  registry.addComponent<Animation>(arc, 3, 0.05f, 32.0f, 32.0f);
  auto &anim = registry.getComponent<Animation>(arc);
  anim.loop = false;
}
