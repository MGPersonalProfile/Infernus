#include "SynergySystem.h"
#include "../components/Health.h"
#include "../components/Item.h"
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

void SynergySystem::loadSynergies(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    return;

  json data;
  file >> data;

  for (auto &entry : data["synergies"]) {
    SynergyDef syn;
    syn.id = entry.value("id", "unknown");
    syn.name = entry.value("name", "???");
    syn.description = entry.value("description", "");
    syn.requiredTag = entry.value("required_tag", "");
    syn.requiredCount = entry.value("required_count", 2);
    syn.bonusDamage = entry.value("bonus_damage", 0.0f);
    syn.bonusSpeed = entry.value("bonus_speed", 0.0f);
    syn.bonusCrit = entry.value("bonus_crit", 0.0f);
    syn.bonusLifesteal = entry.value("bonus_lifesteal", 0.0f);
    syn.bonusMaxHP = entry.value("bonus_max_hp", 0);
    syn.bonusStaminaRegen = entry.value("bonus_stamina_regen", 0.0f);
    synergies.push_back(syn);
    states.push_back({syn.id, false});
  }
}

void SynergySystem::evaluate(Registry &registry, Entity player) {
  if (!registry.hasComponent<AbilityHolder>(player))
    return;

  auto &holder = registry.getComponent<AbilityHolder>(player);

  // Count tags from player's abilities AND items
  std::unordered_map<std::string, int> tagCounts;
  for (auto &ab : holder.abilities) {
    for (auto &tag : ab.tags)
      tagCounts[tag]++;
  }
  if (registry.hasComponent<ItemHolder>(player)) {
    auto &items = registry.getComponent<ItemHolder>(player);
    for (auto &item : items.equippedItems) {
      for (auto &tag : item.tags)
        tagCounts[tag]++;
    }
  }

  // Just update active/inactive state — bonuses are applied in recalculatePlayerStats
  for (int i = 0; i < (int)synergies.size(); i++) {
    states[i].active = (tagCounts[synergies[i].requiredTag] >= synergies[i].requiredCount);
  }
}
