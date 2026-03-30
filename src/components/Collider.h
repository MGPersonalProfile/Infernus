#pragma once
#include "../core/ECS.h"
#include "raylib.h"

struct Collider : public Component {
  Rectangle rect;
  bool isTrigger = false; // solid vs trigger volume

  // Optional offset from Transform position to center the hitbox
  float offsetX = 0.0f;
  float offsetY = 0.0f;

  Collider() = default;
  Collider(float w, float h, bool isTrigger = false)
      : rect({0, 0, w, h}), isTrigger(isTrigger) {}

  Collider(Rectangle r, bool isTrigger = false)
      : rect(r), isTrigger(isTrigger) {}
};
