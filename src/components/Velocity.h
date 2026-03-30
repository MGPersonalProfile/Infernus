#pragma once
#include "../core/ECS.h"

struct Velocity : public Component {
    float vx = 0.0f;
    float vy = 0.0f;
    float friction = 0.0f; // Optional for slip/slide logic

    Velocity() = default;
    Velocity(float vx, float vy) : vx(vx), vy(vy) {}
};
