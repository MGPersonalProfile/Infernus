#pragma once
#include "../core/ECS.h"

class CameraSystem;

class BossAISystem {
public:
  void update(Registry &registry, CameraSystem &cameraSystem, float deltaTime,
              Entity playerEntity);

private:
  void selectPattern(Registry &registry, Entity boss);
  void executeCharge(Registry &registry, Entity boss, CameraSystem &cam,
                     float deltaTime, Entity player);
  void executeGroundSlam(Registry &registry, Entity boss, CameraSystem &cam,
                         float deltaTime);
  void executeStomp(Registry &registry, Entity boss, CameraSystem &cam,
                    float deltaTime);
  void checkPhaseTransition(Registry &registry, Entity boss,
                            CameraSystem &cam);
  void spawnShockwave(Registry &registry, float x, float y, float radius,
                      int damage, Entity owner);
};
