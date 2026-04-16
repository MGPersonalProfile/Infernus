#pragma once
#include "../components/AnimState.h"
#include "../components/Animation.h"
#include "../components/Sprite.h"
#include "../core/ECS.h"
#include "../core/ResourceManager.h"

class AnimationSystem {
public:
  void update(Registry &registry, float deltaTime) {
    // Process AnimState transitions (swap texture + reset animation)
    auto stateful = registry.view<AnimState, Animation, Sprite>();
    for (Entity entity : stateful) {
      auto &as = registry.getComponent<AnimState>(entity);
      if (!as.dirty) continue;
      as.dirty = false;

      auto it = as.clips.find(as.current);
      if (it == as.clips.end()) continue;

      auto &clip = it->second;
      auto &anim = registry.getComponent<Animation>(entity);
      auto &sprite = registry.getComponent<Sprite>(entity);

      sprite.texture = ResourceManager::getInstance().getTexture(clip.texturePath);
      anim.frames = clip.frames;
      anim.frameSpeed = clip.frameSpeed;
      anim.frameWidth = clip.frameWidth;
      anim.frameHeight = clip.frameHeight;
      anim.currentFrame = 0;
      anim.timer = 0.0f;
      anim.loop = clip.loop;
      anim.finished = false;
      anim.startX = 0;
      anim.startY = 0;

      sprite.sourceRect = {0, 0, clip.frameWidth, clip.frameHeight};
    }

    // Tick all animations
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
            // Non-looping animations (attack, death) revert to idle
            if (registry.hasComponent<AnimState>(entity)) {
              auto &as = registry.getComponent<AnimState>(entity);
              if (as.current == AnimStateType::ATTACK)
                as.setState(AnimStateType::IDLE);
            }
          }
        }
      }

      sprite.sourceRect.x = anim.startX + (anim.currentFrame * anim.frameWidth);
      sprite.sourceRect.y = anim.startY;
      sprite.sourceRect.width = anim.frameWidth;
      sprite.sourceRect.height = anim.frameHeight;
    }
  }
};
