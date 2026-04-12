#include "HealthSystem.h"
#include "../debug/Profiler.h"
#include "../audio/AudioManager.h"
#include "../components/AIBehavior.h"
#include "../components/Collider.h"
#include "../components/Destructible.h"
#include "../components/Health.h"
#include "../components/ItemPickup.h"
#include "../components/Lifetime.h"
#include "../components/Loot.h"
#include "../components/Particle.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ResourceManager.h"
#include "../utils/Constants.h"
#include "raylib.h"

void HealthSystem::update(Registry &registry, float deltaTime) {
  INFERNUS_ZONE_N("HealthSystem");
  auto view = registry.view<Health>();

  for (Entity entity : view) {
    auto &health = registry.getComponent<Health>(entity);

    // --- Decrease invulnerability timer ---
    if (health.invulnerabilityTimer > 0.0f) {
      health.invulnerabilityTimer -= deltaTime;
      if (health.invulnerabilityTimer < 0.0f)
        health.invulnerabilityTimer = 0.0f;
    }

    // --- Decrease hit flash timer + trigger stagger ---
    if (health.hitFlashTimer > 0.0f) {
      health.hitFlashTimer -= deltaTime;
      if (health.hitFlashTimer < 0.0f)
        health.hitFlashTimer = 0.0f;

      if (registry.hasComponent<AIBehavior>(entity)) {
        auto &ai = registry.getComponent<AIBehavior>(entity);
        if (ai.currentState != AIState::STAGGER &&
            ai.currentState != AIState::DEATH) {
          ai.currentState = AIState::STAGGER;
          ai.stateTimer = 0.0f;
        }
      }
    }

    // --- Death ---
    if (health.isDead()) {
      float deathX = 0.0f, deathY = 0.0f;
      if (registry.hasComponent<Transform2D>(entity)) {
        auto &t = registry.getComponent<Transform2D>(entity);
        deathX = t.x;
        deathY = t.y;
      }

      // Loot drops
      if (registry.hasComponent<Loot>(entity)) {
        auto &loot = registry.getComponent<Loot>(entity);
        float roll = (float)GetRandomValue(0, 100) / 100.0f;
        if (roll <= loot.dropChance) {
          if (loot.type == LootType::ITEM_DROP) {
            spawnItemPickup(registry, deathX, deathY);
          } else {
            spawnLootOrb(registry, deathX, deathY, loot);
          }
        }
      }

      bool isDestructible = registry.hasComponent<Destructible>(entity);
      spawnDeathParticles(registry, deathX + 20.0f, deathY + 20.0f);
      if (!isDestructible)
        AudioManager::getInstance().playSFX("enemy_death");
      registry.markForDestruction(entity);
    }
  }
}

void HealthSystem::spawnLootOrb(Registry &registry, float x, float y,
                                const Loot &loot) {
  auto &res = ResourceManager::getInstance();

  std::string orbSprite = (loot.type == LootType::HEALTH_ORB)
                              ? "assets/sprites/fx/orb_health.png"
                              : "assets/sprites/fx/orb_stamina.png";
  Texture2D tex = res.getTexture(orbSprite);
  float orbW = 16.0f, orbH = 16.0f;

  Entity orb = registry.createEntity();
  registry.addComponent<Transform2D>(orb, x, y);
  registry.addComponent<Sprite>(orb, tex,
                                Rectangle{0, 0, orbW, orbH}, 3);
  registry.addComponent<Collider>(orb, orbW, orbH, true);
  registry.addComponent<Velocity>(orb, 0.0f, Constants::LOOT_FLOAT_SPEED);
  registry.addComponent<Loot>(orb, loot.type, 1.0f, loot.value);
  registry.addComponent<Lifetime>(orb, Constants::LOOT_LIFETIME);
}

void HealthSystem::spawnItemPickup(Registry &registry, float x, float y) {
  auto &res = ResourceManager::getInstance();
  Texture2D tex = res.getTexture("assets/sprites/fx/item_pickup.png");

  Entity pickup = registry.createEntity();
  registry.addComponent<Transform2D>(pickup, x, y);
  registry.addComponent<Sprite>(pickup, tex, Rectangle{0, 0, 16.0f, 16.0f}, 4);
  registry.addComponent<Collider>(pickup, 16.0f, 16.0f, true);
  registry.addComponent<ItemPickup>(pickup);
  registry.addComponent<Lifetime>(pickup, 15.0f);
}

void HealthSystem::spawnDeathParticles(Registry &registry, float x, float y) {
  auto &res = ResourceManager::getInstance();
  Texture2D tex = res.getTexture("assets/sprites/fx/hit_particle.png");

  int count = GetRandomValue(Constants::DEATH_PARTICLES_MIN,
                             Constants::DEATH_PARTICLES_MAX);
  for (int i = 0; i < count; i++) {
    Entity p = registry.createEntity();
    registry.addComponent<Transform2D>(p, x, y);
    registry.addComponent<Velocity>(p, (float)GetRandomValue(-400, 400),
                                    (float)GetRandomValue(-400, 400));
    registry.addComponent<Lifetime>(p, Constants::DEATH_PARTICLE_LIFETIME);
    registry.addComponent<Sprite>(p, tex, Rectangle{0, 0, 4, 4}, 0);
    registry.addComponent<Particle>(p, RED, Color{100, 0, 0, 0}, 1.5f, 0.1f);
  }
}
