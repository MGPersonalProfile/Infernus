#pragma once
#include "../components/AIBehavior.h"
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Loot.h"
#include "../components/MiniBoss.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "../core/ResourceManager.h"
#include <fstream>
#include <json.hpp>
#include <string>

using json = nlohmann::json;

class MiniBossFactory {
public:
  static Entity create(Registry &registry, const std::string &id, float x,
                       float y, Entity playerEntity) {
    std::string path = "assets/data/minibosses/" + id + ".json";
    std::ifstream file(path);
    json data;
    if (file.is_open()) file >> data;
    else {
      data = {{"name","Elite"},{"type","melee"},{"hp",150},{"damage",20},
              {"knockback",200},{"aggroRange",300},{"attackRange",50},
              {"patrolSpeed",40},{"chaseSpeed",120},{"attackWindup",0.4},
              {"specialType",0},{"specialCooldown",4.0},
              {"lootChance",1.0},{"lootType","health"},{"lootValue",40}};
    }

    Entity e = registry.createEntity();
    auto &t = registry.addComponent<Transform2D>(e, x, y);
    t.scale = 1.8f;

    // Sprite mapping
    std::string typeStr = data.value("type","melee");
    std::string spriteName = "demon";
    int fw=32, fh=32;
    if (typeStr == "ranged") { spriteName="lancer"; fw=24; fh=48; }
    else if (typeStr == "tank") { spriteName="brute"; fw=48; fh=48; }

    Texture2D tex = ResourceManager::getInstance().getTexture(
        "assets/sprites/enemies/" + spriteName + "_idle.png");
    registry.addComponent<Sprite>(e, tex, Rectangle{0,0,(float)fw,(float)fh}, 5);
    registry.addComponent<Collider>(e, (float)fw*1.8f, (float)fh*1.8f, false);
    registry.addComponent<Velocity>(e, 0.0f, 0.0f);
    registry.addComponent<Health>(e, data.value("hp",150));
    registry.addComponent<Combat>(e, data.value("damage",20), data.value("knockback",200.0f));

    AIBehavior ai;
    ai.targetEntity = playerEntity;
    if (typeStr=="ranged") ai.enemyType = EnemyType::RANGED;
    else if (typeStr=="tank") ai.enemyType = EnemyType::TANK;
    else ai.enemyType = EnemyType::MELEE;
    ai.aggroRange = data.value("aggroRange",300.0f);
    ai.attackRange = data.value("attackRange",50.0f);
    ai.patrolSpeed = data.value("patrolSpeed",40.0f);
    ai.chaseSpeed = data.value("chaseSpeed",120.0f);
    ai.attackWindup = data.value("attackWindup",0.4f);
    ai.retreatRange = data.value("retreatRange",0.0f);
    ai.projectileSpeed = data.value("projectileSpeed",300.0f);
    ai.projectileLifetime = data.value("projectileLifetime",2.0f);
    ai.attackCooldownMax = data.value("attackCooldown",1.5f);
    registry.addComponent<AIBehavior>(e, ai);

    std::string name = data.value("name","Elite");
    int sType = data.value("specialType",0);
    float sCD = data.value("specialCooldown",4.0f);
    auto &mb = registry.addComponent<MiniBoss>(e, name, sType, sCD);
    mb.specialTimer = sCD; // Don't attack immediately on spawn

    float lc = data.value("lootChance",1.0f);
    std::string lt = data.value("lootType","health");
    LootType lootType = LootType::HEALTH_ORB;
    if (lt == "stamina") lootType = LootType::STAMINA_ORB;
    else if (lt == "item") lootType = LootType::ITEM_DROP;
    registry.addComponent<Loot>(e, lootType, lc, data.value("lootValue",40));

    return e;
  }
};
