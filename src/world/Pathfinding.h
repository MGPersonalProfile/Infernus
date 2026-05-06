#pragma once
// A* pathfinding on tile grids — header-only, no external deps.
//
// Usage:
//   auto path = Pathfinding::findPath(grid, {sx, sy}, {ex, ey});
//   if (!path.empty()) { /* path[0] is start, path.back() is end */ }
//
// Walkable check: tile != TileType::WALL. PIT/SPIKE/FIRE_TRAP are walkable
// (enemies will path over them — they take damage from these but still cross).

#include "RoomGenerator.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <vector>
#include <utility>
#include <cstdlib>

namespace Pathfinding {

using Tile = std::pair<int, int>;

inline bool isWalkable(const RoomTemplate &room, int x, int y) {
  if (x < 0 || y < 0 || x >= room.width || y >= room.height) return false;
  return room.grid[x][y] != TileType::WALL;
}

inline int manhattan(Tile a, Tile b) {
  return std::abs(a.first - b.first) + std::abs(a.second - b.second);
}

// Hash for Tile to use in unordered_map
struct TileHash {
  std::size_t operator()(const Tile &t) const noexcept {
    return ((std::size_t)t.first * 73856093u) ^ ((std::size_t)t.second * 19349663u);
  }
};

// Returns path from start to end as a vector of tile coords, inclusive of both.
// Empty vector if no path exists or grid is empty.
inline std::vector<Tile> findPath(const RoomTemplate &room, Tile start, Tile end) {
  std::vector<Tile> result;
  if (!isWalkable(room, start.first, start.second)) return result;
  if (!isWalkable(room, end.first, end.second)) return result;
  if (start == end) { result.push_back(start); return result; }

  struct Node {
    Tile pos;
    int gScore;
    int fScore;
  };
  struct NodeCmp {
    bool operator()(const Node &a, const Node &b) const { return a.fScore > b.fScore; }
  };

  std::priority_queue<Node, std::vector<Node>, NodeCmp> open;
  std::unordered_map<Tile, Tile, TileHash> cameFrom;
  std::unordered_map<Tile, int, TileHash> gScore;

  open.push({start, 0, manhattan(start, end)});
  gScore[start] = 0;

  // 4-directional movement (no diagonals — keeps souls-like feel grounded)
  const int DX[4] = {1, -1, 0, 0};
  const int DY[4] = {0, 0, 1, -1};

  // Hard cap on iterations to prevent runaway in degenerate cases
  int iterations = 0;
  const int MAX_ITER = 4000;

  while (!open.empty() && iterations++ < MAX_ITER) {
    Node current = open.top();
    open.pop();

    if (current.pos == end) {
      // Reconstruct path
      Tile cur = end;
      while (cur != start) {
        result.push_back(cur);
        auto it = cameFrom.find(cur);
        if (it == cameFrom.end()) { result.clear(); return result; }
        cur = it->second;
      }
      result.push_back(start);
      // Reverse so start is first
      std::reverse(result.begin(), result.end());
      return result;
    }

    int curG = gScore[current.pos];
    for (int i = 0; i < 4; i++) {
      Tile next = {current.pos.first + DX[i], current.pos.second + DY[i]};
      if (!isWalkable(room, next.first, next.second)) continue;
      int tentativeG = curG + 1;
      auto it = gScore.find(next);
      if (it != gScore.end() && tentativeG >= it->second) continue;
      cameFrom[next] = current.pos;
      gScore[next] = tentativeG;
      int f = tentativeG + manhattan(next, end);
      open.push({next, tentativeG, f});
    }
  }

  return result; // empty: no path found
}

// Simplified: returns the next waypoint (in tile coords) on the path from start to end,
// or {-1, -1} if no path. Useful when caller only cares about next step.
inline Tile nextWaypoint(const RoomTemplate &room, Tile start, Tile end) {
  auto path = findPath(room, start, end);
  if (path.size() < 2) return {-1, -1};
  return path[1];
}

} // namespace Pathfinding
