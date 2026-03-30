#pragma once
#include "../core/ECS.h"

struct Destructible : public Component {
  bool canDropItem = false;
  float itemDropChance = 0.3f;

  Destructible() = default;
};
