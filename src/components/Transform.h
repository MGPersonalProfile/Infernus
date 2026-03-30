#pragma once
#include "../core/ECS.h"

struct Transform2D : public Component {
  float x = 0.0f;
  float y = 0.0f;
  float scale = 1.0f;
  float rotation = 0.0f;
  float facingX = 1.0f;  // Last movement direction (for attacks)
  float facingY = 0.0f;

  Transform2D() = default;
  Transform2D(float x, float y) : x(x), y(y) {}
};
