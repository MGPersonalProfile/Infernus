#include "RoomGenerator.h"
#include "../components/Animation.h"
#include "../components/Combat.h"
#include "../components/Destructible.h"
#include "../components/Health.h"
#include "../components/Lifetime.h"
#include "../components/Loot.h"
#include "../components/Trap.h"
#include "raylib.h"

// Tag component for room geometry (walls, obstacles)
struct RoomGeometry : public Component {};

RoomTemplate RoomGenerator::generate(int difficulty, bool isBossRoom) {
  RoomTemplate room;

  if (isBossRoom) {
    room.name = "Sala del Jefe";
    room.width = 24;
    room.height = 16;
  } else {
    room.width = 18 + GetRandomValue(0, 4);
    room.height = 12 + GetRandomValue(0, 3);
    room.name = TextFormat("Sala %d", difficulty + 1);
  }

  room.tileSize = 64;

  room.grid.resize(room.height,
                   std::vector<TileType>(room.width, TileType::FLOOR));

  generateWalls(room);

  if (!isBossRoom)
    generateObstacles(room, difficulty);

  room.playerSpawn = {2, room.height / 2};

  // Guarantee a clear corridor from player spawn (3 tiles wide, first 5 tiles only)
  int spawnY = room.height / 2;
  int clearEnd = (room.width - 1 < 7) ? room.width - 1 : 7;
  for (int x = 1; x < clearEnd; x++) {
    for (int dy = -1; dy <= 1; dy++) {
      int y = spawnY + dy;
      if (y > 0 && y < room.height - 1 &&
          room.grid[y][x] == TileType::WALL)
        room.grid[y][x] = TileType::FLOOR;
    }
  }

  int enemyCount = isBossRoom ? 0 : (2 + difficulty);
  placeSpawnPoints(room, enemyCount);

  return room;
}

void RoomGenerator::generateWalls(RoomTemplate &room) {
  for (int y = 0; y < room.height; y++) {
    for (int x = 0; x < room.width; x++) {
      if (x == 0 || x == room.width - 1 || y == 0 || y == room.height - 1)
        room.grid[y][x] = TileType::WALL;
    }
  }
}

void RoomGenerator::generateObstacles(RoomTemplate &room, int difficulty) {
  int clusters = 2 + difficulty;

  for (int c = 0; c < clusters; c++) {
    int cx = GetRandomValue(4, room.width - 5);
    int cy = GetRandomValue(3, room.height - 4);

    int shape = GetRandomValue(0, 7);

    switch (shape) {
    case 0: // Single pillar (2x2)
      for (int dy = 0; dy < 2; dy++)
        for (int dx = 0; dx < 2; dx++)
          if (cy + dy < room.height - 1 && cx + dx < room.width - 1)
            room.grid[cy + dy][cx + dx] = TileType::WALL;
      break;

    case 1: // Horizontal wall segment
      for (int dx = 0; dx < 3 + GetRandomValue(0, 2); dx++)
        if (cx + dx < room.width - 1)
          room.grid[cy][cx + dx] = TileType::WALL;
      break;

    case 2: // Vertical wall segment
      for (int dy = 0; dy < 3 + GetRandomValue(0, 2); dy++)
        if (cy + dy < room.height - 1)
          room.grid[cy + dy][cx] = TileType::WALL;
      break;

    case 3: // L-shape
      for (int dx = 0; dx < 3; dx++)
        if (cx + dx < room.width - 1)
          room.grid[cy][cx + dx] = TileType::WALL;
      for (int dy = 1; dy < 3; dy++)
        if (cy + dy < room.height - 1)
          room.grid[cy + dy][cx] = TileType::WALL;
      break;

    case 4: // Cross (+)
      if (cy - 1 >= 1)
        room.grid[cy - 1][cx] = TileType::WALL;
      room.grid[cy][cx] = TileType::WALL;
      if (cy + 1 < room.height - 1)
        room.grid[cy + 1][cx] = TileType::WALL;
      if (cx - 1 >= 1)
        room.grid[cy][cx - 1] = TileType::WALL;
      if (cx + 1 < room.width - 1)
        room.grid[cy][cx + 1] = TileType::WALL;
      break;

    case 5: // U-shape (open top)
      for (int dy = 0; dy < 3; dy++) {
        if (cy + dy < room.height - 1) {
          room.grid[cy + dy][cx] = TileType::WALL;
          if (cx + 3 < room.width - 1)
            room.grid[cy + dy][cx + 3] = TileType::WALL;
        }
      }
      if (cy + 2 < room.height - 1)
        for (int dx = 0; dx <= 3 && cx + dx < room.width - 1; dx++)
          room.grid[cy + 2][cx + dx] = TileType::WALL;
      break;

    case 6: // Corridor (two parallel walls)
      for (int dy = 0; dy < 4; dy++) {
        if (cy + dy < room.height - 1) {
          room.grid[cy + dy][cx] = TileType::WALL;
          if (cx + 2 < room.width - 1)
            room.grid[cy + dy][cx + 2] = TileType::WALL;
        }
      }
      break;

    case 7: // Central arena (4 pillars in square)
      for (int corner = 0; corner < 4; corner++) {
        int px = cx + (corner % 2) * 4;
        int py = cy + (corner / 2) * 4;
        if (px < room.width - 1 && py < room.height - 1)
          room.grid[py][px] = TileType::WALL;
      }
      break;
    }
  }

  // Spikes from difficulty 1+
  if (difficulty >= 1) {
    int spikes = GetRandomValue(1, 2 + difficulty);
    for (int s = 0; s < spikes; s++) {
      int sx = GetRandomValue(3, room.width - 4);
      int sy = GetRandomValue(2, room.height - 3);
      if (room.grid[sy][sx] == TileType::FLOOR)
        room.grid[sy][sx] = TileType::SPIKE;
    }
  }

  // Fire traps from difficulty 2+
  if (difficulty >= 2) {
    int fires = GetRandomValue(1, 1 + difficulty / 2);
    for (int f = 0; f < fires; f++) {
      int fx = GetRandomValue(3, room.width - 4);
      int fy = GetRandomValue(2, room.height - 3);
      if (room.grid[fy][fx] == TileType::FLOOR)
        room.grid[fy][fx] = TileType::FIRE_TRAP;
    }
  }

  // Pits from difficulty 4+
  if (difficulty >= 4) {
    int pits = GetRandomValue(1, 2);
    for (int p = 0; p < pits; p++) {
      int px = GetRandomValue(4, room.width - 5);
      int py = GetRandomValue(3, room.height - 4);
      if (room.grid[py][px] == TileType::FLOOR)
        room.grid[py][px] = TileType::PIT;
    }
  }
}

void RoomGenerator::placeSpawnPoints(RoomTemplate &room, int enemyCount) {
  room.enemySpawns.clear();

  for (int i = 0; i < enemyCount; i++) {
    bool placed = false;
    for (int attempt = 0; attempt < 50; attempt++) {
      int sx = GetRandomValue(room.width / 3, room.width - 3);
      int sy = GetRandomValue(2, room.height - 3);

      if (room.grid[sy][sx] == TileType::FLOOR) {
        room.enemySpawns.push_back({sx, sy});
        room.grid[sy][sx] = TileType::SPAWN_POINT;
        placed = true;
        break;
      }
    }
    if (!placed) {
      // Fallback: scan for any floor tile in the right half
      for (int fy = 2; fy < room.height - 2 && !placed; fy++) {
        for (int fx = room.width / 3; fx < room.width - 2 && !placed; fx++) {
          if (room.grid[fy][fx] == TileType::FLOOR) {
            room.enemySpawns.push_back({fx, fy});
            room.grid[fy][fx] = TileType::SPAWN_POINT;
            placed = true;
          }
        }
      }
    }
  }
}

void RoomGenerator::instantiate(Registry &registry, const RoomTemplate &room) {
  currentRoom = room;
  auto &res = ResourceManager::getInstance();

  for (int y = 0; y < room.height; y++) {
    for (int x = 0; x < room.width; x++) {
      TileType tile = room.grid[y][x];
      float wx = tileToWorld(x, room.tileSize);
      float wy = tileToWorld(y, room.tileSize);

      if (tile == TileType::FLOOR || tile == TileType::SPAWN_POINT) {
        // Floor tile
        Entity floor = registry.createEntity();
        registry.addComponent<Transform2D>(floor, wx, wy);
        Texture2D floorTex = res.getTexture("assets/sprites/tiles/floor.png");
        registry.addComponent<Sprite>(floor, floorTex,
                                      Rectangle{0, 0, 64.0f, 64.0f}, 0);
        registry.addComponent<RoomGeometry>(floor);
        roomEntities.push_back(floor);

        // Random floor decorations (15% chance)
        if (GetRandomValue(0, 99) < 15) {
          Entity decor = registry.createEntity();
          registry.addComponent<Transform2D>(decor, wx, wy);
          int decorType = GetRandomValue(0, 3);
          const char *decorPaths[] = {
              "assets/sprites/tiles/decor_crack.png",
              "assets/sprites/tiles/decor_blood.png",
              "assets/sprites/tiles/decor_bones.png",
              "assets/sprites/tiles/decor_rune.png"};
          Texture2D dTex = res.getTexture(decorPaths[decorType]);
          registry.addComponent<Sprite>(decor, dTex,
                                        Rectangle{0, 0, 64.0f, 64.0f}, 1); // fixed to layer 1 to solve z-fighting
          registry.addComponent<RoomGeometry>(decor);
          roomEntities.push_back(decor);
        }

      } else if (tile == TileType::WALL) {
        Entity wall = registry.createEntity();
        registry.addComponent<Transform2D>(wall, wx, wy);
        registry.addComponent<Collider>(wall, (float)room.tileSize,
                                        (float)room.tileSize, false);
        Texture2D tex = res.getTexture("assets/sprites/tiles/wall.png");
        registry.addComponent<Sprite>(wall, tex,
                                      Rectangle{0, 0, 64.0f, 64.0f}, 1);
        registry.addComponent<RoomGeometry>(wall);
        roomEntities.push_back(wall);

        // Torch near some walls (10% chance, only interior-facing walls)
        if (GetRandomValue(0, 99) < 10 && y > 0 && y < room.height - 1) {
          Entity torch = registry.createEntity();
          registry.addComponent<Transform2D>(torch, wx, wy - 32.0f);
          Texture2D tTex = res.getTexture("assets/sprites/tiles/torch.png");
          registry.addComponent<Sprite>(torch, tTex,
                                        Rectangle{0, 0, 16.0f, 32.0f}, 2);
          registry.addComponent<Animation>(torch, 4, 0.15f, 16.0f, 32.0f);
          registry.addComponent<RoomGeometry>(torch);
          roomEntities.push_back(torch);
        }

      } else if (tile == TileType::SPIKE) {
        // Floor under spike
        Entity floor = registry.createEntity();
        registry.addComponent<Transform2D>(floor, wx, wy);
        Texture2D floorTex = res.getTexture("assets/sprites/tiles/floor.png");
        registry.addComponent<Sprite>(floor, floorTex,
                                      Rectangle{0, 0, 64.0f, 64.0f}, 0);
        registry.addComponent<RoomGeometry>(floor);
        roomEntities.push_back(floor);

        Entity spike = registry.createEntity();
        registry.addComponent<Transform2D>(spike, wx, wy);
        registry.addComponent<Collider>(spike, (float)room.tileSize,
                                        (float)room.tileSize, true);
        Texture2D tex = res.getTexture("assets/sprites/tiles/spikes.png");
        registry.addComponent<Sprite>(spike, tex,
                                      Rectangle{0, 0, 64.0f, 64.0f}, 1);
        registry.addComponent<Trap>(spike, TrapType::SPIKE, 10);
        registry.addComponent<RoomGeometry>(spike);
        roomEntities.push_back(spike);

      } else if (tile == TileType::FIRE_TRAP) {
        // Floor under fire trap
        Entity floor = registry.createEntity();
        registry.addComponent<Transform2D>(floor, wx, wy);
        Texture2D floorTex = res.getTexture("assets/sprites/tiles/floor.png");
        registry.addComponent<Sprite>(floor, floorTex,
                                      Rectangle{0, 0, 64.0f, 64.0f}, 0);
        registry.addComponent<RoomGeometry>(floor);
        roomEntities.push_back(floor);

        Entity fire = registry.createEntity();
        registry.addComponent<Transform2D>(fire, wx, wy);
        registry.addComponent<Collider>(fire, (float)room.tileSize,
                                        (float)room.tileSize, true);
        Texture2D tex = res.getTexture("assets/sprites/tiles/fire_trap.png");
        registry.addComponent<Sprite>(fire, tex,
                                      Rectangle{0, 0, 64.0f, 64.0f}, 1);
        registry.addComponent<Animation>(fire, 2, 0.3f, 64.0f, 64.0f);
        registry.addComponent<Trap>(fire, TrapType::FIRE_TRAP, 15);
        registry.addComponent<RoomGeometry>(fire);
        roomEntities.push_back(fire);

      } else if (tile == TileType::PIT) {
        Entity pit = registry.createEntity();
        registry.addComponent<Transform2D>(pit, wx, wy);
        registry.addComponent<Collider>(pit, (float)room.tileSize,
                                        (float)room.tileSize, false);
        Texture2D tex = res.getTexture("assets/sprites/tiles/pit.png");
        registry.addComponent<Sprite>(pit, tex,
                                      Rectangle{0, 0, 64.0f, 64.0f}, 0);
        registry.addComponent<RoomGeometry>(pit);
        roomEntities.push_back(pit);
      }
    }
  }

  // Spawn a few destructible pillars
  int destructibles = GetRandomValue(0, 2);
  for (int d = 0; d < destructibles; d++) {
    int dx = GetRandomValue(4, room.width - 5);
    int dy = GetRandomValue(3, room.height - 4);
    if (room.grid[dy][dx] == TileType::FLOOR) {
      float dwx = tileToWorld(dx, room.tileSize);
      float dwy = tileToWorld(dy, room.tileSize);

      Entity pillar = registry.createEntity();
      registry.addComponent<Transform2D>(pillar, dwx + 16, dwy + 8);
      registry.addComponent<Collider>(pillar, 32.0f, 48.0f, false);
      Texture2D tex = res.getTexture("assets/sprites/tiles/pillar.png");
      registry.addComponent<Sprite>(pillar, tex,
                                    Rectangle{0, 0, 32.0f, 48.0f}, 3);
      registry.addComponent<Health>(pillar, 30);
      registry.addComponent<Destructible>(pillar);
      registry.addComponent<Loot>(pillar, LootType::HEALTH_ORB, 0.7f, 15);
      registry.addComponent<RoomGeometry>(pillar);
      roomEntities.push_back(pillar);
    }
  }
}

void RoomGenerator::clear(Registry &registry) {
  for (Entity e : roomEntities) {
    if (registry.isAlive(e))
      registry.destroyEntity(e);
  }
  roomEntities.clear();
}
