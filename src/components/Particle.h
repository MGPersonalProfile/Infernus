#pragma once
#include "../core/ECS.h"
#include <raylib.h>

struct Particle : public Component {
  Color startColor;
  Color endColor;
  float startScale;
  float endScale;

  Particle() = default;
  Particle(Color start, Color end, float sScale, float eScale)
      : startColor(start), endColor(end), startScale(sScale), endScale(eScale) {
  }
};
