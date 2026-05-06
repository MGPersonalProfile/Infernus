#pragma once
#include "../components/Health.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ECS.h"
#include "../scripting/LuaEngine.h"
#include "../utils/Constants.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>

class CameraSystem {
public:
  Camera2D camera = {{0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, 1.0f};

  void init(int screenWidth, int screenHeight) {
    camera.target = {0.0f, 0.0f};
    camera.offset = {screenWidth / 2.0f, screenHeight / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
  }

  bool shakeEnabled = true;

  void addShake(float intensity, float duration) {
    if (!shakeEnabled) return;
    shakeIntensity = intensity;
    shakeDuration = duration;
  }

  void update(Registry &registry, Entity targetEntity, float deltaTime) {
    if (!registry.hasComponent<Transform2D>(targetEntity))
      return;

    auto &transform = registry.getComponent<Transform2D>(targetEntity);

    // Lookahead — camera shifts in direction of movement so the player sees
    // ahead rather than behind. Magnitude is Lua-tunable.
    float lookahead = LuaEngine::getFeel("camera_lookahead", 60.0f);
    float aheadX = 0.0f, aheadY = 0.0f;
    if (registry.hasComponent<Velocity>(targetEntity)) {
      auto &vel = registry.getComponent<Velocity>(targetEntity);
      // Normalize and scale; clamp to lookahead magnitude
      float speedSq = vel.vx * vel.vx + vel.vy * vel.vy;
      if (speedSq > 1.0f) {
        float len = sqrtf(speedSq);
        aheadX = (vel.vx / len) * lookahead;
        aheadY = (vel.vy / len) * lookahead * 0.5f; // less Y bias (top-down 2D feel)
      }
    }

    // Smooth follow (lerp) — Lua-tunable
    Vector2 targetPos = {transform.x + 16.0f + aheadX, transform.y + 32.0f + aheadY};
    float lerpSpeed =
        LuaEngine::getFeel("camera_lerp_speed", Constants::CAMERA_LERP_SPEED) * deltaTime;
    camera.target.x = Lerp(camera.target.x, targetPos.x, lerpSpeed);
    camera.target.y = Lerp(camera.target.y, targetPos.y, lerpSpeed);

    // Dynamic zoom — base zoom + bonus when low HP (tension)
    float baseZoom = LuaEngine::getFeel("camera_zoom", 1.0f);
    float zoomLowHP = LuaEngine::getFeel("camera_zoom_lowhp_bonus", 0.1f);
    float dynamicZoom = baseZoom;
    if (registry.hasComponent<Health>(targetEntity)) {
      auto &h = registry.getComponent<Health>(targetEntity);
      if (h.maxHP > 0) {
        float hpRatio = (float)h.currentHP / (float)h.maxHP;
        if (hpRatio < 0.3f) {
          // Smooth bonus from 0.0 (at 30% HP) to 1.0 (at 0% HP)
          float t = (0.3f - hpRatio) / 0.3f;
          dynamicZoom += zoomLowHP * t;
        }
      }
    }
    camera.zoom = Lerp(camera.zoom, dynamicZoom, 4.0f * deltaTime);

    // Screen shake — Lua-tunable decay
    float shakeDecay =
        LuaEngine::getFeel("camera_shake_decay", Constants::CAMERA_SHAKE_DECAY);
    if (shakeDuration > 0.0f) {
      shakeDuration -= deltaTime;
      camera.offset.x +=
          (float)GetRandomValue((int)-shakeIntensity, (int)shakeIntensity);
      camera.offset.y +=
          (float)GetRandomValue((int)-shakeIntensity, (int)shakeIntensity);
      shakeIntensity = Lerp(shakeIntensity, 0.0f, shakeDecay * deltaTime);
    } else {
      float restoreSpeed = shakeDecay * deltaTime;
      camera.offset.x =
          Lerp(camera.offset.x, (float)GetScreenWidth() / 2.0f, restoreSpeed);
      camera.offset.y =
          Lerp(camera.offset.y, (float)GetScreenHeight() / 2.0f, restoreSpeed);
    }
  }

private:
  float shakeIntensity = 0.0f;
  float shakeDuration = 0.0f;
};
