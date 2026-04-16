#pragma once
#include "../components/AnimState.h"
#include "../components/Animation.h"
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Sprite.h"
#include "../components/Stamina.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "../core/ResourceManager.h"

#include <string>

class PlayerFactory {
public:
  static Entity create(Registry &registry, float startX, float startY,
                       const std::string &classId = "warrior") {
    Entity player = registry.createEntity();

    registry.addComponent<Transform2D>(player, startX, startY);

    // Frame sizes per class
    float fw = 48.0f, fh = 56.0f;
    if (classId == "rogue") { fw = 40.0f; fh = 52.0f; }
    else if (classId == "knight") { fw = 52.0f; fh = 60.0f; }

    std::string base = "assets/sprites/player/" + classId;

    // Initial sprite (idle)
    Texture2D tex = ResourceManager::getInstance().getTexture(base + "_idle.png");
    registry.addComponent<Sprite>(player, tex, Rectangle{0, 0, fw, fh}, 10);
    registry.addComponent<Animation>(player, 6, 0.4f, fw, fh);

    // AnimState — register all clips (frame counts updated when Antigravity delivers)
    auto &as = registry.addComponent<AnimState>(player);
    as.addClip(AnimStateType::IDLE, base + "_idle.png", 6, 0.4f, fw, fh);
    as.addClip(AnimStateType::RUN, base + "_run.png", 8, 0.1f, fw, fh);
    as.addClip(AnimStateType::ATTACK, base + "_attack.png", 6, 0.08f, fw, fh, false);

    registry.addComponent<Velocity>(player, 0.0f, 0.0f);
    registry.addComponent<Collider>(player, fw * 0.75f, fh * 0.9f, false);

    // Combat stats: 100 HP, 100 Stamina (regen 20/s, 1.0s delay), 15 damage
    registry.addComponent<Health>(player, 100);
    registry.addComponent<Stamina>(player, 100.0f, 20.0f, 1.0f);
    registry.addComponent<Combat>(player, 15, 150.0f);

    return player;
  }
};
