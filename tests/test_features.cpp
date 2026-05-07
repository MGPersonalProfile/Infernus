// Smoke tests for systems that don't require the full game subsystem chain.
// Active abilities + combat-on-hit are tested in test_combat.cpp; this file
// covers the standalone-testable systems: A* pathfinding, JSON loader,
// ActiveAbilities cooldown ticking (component-only, no dispatch).

#include "../src/components/ActiveAbility.h"
#include "../src/core/ECS.h"
#include "../src/utils/JsonLoader.h"
#include "../src/world/Pathfinding.h"
#include "../src/world/RoomGenerator.h"
#include "raylib.h"
#include <cassert>
#include <iostream>

// ---------------------------------------------------------------------------
// A* pathfinding
// ---------------------------------------------------------------------------

static RoomTemplate makeOpenRoom(int w, int h) {
  RoomTemplate r;
  r.width = w; r.height = h; r.tileSize = 64;
  // Row-major: grid[y][x] (matches RoomGenerator + LDtkLoader convention)
  r.grid.assign(h, std::vector<TileType>(w, TileType::FLOOR));
  // Outer walls
  for (int x = 0; x < w; x++) {
    r.grid[0][x] = TileType::WALL;
    r.grid[h - 1][x] = TileType::WALL;
  }
  for (int y = 0; y < h; y++) {
    r.grid[y][0] = TileType::WALL;
    r.grid[y][w - 1] = TileType::WALL;
  }
  return r;
}

void testPathfindingDirectLine() {
  auto r = makeOpenRoom(10, 10);
  auto path = Pathfinding::findPath(r, {1, 1}, {8, 1});
  assert(!path.empty() && "Expected a path in open corridor");
  assert(path.front() == std::make_pair(1, 1));
  assert(path.back() == std::make_pair(8, 1));
  // Manhattan distance is 7, plus inclusive endpoints = 8 tiles
  assert((int)path.size() == 8);
  std::cout << "[OK] Pathfinding direct line\n";
}

void testPathfindingAroundWall() {
  // Build a vertical wall at column x=5, rows y=1..7 (row-major: grid[y][5])
  // Path must detour through y=8 corridor.
  auto r = makeOpenRoom(10, 10);
  for (int y = 1; y <= 7; y++) r.grid[y][5] = TileType::WALL;
  auto path = Pathfinding::findPath(r, {2, 4}, {8, 4});
  assert(!path.empty() && "Expected a path around the wall");
  for (auto &t : path) {
    if (t.first == 5) {
      assert((t.second < 1 || t.second > 7) &&
             "Path should not cross wall column at blocked rows");
    }
  }
  std::cout << "[OK] Pathfinding around wall\n";
}

void testPathfindingNoPath() {
  // Block column 5 from y=1 to y=8 inclusive — fully cuts off right half
  // (outer walls close y=0 and y=9 already).
  auto r = makeOpenRoom(10, 10);
  for (int y = 1; y <= 8; y++) r.grid[y][5] = TileType::WALL;
  auto path = Pathfinding::findPath(r, {2, 4}, {8, 4});
  assert(path.empty() && "Expected no path when corridor fully blocked");
  std::cout << "[OK] Pathfinding no path\n";
}

void testPathfindingSamePosition() {
  auto r = makeOpenRoom(10, 10);
  auto path = Pathfinding::findPath(r, {5, 5}, {5, 5});
  assert(path.size() == 1 && path[0] == std::make_pair(5, 5));
  std::cout << "[OK] Pathfinding same-tile\n";
}

// ---------------------------------------------------------------------------
// ActiveAbilities component (cooldown ticking only — no dispatcher needed)
// ---------------------------------------------------------------------------

void testActiveAbilityCooldownTick() {
  ActiveAbilities a;
  a.hasQ = true;
  a.hasE = true;
  a.cooldownQ = 5.0f;
  a.cooldownE = 3.0f;

  assert(!a.readyQ() && "Q should not be ready while on CD");
  assert(!a.readyE() && "E should not be ready while on CD");

  a.tick(2.5f);
  assert(a.cooldownQ == 2.5f);
  assert(a.cooldownE == 0.5f);

  a.tick(1.0f);
  assert(a.cooldownQ == 1.5f);
  assert(a.cooldownE == 0.0f && "E should clamp to 0");
  assert(a.readyE() && "E should be ready after CD expires");

  a.tick(10.0f);
  assert(a.cooldownQ == 0.0f && "Q should clamp to 0 even when overticked");
  assert(a.readyQ());

  std::cout << "[OK] ActiveAbilities cooldown tick + ready check\n";
}

// ---------------------------------------------------------------------------
// JSON loader
// ---------------------------------------------------------------------------

void testJsonLoaderHandlesMissing() {
  nlohmann::json data;
  bool ok = JsonLoader::load("nonexistent_file_xyz.json", data);
  assert(!ok && "Should fail gracefully on missing file");
  assert(data.empty() && "Data should remain empty after failure");
  std::cout << "[OK] JSON loader handles missing file\n";
}

void testJsonLoaderParsesValid() {
  nlohmann::json data;
  bool ok = JsonLoader::load("assets/data/active_abilities.json", data);
  assert(ok && "Should parse a known-valid file");
  assert(data.contains("abilities"));
  assert(data["abilities"].is_array());
  assert(data["abilities"].size() >= 5);
  std::cout << "[OK] JSON loader parses valid file\n";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
  // Some systems use raylib (random, etc.) so init a hidden window
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(1, 1, "test_features");

  std::cout << "--- A* Pathfinding ---\n";
  testPathfindingDirectLine();
  testPathfindingAroundWall();
  testPathfindingNoPath();
  testPathfindingSamePosition();

  std::cout << "--- Active Abilities ---\n";
  testActiveAbilityCooldownTick();

  std::cout << "--- JSON Loader ---\n";
  testJsonLoaderHandlesMissing();
  testJsonLoaderParsesValid();

  std::cout << "--- All Feature Tests Passed ---\n";
  CloseWindow();
  return 0;
}
