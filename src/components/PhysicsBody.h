#pragma once
#include "../core/ECS.h"

struct PhysicsBody : public Component {
  float mass = 1.0f;
  float acceleration = 15.0f;
  float friction = 10.0f;
  float maxSpeed = 300.0f;

  PhysicsBody() = default;
  PhysicsBody(float mass, float acceleration, float friction, float maxSpeed)
      : mass(mass), acceleration(acceleration), friction(friction),
        maxSpeed(maxSpeed) {}
};
