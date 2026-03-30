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
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Loot.h"
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
    // Map enemy types to sprite paths and frame sizes
    std::string spriteName = "demon"; // default
    int frameW = 32, frameH = 32;
    std::string typeStr2 = data.value("type", "melee");
    if (enemyId == "assassin") {
      spriteName = "assassin";
      frameW = 24; frameH = 40;
    } else if (enemyId == "bomber") {
      spriteName = "bomber";
      frameW = 32; frameH = 32;
    } else if (typeStr2 == "ranged") {
      spriteName = "lancer";
      frameW = 24; frameH = 48;
    } else if (typeStr2 == "tank") {
      spriteName = "brute";
      frameW = 48; frameH = 48;
    }

    std::string spritePath = "assets/sprites/enemies/" + spriteName + "_idle.png";
    Texture2D tex = ResourceManager::getInstance().getTexture(spritePath);
    registry.addComponent<Sprite>(enemy, tex,
                                  Rectangle{0, 0, (float)frameW, (float)frameH}, 5);

    // Collider matches scaled visual size
    registry.addComponent<Collider>(enemy, (float)frameW * 1.4f, (float)frameH * 1.4f, false);
    registry.addComponent<Velocity>(enemy, 0.0f, 0.0f);

    int hp = data.value("hp", 30);
    registry.addComponent<Health>(enemy, hp);

    int damage = data.value("damage", 10);
    float knockback = data.value("knockback", 100.0f);
    registry.addComponent<Combat>(enemy, damage, knockback);

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

    return enemy;
  }
};
