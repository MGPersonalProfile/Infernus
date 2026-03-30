#include "ECS.h"
#include <algorithm>

Entity Registry::createEntity() {
  Entity id = nextEntityId++;
  activeEntities.push_back(id);
  return id;
}

void Registry::destroyEntity(Entity entity) {
  auto it = std::find(activeEntities.begin(), activeEntities.end(), entity);
  if (it != activeEntities.end()) {
    activeEntities.erase(it);
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
  return std::find(activeEntities.begin(), activeEntities.end(), entity) !=
         activeEntities.end();
}
