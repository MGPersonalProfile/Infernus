#include "ItemSystem.h"
#include <algorithm>
#include <fstream>
#include <json.hpp>
#include "raylib.h"

using json = nlohmann::json;

static ItemEffect parseEffect(const std::string &s) {
  if (s == "stat_damage") return ItemEffect::STAT_DAMAGE;
  if (s == "stat_speed") return ItemEffect::STAT_SPEED;
  if (s == "stat_max_hp") return ItemEffect::STAT_MAX_HP;
  if (s == "stat_crit") return ItemEffect::STAT_CRIT;
  if (s == "stat_lifesteal") return ItemEffect::STAT_LIFESTEAL;
  if (s == "stat_stamina_regen") return ItemEffect::STAT_STAMINA_REGEN;
  if (s == "stat_attack_speed") return ItemEffect::STAT_ATTACK_SPEED;
  if (s == "stat_thorns") return ItemEffect::STAT_THORNS;
  if (s == "multi_hit") return ItemEffect::MULTI_HIT;
  if (s == "projectile_attack") return ItemEffect::PROJECTILE_ATTACK;
  if (s == "area_attack") return ItemEffect::AREA_ATTACK;
  if (s == "chain_attack") return ItemEffect::CHAIN_ATTACK;
  if (s == "fire_trail") return ItemEffect::FIRE_TRAIL;
  return ItemEffect::STAT_DAMAGE;
}

static ItemRarity parseRarity(const std::string &s) {
  if (s == "uncommon") return ItemRarity::UNCOMMON;
  if (s == "rare") return ItemRarity::RARE;
  if (s == "epic") return ItemRarity::EPIC;
  if (s == "legendary") return ItemRarity::LEGENDARY;
  return ItemRarity::COMMON;
}

void ItemSystem::loadItems(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) return;
  json data;
  file >> data;

  for (auto &j : data["items"]) {
    ItemData item;
    item.id = j.value("id", "");
    item.name = j.value("name", "???");
    item.description = j.value("description", "");
    item.rarity = parseRarity(j.value("rarity", "common"));
    item.tags = j.value("tags", std::vector<std::string>{});

    if (j.contains("effects")) {
      for (auto &ej : j["effects"]) {
        ItemEffectData eff;
        eff.type = parseEffect(ej.value("type", "stat_damage"));
        eff.value = ej.value("value", 0.0f);
        item.effects.push_back(eff);
      }
    }
    itemPool.push_back(item);
  }
}

ItemData ItemSystem::getItemById(const std::string &id) const {
  for (auto &item : itemPool)
    if (item.id == id) return item;
  return ItemData{};
}

ItemData ItemSystem::getRandomItem(int difficulty) const {
  if (itemPool.empty()) return ItemData{};

  // Weight by rarity — higher difficulty = better items
  std::vector<int> weights;
  for (auto &item : itemPool) {
    int w = 10;
    switch (item.rarity) {
    case ItemRarity::COMMON: w = std::max(5, 40 - difficulty * 3); break;
    case ItemRarity::UNCOMMON: w = 30; break;
    case ItemRarity::RARE: w = 20 + difficulty; break;
    case ItemRarity::EPIC: w = 8 + difficulty * 2; break;
    case ItemRarity::LEGENDARY: w = 2 + difficulty; break;
    }
    weights.push_back(w);
  }

  int totalWeight = 0;
  for (int w : weights) totalWeight += w;

  int roll = GetRandomValue(0, totalWeight - 1);
  int cumulative = 0;
  for (size_t i = 0; i < itemPool.size(); i++) {
    cumulative += weights[i];
    if (roll < cumulative) return itemPool[i];
  }
  return itemPool.back();
}

std::vector<ItemData> ItemSystem::getRandomItems(int count, int difficulty) const {
  std::vector<ItemData> result;
  for (int i = 0; i < count && i < (int)itemPool.size(); i++) {
    ItemData item;
    int attempts = 0;
    do {
      item = getRandomItem(difficulty);
      attempts++;
    } while (attempts < 20 && std::any_of(result.begin(), result.end(),
             [&](const ItemData &r) { return r.id == item.id; }));
    result.push_back(item);
  }
  return result;
}

void ItemSystem::grantItem(Registry &registry, Entity player, const ItemData &item) {
  if (!registry.hasComponent<ItemHolder>(player)) return;
  auto &holder = registry.getComponent<ItemHolder>(player);
  holder.addItem(item);
}
