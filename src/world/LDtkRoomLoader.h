#pragma once
// Loads handcrafted rooms from an LDtk project file.
// Desktop only — guarded by INFERNUS_LDTK. Falls back gracefully if missing.

#include <string>
#include <vector>

struct RoomTemplate;

namespace LDtkRoomLoader {

// Load all levels from an LDtk project file. Returns room templates
// compatible with RoomGenerator::instantiate().
// Expected LDtk layer names:
//   "Tiles"        — IntGrid with: 0=floor, 1=wall, 2=pit, 3=spike, 4=fire_trap
//   "Entities"     — Entity layer with "PlayerSpawn" and "EnemySpawn" entities
std::vector<RoomTemplate> loadProject(const std::string& ldtkPath);

// Get a specific room by level name (returns empty RoomTemplate on not found).
RoomTemplate loadLevel(const std::string& ldtkPath, const std::string& levelName);

} // namespace LDtkRoomLoader
