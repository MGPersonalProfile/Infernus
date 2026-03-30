#pragma once
#include "../core/ECS.h"

struct Animation : public Component {
  int frames = 1;
  int currentFrame = 0;
  float frameSpeed = 0.1f;
  float timer = 0.0f;
  bool loop = true;
  bool finished = false;

  // Based on the assumption that spritesheets are row-oriented.
  // X, Y of starting frame in the spritesheet.
  float startX = 0;
  float startY = 0;
  float frameWidth = 0;
  float frameHeight = 0;

  Animation() = default;
  Animation(int frames, float speed, float w, float h, float sx = 0,
            float sy = 0, bool loop = true)
      : frames(frames), frameSpeed(speed), loop(loop), startX(sx), startY(sy),
        frameWidth(w), frameHeight(h) {}
};
