#pragma once
#include "../core/ECS.h"
#include <cstdlib>

enum class TrapType { SPIKE, FIRE_TRAP, PIT };

struct Trap : public Component {
  TrapType type = TrapType::SPIKE;
  float cycleTime = 1.5f;  // fire trap on/off period
  float timer = 0.0f;
  bool active = true;
  int damage = 10;

  Trap() = default;
  Trap(TrapType t, int dmg) : type(t), damage(dmg) {
    if (t == TrapType::FIRE_TRAP)
      timer = (float)(std::rand() % 100) / 100.0f * cycleTime; // desync traps
  }
};
