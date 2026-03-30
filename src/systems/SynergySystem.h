#pragma once
#include "../components/Ability.h"
#include "../core/ECS.h"
#include <string>
#include <vector>

struct SynergyDef {
  std::string id;
  std::string name;
  std::string description;
  std::string requiredTag;    // Tag that triggers this synergy
  int requiredCount = 2;      // How many abilities with this tag are needed

  // Bonus stats when synergy is active
  float bonusDamage = 0.0f;
  float bonusSpeed = 0.0f;
  float bonusCrit = 0.0f;
  float bonusLifesteal = 0.0f;
  int bonusMaxHP = 0;
  float bonusStaminaRegen = 0.0f;
};

struct SynergyState {
  std::string id;
  bool active = false;
};

class SynergySystem {
public:
  void loadSynergies(const std::string &path);
  void evaluate(Registry &registry, Entity player);

  const std::vector<SynergyDef> &getDefs() const { return synergies; }
  const std::vector<SynergyState> &getStates() const { return states; }

private:
  std::vector<SynergyDef> synergies;
  std::vector<SynergyState> states;
};
