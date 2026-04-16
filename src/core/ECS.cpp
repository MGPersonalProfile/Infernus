#include "ECS.h"
#include <algorithm>

Entity Registry::createEntity() {
  Entity id = nextEntityId++;
  activeEntities.push_back(id);
  aliveSet.insert(id);
  return id;
}

void Registry::destroyEntity(Entity entity) {
  if (aliveSet.erase(entity) == 0)
    return; // already destroyed or never existed
  auto it = std::find(activeEntities.begin(), activeEntities.end(), entity);
  if (it != activeEntities.end()) {
    // Swap-and-pop: O(1) removal from vector (order doesn't matter)
    std::swap(*it, activeEntities.back());
    activeEntities.pop_back();
  }
  for (auto &[type, entityMap] : components) {
    entityMap.erase(entity);
  }
}

void Registry::markForDestruction(Entity entity) {
  pendingDestroy.push_back(entity);
}

void Registry::flushDestroyed() {
  for (Entity entity : pendingDestroy) {
    destroyEntity(entity);
  }
  pendingDestroy.clear();
}

bool Registry::isAlive(Entity entity) const {
  return aliveSet.count(entity) > 0;
}
