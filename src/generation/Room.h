#pragma once
#include "../components/Collider.h"
#include "../components/Transform.h"
#include "../core/ECS.h"
#include "raylib.h"
#include <vector>


// Basic 2D array room for setup Phase 0-1
class Room {
public:
  int width;
  int height;
  int tileSize;
  std::vector<std::vector<int>> tiles;

  Room(int w, int h, int ts) : width(w), height(h), tileSize(ts) {
    tiles.resize(width, std::vector<int>(height, 0));
  }

  // A fast mockup level design generator
  void generateMockupLevel() {
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++) {
        // Outer boundaries
        if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
          tiles[x][y] = 1; // Solid Wall
        }
        // A few random blocks
        else if (x % 5 == 0 && y % 5 == 0) {
          tiles[x][y] = 1;
        } else {
          tiles[x][y] = 0; // Empty
        }
      }
    }
  }

  void render() {
    for (int x = 0; x < width; x++) {
      for (int y = 0; y < height; y++) {
        if (tiles[x][y] == 1) {
          DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize,
                        DARKGRAY);
          DrawRectangleLines(x * tileSize, y * tileSize, tileSize, tileSize,
                             BLACK);
        } else {
          DrawRectangleLines(x * tileSize, y * tileSize, tileSize, tileSize,
                             Color{40, 40, 40, 255});
        }
      }
    }
  }

  // Check entity collision with tiles
  void checkEntityCollision(Registry &registry) {
    auto entities = registry.view<Transform2D, Collider>();

    for (Entity entity : entities) {
      auto &transform = registry.getComponent<Transform2D>(entity);
      auto &collider = registry.getComponent<Collider>(entity);

      if (collider.isTrigger)
        continue;

      // Simple tilemap AABB collision testing for each corner of the entity's
      // bounding box
      Rectangle eRect = collider.rect;

      // Min/Max Tile Coordinates intersecting the entity
      int minTX = (int)(eRect.x / tileSize);
      int minTY = (int)(eRect.y / tileSize);
      int maxTX = (int)((eRect.x + eRect.width) / tileSize);
      int maxTY = (int)((eRect.y + eRect.height) / tileSize);

      // Clamp
      if (minTX < 0)
        minTX = 0;
      if (minTY < 0)
        minTY = 0;
      if (maxTX >= width)
        maxTX = width - 1;
      if (maxTY >= height)
        maxTY = height - 1;

      // Basic resolution (very simplified, stops horizontal and vertical
      // penetration separately if possible) For a robust system we'd use swept
      // AABB or process axes separately For Fase 1, a basic push-back suffices
      for (int x = minTX; x <= maxTX; x++) {
        for (int y = minTY; y <= maxTY; y++) {
          if (tiles[x][y] == 1) {
            Rectangle tRect = {(float)x * tileSize, (float)y * tileSize,
                               (float)tileSize, (float)tileSize};
            Rectangle intersection = GetCollisionRec(eRect, tRect);

            // Push out based on the smallest penetration depth
            if (intersection.width > 0 && intersection.height > 0) {
              if (intersection.width < intersection.height) {
                // Horizontal push
                if (eRect.x < tRect.x)
                  transform.x -= intersection.width;
                else
                  transform.x += intersection.width;
              } else {
                // Vertical push
                if (eRect.y < tRect.y)
                  transform.y -= intersection.height;
                else
                  transform.y += intersection.height;
              }
              // Sync collider bounds again after resolution
              collider.rect.x = transform.x + collider.offsetX;
              collider.rect.y = transform.y + collider.offsetY;
            }
          }
        }
      }
    }
  }
};
