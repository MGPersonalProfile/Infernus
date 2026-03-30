#include "../src/components/Transform.h"
#include "../src/components/Velocity.h"
#include "../src/core/ECS.h"
#include <cassert>
#include <iostream>

void testEntityCreation() {
  Registry registry;
  Entity e1 = registry.createEntity();
  Entity e2 = registry.createEntity();
  Entity e3 = registry.createEntity();

  assert(e1 != e2 && e2 != e3);
  assert(registry.isAlive(e1));
  assert(registry.isAlive(e2));

  std::cout << "[OK] Entity creation\n";
}

void testComponents() {
  Registry registry;
  Entity e1 = registry.createEntity();
  Entity e2 = registry.createEntity();

  registry.addComponent<Transform2D>(e1, 10.0f, 20.0f);
  registry.addComponent<Velocity>(e1, 5.0f, 5.0f);
  registry.addComponent<Transform2D>(e2, 100.0f, 200.0f);

  assert(registry.hasComponent<Transform2D>(e1));
  assert(registry.hasComponent<Velocity>(e1));
  assert(registry.hasComponent<Transform2D>(e2));
  assert(!registry.hasComponent<Velocity>(e2));

  auto &t1 = registry.getComponent<Transform2D>(e1);
  assert(t1.x == 10.0f && t1.y == 20.0f);

  std::cout << "[OK] Component add/get/has\n";
}

void testViews() {
  Registry registry;
  Entity e1 = registry.createEntity();
  Entity e2 = registry.createEntity();

  registry.addComponent<Transform2D>(e1, 10.0f, 20.0f);
  registry.addComponent<Velocity>(e1, 5.0f, 5.0f);
  registry.addComponent<Transform2D>(e2, 100.0f, 200.0f);

  auto transformEntities = registry.view<Transform2D>();
  assert(transformEntities.size() == 2);

  auto movingEntities = registry.view<Transform2D, Velocity>();
  assert(movingEntities.size() == 1);
  assert(movingEntities[0] == e1);

  std::cout << "[OK] View queries\n";
}

void testRemoveComponent() {
  Registry registry;
  Entity e = registry.createEntity();
  registry.addComponent<Transform2D>(e, 10.0f, 20.0f);
  registry.addComponent<Velocity>(e, 5.0f, 5.0f);

  registry.removeComponent<Velocity>(e);
  assert(!registry.hasComponent<Velocity>(e));
  auto remaining = registry.view<Transform2D, Velocity>();
  assert(remaining.empty());

  std::cout << "[OK] Remove component\n";
}

void testDeferredDestruction() {
  Registry registry;
  Entity e1 = registry.createEntity();
  Entity e2 = registry.createEntity();

  registry.addComponent<Transform2D>(e1, 0.0f, 0.0f);
  registry.addComponent<Transform2D>(e2, 0.0f, 0.0f);

  registry.markForDestruction(e1);
  // Entity should still be alive before flush
  assert(registry.isAlive(e1));

  registry.flushDestroyed();
  assert(!registry.isAlive(e1));
  assert(registry.isAlive(e2));
  assert(registry.view<Transform2D>().size() == 1);

  std::cout << "[OK] Deferred destruction\n";
}

int main() {
  std::cout << "--- ECS Unit Tests ---\n";
  testEntityCreation();
  testComponents();
  testViews();
  testRemoveComponent();
  testDeferredDestruction();
  std::cout << "--- All ECS Tests Passed ---\n";
  return 0;
}
