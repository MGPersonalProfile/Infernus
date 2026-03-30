// RenderSystem — Sprite Rendering with Layer Sorting and Visual Effects
//
// Draws all entities with Transform2D + Sprite, sorted by layer.
// Applies visual proxies: squash/stretch for combat, semi-transparency
// for i-frames, red flash on hit. Also renders floating health bars.

#pragma once

#include "../core/ECS.h"

class RenderSystem {
public:
  void update(Registry &registry);
};
