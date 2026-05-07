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
  r.grid.assign(w, std::vector<TileType>(h, TileType::FLOOR));
  // Outer walls
  for (int x = 0; x < w; x++) {
    r.grid[x][0] = TileType::WALL;
    r.grid[x][h - 1] = TileType::WALL;
  }
  for (int y = 0; y < h; y++) {
    r.grid[0][y] = TileType::WALL;
    r.grid[w - 1][y] = TileType::WALL;
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
  // Build a wall in the middle and check path goes around it
  auto r = makeOpenRoom(10, 10);
  for (int y = 1; y <= 7; y++) r.grid[5][y] = TileType::WALL;
  // Path from left of wall to right of wall must go around (down through y=8)
  auto path = Pathfinding::findPath(r, {2, 4}, {8, 4});
  assert(!path.empty() && "Expected a path around the wall");
  // Verify no path tile is the wall column at y in [1,7]
  for (auto &t : path) {
    if (t.first == 5) {
      assert((t.second < 1 || t.second > 7) &&
             "Path should not cross wall column at blocked rows");
    }
  }
  std::cout << "[OK] Pathfinding around wall\n";
}

void testPathfindingNoPath() {
  // Surround end with walls — unreachable
  auto r = makeOpenRoom(10, 10);
  for (int y = 1; y <= 8; y++) r.grid[5][y] = TileType::WALL;
  // Add second wall to fully isolate right half
  // (corridor at top/bottom is closed by outer walls)
  auto path = Pathfinding::findPath(r, {2, 4}, {8, 4});
  // Should be empty: no path through fully blocked column
  // (outer walls already closed top y=0 and bottom y=9)
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
