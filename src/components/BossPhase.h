#pragma once
#include "../core/ECS.h"
#include <string>
#include <vector>

// Defines a single phase in a multi-phase boss fight
struct BossPhaseData {
  float hpThreshold = 1.0f;  // Phase activates below this HP ratio
  float speedMultiplier = 1.0f;
  float damageMultiplier = 1.0f;
  float attackWindup = 0.5f;
  float attackCooldown = 2.0f;
  std::vector<std::string> patterns; // Available attack patterns
};

// Identifies available boss attack patterns
enum class BossPattern {
  CHARGE,       // Rush toward player, deal damage on contact
  GROUND_SLAM,  // AoE around boss
  STOMP,        // Shockwave outward
  COMBO,        // Multiple quick attacks
  ENRAGED_CHARGE // Faster, stronger charge
};

struct BossPhase : public Component {
  std::string bossName = "Boss";
  int currentPhase = 0;
  int totalPhases = 3;
  std::vector<BossPhaseData> phases;

  // Current attack pattern state
  BossPattern currentPattern = BossPattern::CHARGE;
  float patternTimer = 0.0f;
  bool patternActive = false;
  int patternStep = 0; // Sub-step within a pattern
  int comboCount = 0;  // Combo repetitions done

  // Charge attack state
  float chargeSpeed = 600.0f;
  float chargeDirX = 0.0f;
  float chargeDirY = 0.0f;
  bool isCharging = false;
  float chargeTimer = 0.0f;
  float chargeLastX = 0.0f; // for stuck detection
  float chargeLastY = 0.0f;
  float chargeStuckTimer = 0.0f;
  float chargeHitTimer = 0.0f; // throttle hitbox spawns during charge

  // Phase transition
  bool transitioning = false;
  float transitionTimer = 0.0f;

  // Enrage
  bool enraged = false;

  BossPhase() = default;

  // Check if boss should transition to next phase based on current HP ratio
  bool shouldTransition(float hpRatio) const {
    if (currentPhase + 1 >= totalPhases)
      return false;
    return hpRatio <= phases[currentPhase + 1].hpThreshold;
  }

  const BossPhaseData &current() const { return phases[currentPhase]; }
};
