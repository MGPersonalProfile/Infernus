#pragma once
#include "../components/Animation.h"
#include "../components/Sprite.h"
#include "../core/ECS.h"

class AnimationSystem {
public:
  void update(Registry &registry, float deltaTime) {
    auto entities = registry.view<Animation, Sprite>();

    for (Entity entity : entities) {
      auto &anim = registry.getComponent<Animation>(entity);
      auto &sprite = registry.getComponent<Sprite>(entity);

      if (anim.finished)
        continue;

      anim.timer += deltaTime;
      if (anim.timer >= anim.frameSpeed) {
        anim.timer = 0.0f;
        anim.currentFrame++;

        if (anim.currentFrame >= anim.frames) {
          if (anim.loop) {
            anim.currentFrame = 0;
          } else {
            anim.currentFrame = anim.frames - 1;
            anim.finished = true;
          }
        }
      }

      // Sync sprite sourceRect to current animation frame
      sprite.sourceRect.x = anim.startX + (anim.currentFrame * anim.frameWidth);
      sprite.sourceRect.y = anim.startY;
      sprite.sourceRect.width = anim.frameWidth;
      sprite.sourceRect.height = anim.frameHeight;
    }
  }
};
