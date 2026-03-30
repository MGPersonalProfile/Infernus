#pragma once
#include "../core/ECS.h"
#include "raylib.h"

struct Sprite : public Component {
  Texture2D texture;
  Rectangle sourceRect;
  bool flipX = false;
  int layer = 0; // Layer for rendering order (z-index)
  Color tint = WHITE;

  Sprite() = default;
  Sprite(Texture2D texture, Rectangle sourceRect, int layer = 0,
         Color tint = WHITE)
      : texture(texture), sourceRect(sourceRect), layer(layer), tint(tint) {}
};
