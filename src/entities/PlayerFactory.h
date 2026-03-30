#pragma once
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

class PlayerFactory {
public:
  static Entity create(Registry &registry, float startX, float startY) {
    Entity player = registry.createEntity();

    registry.addComponent<Transform2D>(player, startX, startY);

    // Player sprite (48x56 per frame, 2-frame idle sheet)
    Texture2D tex =
        ResourceManager::getInstance().getTexture("assets/sprites/player/player_idle.png");
    registry.addComponent<Sprite>(player, tex, Rectangle{0, 0, 48, 56}, 10);

    registry.addComponent<Animation>(player, 2, 0.4f, 48.0f, 56.0f);
    registry.addComponent<Velocity>(player, 0.0f, 0.0f);
    registry.addComponent<Collider>(player, 36.0f, 50.0f, false);

    // Combat stats: 100 HP, 100 Stamina (regen 20/s, 1.0s delay), 15 damage
    registry.addComponent<Health>(player, 100);
    registry.addComponent<Stamina>(player, 100.0f, 20.0f, 1.0f);
    registry.addComponent<Combat>(player, 15, 150.0f);

    return player;
  }
};
