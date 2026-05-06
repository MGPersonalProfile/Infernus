#pragma once
#include "../core/ECS.h"
#include "../components/Ability.h"
#include "../components/ActiveAbility.h"
#include <string>
#include <unordered_map>
#include <vector>

class AbilitySystem {
public:
  // Load passive ability pool from JSON file
  void loadAbilities(const std::string &path);

  // Load active abilities (Q/E slots) from JSON file
  void loadActiveAbilities(const std::string &path);

  // Update active buff timers + active ability cooldowns
  void update(Registry &registry, float deltaTime);

  // Get N random abilities from the pool (for selection screen)
  std::vector<AbilityData> getRandomChoices(int count) const;

  // Grant a passive ability to the player entity
  void grantAbility(Registry &registry, Entity player,
                    const AbilityData &ability);

  // === Active abilities ===
  // Equip the default Q/E actives for the given class id (warrior/rogue/knight).
  void equipDefaultActives(Registry &registry, Entity player,
                           const std::string &classId);

  // Equip a specific active ability into slot 0 (Q) or slot 1 (E).
  void equipActive(Registry &registry, Entity player, const std::string &id,
                   int slot);

  // Try to use the active ability in the given slot.
  // Returns true if the ability was triggered (cost paid, on cooldown now).
  bool tryUseActive(Registry &registry, Entity player, int slot);

  // Lookup
  const ActiveAbilityData *findActive(const std::string &id) const;

private:
  std::vector<AbilityData> abilityPool;
  std::unordered_map<std::string, ActiveAbilityData> activeAbilityPool;
  std::unordered_map<std::string, std::vector<std::string>> defaultActivesByClass;

  // Effect dispatchers
  void dispatchProjectile(Registry &registry, Entity player,
                          const ActiveAbilityData &a);
  void dispatchShield(Registry &registry, Entity player,
                      const ActiveAbilityData &a);
  void dispatchTeleport(Registry &registry, Entity player,
                        const ActiveAbilityData &a);
  void dispatchAOEKnockback(Registry &registry, Entity player,
                            const ActiveAbilityData &a);
  void dispatchLifestealBurst(Registry &registry, Entity player,
                              const ActiveAbilityData &a, int slot);
};
