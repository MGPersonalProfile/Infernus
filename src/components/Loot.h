// ============================================================================
// Loot.h — Loot Drop Component
// ============================================================================
// Attached to enemies that can drop items on death. The HealthSystem checks
// for this component when an entity dies and spawns the appropriate pickup.
// ============================================================================

#pragma once
#include "../core/ECS.h"

// Type of loot that can be dropped
enum class LootType {
  HEALTH_ORB,  // Restores player HP
  STAMINA_ORB, // Restores player stamina
  ITEM_DROP    // Drops an equippable item
};

struct Loot : public Component {
  LootType type = LootType::HEALTH_ORB;
  float dropChance = 0.5f; // 0.0 to 1.0 — probability of dropping
  int value = 20;          // Amount of HP or stamina restored

  Loot() = default;
  Loot(LootType lootType, float chance, int val)
      : type(lootType), dropChance(chance), value(val) {}
};
