#pragma once
#include "../core/ECS.h"
#include "../components/Ability.h"
#include <string>
#include <vector>

class AbilitySystem {
public:
  // Load ability pool from JSON file
  void loadAbilities(const std::string &path);

  // Update active buff timers
  void update(Registry &registry, float deltaTime);

  // Get N random abilities from the pool (for selection screen)
  std::vector<AbilityData> getRandomChoices(int count) const;

  // Grant an ability to the player entity
  void grantAbility(Registry &registry, Entity player,
                    const AbilityData &ability);

private:
  std::vector<AbilityData> abilityPool;
};
