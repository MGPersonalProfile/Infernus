// EnemyFactory — Creates enemy entities from JSON configuration files
//
// Loads stats from assets/data/enemies/<id>.json and creates fully-configured
// ECS entities. Each enemy gets: Transform, Sprite, Collider, Health, Combat,
// Velocity, AIBehavior, and optionally Loot.
//
// Usage:
//   EnemyFactory::create(registry, "melee", 400.0f, 300.0f, playerEntity);

#pragma once
#include "../components/AIBehavior.h"
#include "../components/AnimState.h"
#include "../components/Animation.h"
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Loot.h"
#include "../components/PhysicsBody.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "../core/ResourceManager.h"

#include <fstream>
#include <json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

class EnemyFactory {
public:
  static Entity create(Registry &registry, const std::string &enemyId, float x,
                       float y, Entity playerEntity) {
    // --- Load JSON config ---
    std::string path = "assets/data/enemies/" + enemyId + ".json";
    std::ifstream file(path);

    json data;
    if (file.is_open()) {
      file >> data;
    } else {
      // Fallback defaults (magenta = missing config)
      data = {{"name", "Unknown"},
              {"type", "melee"},
              {"hp", 30},
              {"damage", 10},
              {"knockback", 100.0},
              {"aggroRange", 200.0},
              {"attackRange", 45.0},
              {"patrolSpeed", 60.0},
              {"chaseSpeed", 120.0},
              {"attackWindup", 0.3},
              {"color", {255, 0, 255, 255}},
              {"size", {32, 32}},
              {"lootChance", 0.5},
              {"lootType", "health"},
              {"lootValue", 10}};
    }

    Entity enemy = registry.createEntity();

    auto &transform = registry.addComponent<Transform2D>(enemy, x, y);
    transform.scale = 1.4f;  // Scale enemies up slightly

    // --- Sprite (actual textures) ---
    // Frame dimensions and counts from JSON "animation" block, with fallbacks
    int frameW = 32, frameH = 32;
    int idleFrames = 4, runFrames = 6, attackFrames = 4;
    float idleSpeed = 0.15f, runSpeed = 0.1f, attackSpeed = 0.08f;

    if (data.contains("animation")) {
      auto &anim = data["animation"];
      frameW = anim.value("frameWidth", 32);
      frameH = anim.value("frameHeight", 32);
      if (anim.contains("idle")) {
        idleFrames = anim["idle"].value("frames", 4);
        idleSpeed = anim["idle"].value("speed", 0.15f);
      }
      if (anim.contains("run")) {
        runFrames = anim["run"].value("frames", 6);
        runSpeed = anim["run"].value("speed", 0.1f);
      }
      if (anim.contains("attack")) {
        attackFrames = anim["attack"].value("frames", 4);
        attackSpeed = anim["attack"].value("speed", 0.08f);
      }
    } else if (data.contains("size")) {
      // Legacy fallback: use "size" field from JSON
      frameW = data["size"][0].get<int>();
      frameH = data["size"][1].get<int>();
    }

    std::string spriteBase = "assets/sprites/enemies/" + enemyId;
    Texture2D tex = ResourceManager::getInstance().getTexture(spriteBase + "_idle.png");
    registry.addComponent<Sprite>(enemy, tex,
                                  Rectangle{0, 0, (float)frameW, (float)frameH}, 5);
    registry.addComponent<Animation>(enemy, idleFrames, idleSpeed, (float)frameW, (float)frameH);

    // AnimState clips (idle/run/attack)
    auto &as = registry.addComponent<AnimState>(enemy);
    as.addClip(AnimStateType::IDLE, spriteBase + "_idle.png", idleFrames, idleSpeed, (float)frameW, (float)frameH);
    as.addClip(AnimStateType::RUN, spriteBase + "_run.png", runFrames, runSpeed, (float)frameW, (float)frameH);
    as.addClip(AnimStateType::ATTACK, spriteBase + "_attack.png", attackFrames, attackSpeed, (float)frameW, (float)frameH, false);

    // Collider matches scaled visual size
    registry.addComponent<Collider>(enemy, (float)frameW * 1.4f, (float)frameH * 1.4f, false);
    registry.addComponent<Velocity>(enemy, 0.0f, 0.0f);

    int hp = data.value("hp", 30);
    auto &health = registry.addComponent<Health>(enemy, hp);

    // Elemental resistances (from JSON "resistances" block)
    if (data.contains("resistances")) {
      auto &r = data["resistances"];
      health.resistances.physical  = r.value("physical", 1.0f);
      health.resistances.fire      = r.value("fire", 1.0f);
      health.resistances.ice       = r.value("ice", 1.0f);
      health.resistances.lightning = r.value("lightning", 1.0f);
      health.resistances.toxic     = r.value("toxic", 1.0f);
    }

    int damage = data.value("damage", 10);
    float knockback = data.value("knockback", 100.0f);
    auto &combat = registry.addComponent<Combat>(enemy, damage, knockback);

    // Damage type from JSON
    std::string dmgTypeStr = data.value("damageType", "physical");
    if (dmgTypeStr == "fire") combat.damageType = DamageType::FIRE;
    else if (dmgTypeStr == "ice") combat.damageType = DamageType::ICE;
    else if (dmgTypeStr == "lightning") combat.damageType = DamageType::LIGHTNING;
    else if (dmgTypeStr == "toxic") combat.damageType = DamageType::TOXIC;

    // --- AI Behavior ---
    AIBehavior ai;
    ai.targetEntity = playerEntity;

    std::string typeStr = data.value("type", "melee");
    if (typeStr == "ranged")
      ai.enemyType = EnemyType::RANGED;
    else if (typeStr == "tank")
      ai.enemyType = EnemyType::TANK;
    else
      ai.enemyType = EnemyType::MELEE;

    ai.aggroRange = data.value("aggroRange", 200.0f);
    ai.attackRange = data.value("attackRange", 45.0f);
    ai.patrolSpeed = data.value("patrolSpeed", 60.0f);
    ai.chaseSpeed = data.value("chaseSpeed", 120.0f);
    ai.attackWindup = data.value("attackWindup", 0.3f);
    ai.retreatRange = data.value("retreatRange", 0.0f);
    ai.projectileSpeed = data.value("projectileSpeed", 300.0f);
    ai.projectileLifetime = data.value("projectileLifetime", 2.0f);
    ai.attackCooldownMax = data.value("attackCooldown", 1.5f);

    registry.addComponent<AIBehavior>(enemy, ai);

    // --- Loot (optional) ---
    float lootChance = data.value("lootChance", 0.0f);
    if (lootChance > 0.0f) {
      std::string lootTypeStr = data.value("lootType", "health");
      LootType lootType = LootType::HEALTH_ORB;
      if (lootTypeStr == "stamina") lootType = LootType::STAMINA_ORB;
      else if (lootTypeStr == "item") lootType = LootType::ITEM_DROP;
      int lootValue = data.value("lootValue", 10);
      registry.addComponent<Loot>(enemy, lootType, lootChance, lootValue);
    }

    // --- Physics (optional, from JSON "physics" block) ---
    if (data.contains("physics")) {
      auto &phys = data["physics"];
      float mass = phys.value("mass", 1.0f);
      float accel = phys.value("acceleration", 15.0f);
      float fric = phys.value("friction", 10.0f);
      float maxSpd = ai.chaseSpeed * 1.5f;
      registry.addComponent<PhysicsBody>(enemy, mass, accel, fric, maxSpd);
    }

    return enemy;
  }
};
