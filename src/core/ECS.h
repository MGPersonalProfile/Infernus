#pragma once

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using Entity = uint32_t;
constexpr Entity NULL_ENTITY = UINT32_MAX;

// Any struct that inherits from Component can be attached to an Entity.
struct Component {
  virtual ~Component() = default;
};

// The core Entity Component System registry.
// Manages entity lifecycles, component storage, and multi-component queries.
class Registry {
public:
  Registry() = default;
  ~Registry() = default;

  // -- Entity Management -----------------------------------------------------
  Entity createEntity();
  void destroyEntity(Entity entity); // Immediate destruction
  void markForDestruction(Entity entity); // Deferred — call flushDestroyed()
  void flushDestroyed(); // Process all pending destructions
  bool isAlive(Entity entity) const;

  // -- Component Management ---------------------------------------------------
  template <typename T, typename... Args>
  T &addComponent(Entity entity, Args &&...args);

  template <typename T> void removeComponent(Entity entity);
  template <typename T> bool hasComponent(Entity entity) const;
  template <typename T> T &getComponent(Entity entity);
  template <typename T> const T &getComponent(Entity entity) const;

  // View: returns all entities that have ALL specified components.
  // E.g., for (Entity e : registry.view<Transform2D, Velocity>()) { ... }
  template <typename... Ts> std::vector<Entity> view();

private:
  Entity nextEntityId = 0;
  std::vector<Entity> activeEntities;
  std::unordered_set<Entity> aliveSet; // O(1) isAlive lookups
  std::vector<Entity> pendingDestroy;

  // Outer map: type_index → per-entity storage.
  // Inner map: entity ID → owning pointer to the component instance.
  std::unordered_map<
      std::type_index,
      std::unordered_map<Entity, std::unique_ptr<Component>>>
      components;
};

// =============================================================================
// Template implementations (must remain in header)
// =============================================================================

template <typename T, typename... Args>
T &Registry::addComponent(Entity entity, Args &&...args) {
  auto component = std::make_unique<T>(std::forward<Args>(args)...);
  T *ptr = component.get();
  components[typeid(T)][entity] = std::move(component);
  return *ptr;
}

template <typename T> void Registry::removeComponent(Entity entity) {
  auto it = components.find(typeid(T));
  if (it != components.end()) {
    it->second.erase(entity);
  }
}

template <typename T> bool Registry::hasComponent(Entity entity) const {
  auto it = components.find(typeid(T));
  if (it != components.end()) {
    return it->second.count(entity) > 0;
  }
  return false;
}

template <typename T> T &Registry::getComponent(Entity entity) {
  return *static_cast<T *>(components[typeid(T)][entity].get());
}

template <typename T> const T &Registry::getComponent(Entity entity) const {
  return *static_cast<const T *>(
      components.at(typeid(T)).at(entity).get());
}

template <typename... Ts> std::vector<Entity> Registry::view() {
  std::vector<Entity> result;
  result.reserve(activeEntities.size());
  for (Entity entity : activeEntities) {
    if ((hasComponent<Ts>(entity) && ...)) {
      result.push_back(entity);
    }
  }
  return result;
}
