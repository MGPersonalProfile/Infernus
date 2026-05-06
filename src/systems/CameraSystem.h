#pragma once
#include "../components/Transform.h"
#include "../core/ECS.h"
#include "../scripting/LuaEngine.h"
#include "../utils/Constants.h"
#include "raylib.h"
#include "raymath.h"

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

    // Smooth follow (lerp) — Lua-tunable
    Vector2 targetPos = {transform.x + 16.0f, transform.y + 32.0f};
    float lerpSpeed =
        LuaEngine::getFeel("camera_lerp_speed", Constants::CAMERA_LERP_SPEED) * deltaTime;
    camera.target.x = Lerp(camera.target.x, targetPos.x, lerpSpeed);
    camera.target.y = Lerp(camera.target.y, targetPos.y, lerpSpeed);

    // Live zoom — Lua-tunable (defaults to 1.0; will set to 1.5 in Fase 2)
    camera.zoom = LuaEngine::getFeel("camera_zoom", 1.0f);

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
