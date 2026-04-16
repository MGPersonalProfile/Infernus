#pragma once
#include "../components/AnimState.h"
#include "../components/Animation.h"
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

    // Boss sprite — frame dimensions and counts from JSON "animation" block
    int frameW = 80, frameH = 80;
    int idleFrames = 2, chargeFrames = 3, slamFrames = 3;
    float idleSpeed = 0.2f, chargeSpeed_anim = 0.1f, slamSpeed = 0.1f;

    if (data.contains("animation")) {
      auto &anim = data["animation"];
      frameW = anim.value("frameWidth", 80);
      frameH = anim.value("frameHeight", 80);
      if (anim.contains("idle")) {
        idleFrames = anim["idle"].value("frames", 2);
        idleSpeed = anim["idle"].value("speed", 0.2f);
      }
      if (anim.contains("charge")) {
        chargeFrames = anim["charge"].value("frames", 3);
        chargeSpeed_anim = anim["charge"].value("speed", 0.1f);
      }
      if (anim.contains("slam")) {
        slamFrames = anim["slam"].value("frames", 3);
        slamSpeed = anim["slam"].value("speed", 0.1f);
      }
    }

    std::string spriteBase = "assets/sprites/bosses/" + bossId;
    Texture2D tex = ResourceManager::getInstance().getTexture(spriteBase + "_idle.png");
    registry.addComponent<Sprite>(boss, tex,
                                  Rectangle{0, 0, (float)frameW, (float)frameH}, 6);
    registry.addComponent<Animation>(boss, idleFrames, idleSpeed, (float)frameW, (float)frameH);

    // AnimState clips
    auto &as = registry.addComponent<AnimState>(boss);
    as.addClip(AnimStateType::IDLE, spriteBase + "_idle.png", idleFrames, idleSpeed, (float)frameW, (float)frameH);
    as.addClip(AnimStateType::CHARGE, spriteBase + "_charge.png", chargeFrames, chargeSpeed_anim, (float)frameW, (float)frameH);
    as.addClip(AnimStateType::SLAM, spriteBase + "_slam.png", slamFrames, slamSpeed, (float)frameW, (float)frameH, false);

    // Collider matches scaled visual
    registry.addComponent<Collider>(boss, (float)frameW * 1.3f, (float)frameH * 1.3f, false);
    registry.addComponent<Velocity>(boss, 0.0f, 0.0f);

    int hp = data.value("hp", 500);
    auto &health = registry.addComponent<Health>(boss, hp);
    if (data.contains("resistances")) {
      auto &r = data["resistances"];
      health.resistances.physical  = r.value("physical", 1.0f);
      health.resistances.fire      = r.value("fire", 1.0f);
      health.resistances.ice       = r.value("ice", 1.0f);
      health.resistances.lightning = r.value("lightning", 1.0f);
      health.resistances.toxic     = r.value("toxic", 1.0f);
    }

    int damage = data.value("damage", 20);
    float knockback = data.value("knockback", 200.0f);
    auto &combat = registry.addComponent<Combat>(boss, damage, knockback);
    std::string dmgTypeStr = data.value("damageType", "physical");
    if (dmgTypeStr == "fire") combat.damageType = DamageType::FIRE;
    else if (dmgTypeStr == "ice") combat.damageType = DamageType::ICE;
    else if (dmgTypeStr == "lightning") combat.damageType = DamageType::LIGHTNING;
    else if (dmgTypeStr == "toxic") combat.damageType = DamageType::TOXIC;

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
