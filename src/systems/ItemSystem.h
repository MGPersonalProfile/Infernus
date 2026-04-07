#pragma once
#include "../components/Item.h"
#include "../core/ECS.h"
#include <string>
#include <vector>

class ItemSystem {
public:
  void loadItems(const std::string &path);
  ItemData getRandomItem(int difficulty) const;
  ItemData getItemById(const std::string &id) const;
  std::vector<ItemData> getRandomItems(int count, int difficulty) const;
  void grantItem(Registry &registry, Entity player, const ItemData &item);

private:
  std::vector<ItemData> itemPool;
};
