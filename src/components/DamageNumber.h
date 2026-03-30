#pragma once
#include "../core/ECS.h"
#include "raylib.h"
#include <string>

// Floating damage number that rises and fades
struct DamageNumber : public Component {
  std::string text;
  Color color = WHITE;
  float lifetime = 0.8f;
  float maxLifetime = 0.8f;
  float riseSpeed = -60.0f; // negative = upward

  DamageNumber() = default;
  DamageNumber(const std::string &txt, Color col)
      : text(txt), color(col), lifetime(0.8f), maxLifetime(0.8f) {}
  DamageNumber(const std::string &txt, Color col, float life)
      : text(txt), color(col), lifetime(life), maxLifetime(life) {}

  float alpha() const {
    return (maxLifetime > 0.0f) ? (lifetime / maxLifetime) : 0.0f;
  }
};
