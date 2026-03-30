#pragma once
#include "../core/ECS.h"

class CameraSystem;

class MiniBossAISystem {
public:
  void update(Registry &registry, CameraSystem &cam, float deltaTime,
              Entity playerEntity);

private:
  void executeCharge(Registry &registry, Entity mb, CameraSystem &cam,
                     float deltaTime, Entity player);
  void executeMultishot(Registry &registry, Entity mb, float deltaTime,
                        Entity player);
  void executeSlam(Registry &registry, Entity mb, CameraSystem &cam,
                   float deltaTime);
};
