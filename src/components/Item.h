#pragma once
#include "../core/ECS.h"
#include <string>
#include <vector>

enum class ItemRarity { COMMON, UNCOMMON, RARE, EPIC, LEGENDARY };

enum class ItemEffect {
  // Stat bonuses
  STAT_DAMAGE,
  STAT_SPEED,
  STAT_MAX_HP,
  STAT_CRIT,
  STAT_LIFESTEAL,
  STAT_STAMINA_REGEN,
  STAT_ATTACK_SPEED,
  STAT_THORNS,
  // Attack pattern modifiers
  MULTI_HIT,
  PROJECTILE_ATTACK,
  AREA_ATTACK,
  CHAIN_ATTACK,
  FIRE_TRAIL,
};

struct ItemEffectData {
  ItemEffect type;
  float value;
};

struct ItemData {
  std::string id;
  std::string name;
  std::string description;
  ItemRarity rarity = ItemRarity::COMMON;
  std::vector<std::string> tags;
  std::vector<ItemEffectData> effects;
};

// Component: tracks equipped items on player
struct ItemHolder : public Component {
  std::vector<ItemData> equippedItems;
  int maxItems = 8;

  bool isFull() const { return (int)equippedItems.size() >= maxItems; }

  bool hasItem(const std::string &id) const {
    for (auto &it : equippedItems)
      if (it.id == id)
        return true;
    return false;
  }

  bool addItem(const ItemData &item) {
    if (isFull())
      return false;
    equippedItems.push_back(item);
    return true;
  }

  void removeItem(int index) {
    if (index >= 0 && index < (int)equippedItems.size())
      equippedItems.erase(equippedItems.begin() + index);
  }

  ItemHolder() = default;
};
