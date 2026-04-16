#pragma once
#include "raylib.h"
#include <string>
#include <vector>

enum class RoomType { COMBAT, ELITE, SHOP, REST, BOSS };

struct MapNode {
  RoomType type = RoomType::COMBAT;
  int layer = 0;           // depth in the graph (0 = start)
  int index = 0;           // position within the layer
  bool visited = false;
  bool current = false;
  std::vector<int> connections; // indices into next layer's nodes

  // Difficulty for combat/elite rooms (maps to old currentRoom)
  int difficulty = 0;

  const char *label() const {
    switch (type) {
    case RoomType::COMBAT: return "Combate";
    case RoomType::ELITE:  return "Elite";
    case RoomType::SHOP:   return "Tienda";
    case RoomType::REST:   return "Descanso";
    case RoomType::BOSS:   return "Jefe";
    }
    return "???";
  }

  Color color() const {
    switch (type) {
    case RoomType::COMBAT: return Color{200, 60, 60, 255};
    case RoomType::ELITE:  return Color{220, 180, 40, 255};
    case RoomType::SHOP:   return Color{60, 180, 60, 255};
    case RoomType::REST:   return Color{60, 120, 220, 255};
    case RoomType::BOSS:   return Color{200, 40, 200, 255};
    }
    return WHITE;
  }
};

// A run's map: layers of nodes forming a directed acyclic graph.
// Layer 0 = start (1 combat), last layer = boss (1 node).
// Player picks one node per layer, advancing toward the boss.
class RunMap {
public:
  std::vector<std::vector<MapNode>> layers;
  int currentLayer = 0;
  int currentIndex = 0;     // selected node within currentLayer
  int cursorIndex = 0;      // cursor for MAP_SELECT screen

  void generate() {
    layers.clear();
    currentLayer = 0;
    currentIndex = 0;
    cursorIndex = 0;

    // Layer 0: single combat room (intro)
    layers.push_back({makeNode(RoomType::COMBAT, 0, 0, 0)});
    layers[0][0].connections = {0, 1}; // connect to both nodes in layer 1

    // Layer 1: 2 choices
    {
      std::vector<MapNode> l;
      l.push_back(makeNode(randomNonBoss(1), 1, 0, 1));
      l.push_back(makeNode(randomNonBoss(1), 1, 1, 1));
      // Ensure at least one combat
      if (l[0].type != RoomType::COMBAT && l[1].type != RoomType::COMBAT)
        l[GetRandomValue(0, 1)].type = RoomType::COMBAT;
      l[0].connections = {0, 1};
      l[1].connections = {0, 1, 2};
      layers.push_back(l);
    }

    // Layer 2: 3 choices
    {
      std::vector<MapNode> l;
      l.push_back(makeNode(randomNonBoss(2), 2, 0, 2));
      l.push_back(makeNode(randomNonBoss(2), 2, 1, 2));
      l.push_back(makeNode(randomNonBoss(2), 2, 2, 2));
      // Ensure at least one combat/elite
      bool hasFight = false;
      for (auto &n : l)
        if (n.type == RoomType::COMBAT || n.type == RoomType::ELITE)
          hasFight = true;
      if (!hasFight)
        l[GetRandomValue(0, 2)].type = RoomType::ELITE;
      l[0].connections = {0};
      l[1].connections = {0, 1};
      l[2].connections = {1};
      layers.push_back(l);
    }

    // Layer 3: 2 choices (harder — combat or elite only)
    {
      std::vector<MapNode> l;
      RoomType t0 = GetRandomValue(0, 1) ? RoomType::ELITE : RoomType::COMBAT;
      RoomType t1 = GetRandomValue(0, 1) ? RoomType::COMBAT : RoomType::ELITE;
      l.push_back(makeNode(t0, 3, 0, 3));
      l.push_back(makeNode(t1, 3, 1, 3));
      l[0].connections = {0};
      l[1].connections = {0};
      layers.push_back(l);
    }

    // Layer 4: boss
    layers.push_back({makeNode(RoomType::BOSS, 4, 0, 4)});

    // Mark first room as current
    layers[0][0].current = true;
  }

  MapNode &current() { return layers[currentLayer][currentIndex]; }

  // Get the reachable nodes in the next layer from the current node
  std::vector<int> nextChoices() const {
    if (currentLayer >= (int)layers.size() - 1) return {};
    return layers[currentLayer][currentIndex].connections;
  }

  // Advance to a chosen node in the next layer
  void advanceTo(int nextIndex) {
    layers[currentLayer][currentIndex].current = false;
    layers[currentLayer][currentIndex].visited = true;
    currentLayer++;
    currentIndex = nextIndex;
    cursorIndex = 0;
    layers[currentLayer][currentIndex].current = true;
  }

  bool isBossNext() const {
    if (currentLayer + 1 >= (int)layers.size()) return false;
    return layers[currentLayer + 1].size() == 1 &&
           layers[currentLayer + 1][0].type == RoomType::BOSS;
  }

  bool isLastLayer() const {
    return currentLayer >= (int)layers.size() - 1;
  }

  int totalLayers() const { return (int)layers.size(); }

private:
  static MapNode makeNode(RoomType type, int layer, int index, int difficulty) {
    MapNode n;
    n.type = type;
    n.layer = layer;
    n.index = index;
    n.difficulty = difficulty;
    return n;
  }

  static RoomType randomNonBoss(int layer) {
    // Weights depend on layer depth
    int roll = GetRandomValue(0, 99);
    if (layer <= 1) {
      // Early: mostly combat, some shop/rest
      if (roll < 50) return RoomType::COMBAT;
      if (roll < 75) return RoomType::SHOP;
      return RoomType::REST;
    } else {
      // Later: combat/elite/shop/rest
      if (roll < 35) return RoomType::COMBAT;
      if (roll < 55) return RoomType::ELITE;
      if (roll < 80) return RoomType::SHOP;
      return RoomType::REST;
    }
  }
};
