#include "AbilitySystem.h"
#include "../debug/Profiler.h"
#include "../components/Health.h"
#include "../components/Stamina.h"
#include <algorithm>
#include <fstream>
#include <json.hpp>
#include "raylib.h"

using json = nlohmann::json;

// Map string -> AbilityEffect enum
static AbilityEffect parseEffect(const std::string &s) {
  if (s == "damage_boost")
    return AbilityEffect::DAMAGE_BOOST;
  if (s == "speed_boost")
    return AbilityEffect::SPEED_BOOST;
  if (s == "heal")
    return AbilityEffect::HEAL;
  if (s == "shield")
    return AbilityEffect::SHIELD;
  if (s == "thorns")
    return AbilityEffect::THORNS;
  if (s == "lifesteal")
    return AbilityEffect::LIFESTEAL;
  if (s == "stamina_regen")
    return AbilityEffect::STAMINA_REGEN;
  if (s == "fire_trail")
    return AbilityEffect::FIRE_TRAIL;
  if (s == "crit_chance")
    return AbilityEffect::CRIT_CHANCE;
  if (s == "max_hp_up")
    return AbilityEffect::MAX_HP_UP;
  if (s == "dash_damage")
    return AbilityEffect::DASH_DAMAGE;
  if (s == "attack_speed_up")
    return AbilityEffect::ATTACK_SPEED_UP;
  return AbilityEffect::DAMAGE_BOOST;
}

static AbilityRarity parseRarity(const std::string &s) {
  if (s == "rare")
    return AbilityRarity::RARE;
  if (s == "epic")
    return AbilityRarity::EPIC;
  return AbilityRarity::COMMON;
}

void AbilitySystem::loadAbilities(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    return;

  json data;
  file >> data;

  for (auto &entry : data["abilities"]) {
    AbilityData ab;
    ab.id = entry.value("id", "unknown");
    ab.name = entry.value("name", "???");
    ab.description = entry.value("description", "");
    ab.effect = parseEffect(entry.value("effect", "damage_boost"));
    ab.rarity = parseRarity(entry.value("rarity", "common"));
    ab.value = entry.value("value", 0.0f);
    ab.duration = entry.value("duration", 0.0f);
    ab.cooldown = entry.value("cooldown", 0.0f);

    if (entry.contains("tags")) {
      for (auto &tag : entry["tags"])
        ab.tags.push_back(tag.get<std::string>());
    }

    abilityPool.push_back(ab);
  }
}

void AbilitySystem::update(Registry &registry, float deltaTime) {
  INFERNUS_ZONE_N("AbilitySystem");
  auto holders = registry.view<AbilityHolder>();
  for (Entity e : holders) {
    auto &holder = registry.getComponent<AbilityHolder>(e);

    bool changed = false;
    for (auto it = holder.activeBuffs.begin();
         it != holder.activeBuffs.end();) {
      it->remainingTime -= deltaTime;
      if (it->remainingTime <= 0.0f) {
        it = holder.activeBuffs.erase(it);
        changed = true;
      } else {
        ++it;
      }
    }

    if (changed)
      holder.recalculate();
  }
}

std::vector<AbilityData> AbilitySystem::getRandomChoices(int count) const {
  std::vector<AbilityData> choices;
  if (abilityPool.empty())
    return choices;

  // Copy indices, shuffle, pick first N
  std::vector<int> indices;
  indices.reserve(abilityPool.size());
  for (int i = 0; i < (int)abilityPool.size(); i++)
    indices.push_back(i);

  // Fisher-Yates shuffle using raylib's random
  for (int i = (int)indices.size() - 1; i > 0; i--) {
    int j = GetRandomValue(0, i);
    std::swap(indices[i], indices[j]);
  }

  int n = std::min(count, (int)indices.size());
  for (int i = 0; i < n; i++)
    choices.push_back(abilityPool[indices[i]]);

  return choices;
}

void AbilitySystem::grantAbility(Registry &registry, Entity player,
                                 const AbilityData &ability) {
  if (!registry.hasComponent<AbilityHolder>(player))
    registry.addComponent<AbilityHolder>(player);

  auto &holder = registry.getComponent<AbilityHolder>(player);

  // Immediate one-shot effects
  if (ability.effect == AbilityEffect::HEAL) {
    if (registry.hasComponent<Health>(player)) {
      auto &h = registry.getComponent<Health>(player);
      h.currentHP = std::min(h.currentHP + (int)ability.value, h.maxHP);
    }
    return; // Don't store heal as a permanent ability
  }

  if (ability.effect == AbilityEffect::SHIELD) {
    if (registry.hasComponent<Health>(player)) {
      registry.getComponent<Health>(player).invulnerabilityTimer = ability.value;
    }
    return;
  }

  // Timed buff
  if (ability.duration > 0.0f) {
    holder.activeBuffs.push_back(
        {ability.effect, ability.value, ability.duration});
    holder.recalculate();
    return;
  }

  // Permanent passive — apply max HP immediately
  if (ability.effect == AbilityEffect::MAX_HP_UP) {
    if (registry.hasComponent<Health>(player)) {
      auto &h = registry.getComponent<Health>(player);
      h.maxHP += (int)ability.value;
      h.currentHP += (int)ability.value;
    }
  }

  holder.abilities.push_back(ability);
  holder.recalculate();
}
