#pragma once
#include "../core/ECS.h"
#include "Item.h"

struct ItemPickup : public Component {
  ItemData item;
  ItemPickup() = default;
  ItemPickup(const ItemData &i) : item(i) {}
};
