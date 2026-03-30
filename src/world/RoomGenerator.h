#pragma once
#include "../core/ECS.h"
#include "../core/ResourceManager.h"
#include "../components/Collider.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "raylib.h"
#include <string>
#include <vector>

// Types of tiles in a room
enum class TileType { FLOOR, WALL, PIT, SPIKE, FIRE_TRAP, SPAWN_POINT, EXIT };

// A room template defines the layout
struct RoomTemplate {
  std::string name;
  int width = 20;   // in tiles
  int height = 12;  // in tiles
  int tileSize = 64;
  std::vector<std::vector<TileType>> grid;

  // Spawn locations for enemies (in tile coords)
  std::vector<std::pair<int, int>> enemySpawns;
  // Player spawn (in tile coords)
  std::pair<int, int> playerSpawn = {1, 6};
};

class RoomGenerator {
public:
  // Generate a random room template for the given difficulty
  RoomTemplate generate(int difficulty, bool isBossRoom);

  // Instantiate the room as entities in the world (walls, obstacles)
  void instantiate(Registry &registry, const RoomTemplate &room);

  // Clear all room entities
  void clear(Registry &registry);

  // Get world-space position of a tile
  static float tileToWorld(int tile, int tileSize) {
    return (float)(tile * tileSize);
  }

  // Get room pixel dimensions
  int getPixelWidth() const { return currentRoom.width * currentRoom.tileSize; }
  int getPixelHeight() const {
    return currentRoom.height * currentRoom.tileSize;
  }

  const RoomTemplate &getCurrentRoom() const { return currentRoom; }

private:
  RoomTemplate currentRoom;
  std::vector<Entity> roomEntities;

  void generateWalls(RoomTemplate &room);
  void generateObstacles(RoomTemplate &room, int difficulty);
  void placeSpawnPoints(RoomTemplate &room, int enemyCount);
};
