#include "LDtkRoomLoader.h"
#include "RoomGenerator.h"

#ifdef INFERNUS_LDTK

#include <LDtkLoader/Project.hpp>
#include "raylib.h"

namespace LDtkRoomLoader {

static TileType intToTile(int val) {
    switch (val) {
        case 0: return TileType::FLOOR;
        case 1: return TileType::WALL;
        case 2: return TileType::PIT;
        case 3: return TileType::SPIKE;
        case 4: return TileType::FIRE_TRAP;
        default: return TileType::FLOOR;
    }
}

static RoomTemplate convertLevel(const ldtk::Level& level) {
    RoomTemplate room;
    room.name = level.name;

    // Find the IntGrid layer "Tiles"
    const ldtk::Layer* tilesLayer = nullptr;
    const ldtk::Layer* entitiesLayer = nullptr;
    for (auto& layer : level.allLayers()) {
        if (layer.getName() == "Tiles")    tilesLayer = &layer;
        if (layer.getName() == "Entities") entitiesLayer = &layer;
    }

    // Grid dimensions from level size and cell size
    auto size = level.size;
    int cellSize = 64; // default
    if (tilesLayer) {
        cellSize = tilesLayer->getCellSize();
    }
    room.tileSize = cellSize;
    room.width  = size.x / cellSize;
    room.height = size.y / cellSize;

    // Build tile grid
    room.grid.resize(room.height, std::vector<TileType>(room.width, TileType::FLOOR));
    if (tilesLayer) {
        for (int y = 0; y < room.height; ++y) {
            for (int x = 0; x < room.width; ++x) {
                auto cell = tilesLayer->getIntGridVal(x, y);
                room.grid[y][x] = intToTile(cell.value);
            }
        }
    }

    // Extract entity spawns
    if (entitiesLayer) {
        for (auto& entity : entitiesLayer->allEntities()) {
            int tx = (int)entity.getPosition().x / cellSize;
            int ty = (int)entity.getPosition().y / cellSize;
            if (entity.getName() == "PlayerSpawn") {
                room.playerSpawn = {tx, ty};
            } else if (entity.getName() == "EnemySpawn") {
                room.enemySpawns.push_back({tx, ty});
            }
        }
    }

    return room;
}

std::vector<RoomTemplate> loadProject(const std::string& ldtkPath) {
    std::vector<RoomTemplate> rooms;
    try {
        ldtk::Project project;
        project.loadFromFile(ldtkPath);
        for (auto& world : project.allWorlds()) {
            for (auto& level : world.allLevels()) {
                rooms.push_back(convertLevel(level));
            }
        }
        TraceLog(LOG_INFO, "LDTK: Loaded %d rooms from %s",
                 (int)rooms.size(), ldtkPath.c_str());
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "LDTK: Failed to load %s: %s",
                 ldtkPath.c_str(), e.what());
    }
    return rooms;
}

RoomTemplate loadLevel(const std::string& ldtkPath, const std::string& levelName) {
    try {
        ldtk::Project project;
        project.loadFromFile(ldtkPath);
        for (auto& world : project.allWorlds()) {
            for (auto& level : world.allLevels()) {
                if (level.name == levelName) {
                    return convertLevel(level);
                }
            }
        }
        TraceLog(LOG_WARNING, "LDTK: Level '%s' not found in %s",
                 levelName.c_str(), ldtkPath.c_str());
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "LDTK: Failed to load %s: %s",
                 ldtkPath.c_str(), e.what());
    }
    return RoomTemplate{};
}

} // namespace LDtkRoomLoader

#else // !INFERNUS_LDTK — stubs

namespace LDtkRoomLoader {
std::vector<RoomTemplate> loadProject(const std::string&) { return {}; }
RoomTemplate loadLevel(const std::string&, const std::string&) { return RoomTemplate{}; }
}

#endif
