#pragma once
#include "../core/ECS.h"
#include <string>

struct MiniBoss : public Component {
  std::string name = "Elite";

  // Special attack state
  float specialTimer = 0.0f;
  float specialCooldown = 3.0f;
  int specialType = 0; // 0=charge, 1=multishot, 2=slam
  bool specialActive = false;
  int specialStep = 0;

  // Charge state
  float chargeDirX = 0.0f;
  float chargeDirY = 0.0f;
  float chargeTimer = 0.0f;

  MiniBoss() = default;
  MiniBoss(const std::string &n, int sType, float cd)
      : name(n), specialCooldown(cd), specialType(sType) {}
};
