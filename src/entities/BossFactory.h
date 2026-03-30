#pragma once
#include "../components/BossPhase.h"
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

using json = nlohmann::json;

class BossFactory {
public:
  static Entity create(Registry &registry, const std::string &bossId, float x,
                       float y) {
    std::string path = "assets/data/bosses/" + bossId + ".json";
    std::ifstream file(path);

    json data;
    if (file.is_open()) {
      file >> data;
    } else {
      // Fallback: generic boss
      data = {{"name", "Unknown Boss"},
              {"hp", 500},
              {"damage", 20},
              {"knockback", 200.0},
              {"chargeSpeed", 600.0},
              {"color", {180, 50, 50, 255}},
              {"size", {64, 64}},
              {"phases",
               {{{"hpThreshold", 1.0},
                 {"speedMultiplier", 1.0},
                 {"damageMultiplier", 1.0},
                 {"attackWindup", 0.6},
                 {"attackCooldown", 2.0},
                 {"patterns", {"charge", "ground_slam"}}},
                {{"hpThreshold", 0.6},
                 {"speedMultiplier", 1.3},
                 {"damageMultiplier", 1.2},
                 {"attackWindup", 0.4},
                 {"attackCooldown", 1.5},
                 {"patterns", {"charge", "ground_slam", "stomp"}}},
                {{"hpThreshold", 0.3},
                 {"speedMultiplier", 1.6},
                 {"damageMultiplier", 1.5},
                 {"attackWindup", 0.3},
                 {"attackCooldown", 1.0},
                 {"patterns",
                  {"enraged_charge", "ground_slam", "stomp", "combo"}}}}}};
    }

    Entity boss = registry.createEntity();

    auto &bossTransform = registry.addComponent<Transform2D>(boss, x, y);
    bossTransform.scale = 1.3f;  // Scale boss up for intimidation

    // Boss sprite (80x80 per frame, 2-frame idle)
    std::string spritePath = "assets/sprites/bosses/" + bossId + "_idle.png";
    Texture2D tex = ResourceManager::getInstance().getTexture(spritePath);
    int frameW = 80, frameH = 80;
    registry.addComponent<Sprite>(boss, tex,
                                  Rectangle{0, 0, (float)frameW, (float)frameH}, 6);

    // Collider matches scaled visual
    registry.addComponent<Collider>(boss, (float)frameW * 1.3f, (float)frameH * 1.3f, false);
    registry.addComponent<Velocity>(boss, 0.0f, 0.0f);

    int hp = data.value("hp", 500);
    registry.addComponent<Health>(boss, hp);

    int damage = data.value("damage", 20);
    float knockback = data.value("knockback", 200.0f);
    registry.addComponent<Combat>(boss, damage, knockback);

    // --- Boss Phase ---
    BossPhase bp;
    bp.bossName = data.value("name", "Boss");
    bp.chargeSpeed = data.value("chargeSpeed", 600.0f);

    auto &phasesArr = data["phases"];
    bp.totalPhases = (int)phasesArr.size();
    for (auto &p : phasesArr) {
      BossPhaseData pd;
      pd.hpThreshold = p.value("hpThreshold", 1.0f);
      pd.speedMultiplier = p.value("speedMultiplier", 1.0f);
      pd.damageMultiplier = p.value("damageMultiplier", 1.0f);
      pd.attackWindup = p.value("attackWindup", 0.5f);
      pd.attackCooldown = p.value("attackCooldown", 2.0f);
      for (auto &pat : p["patterns"]) {
        pd.patterns.push_back(pat.get<std::string>());
      }
      bp.phases.push_back(pd);
    }
    bp.patternTimer = 1.5f; // Initial delay before first attack

    registry.addComponent<BossPhase>(boss, bp);
    registry.addComponent<Loot>(boss, LootType::HEALTH_ORB, 1.0f, 50);

    return boss;
  }
};
