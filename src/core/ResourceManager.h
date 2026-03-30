#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>

class ResourceManager {
public:
  static ResourceManager &getInstance() {
    static ResourceManager instance;
    return instance;
  }

  ResourceManager(const ResourceManager &) = delete;
  void operator=(const ResourceManager &) = delete;

  // Load a texture from a file path (cached)
  Texture2D getTexture(const std::string &path) {
    if (textures.find(path) == textures.end()) {
      textures[path] = LoadTexture(path.c_str());
    }
    return textures[path];
  }

  // Get or create a solid-color placeholder texture (cached by size+color).
  // Solves the GPU texture leak that occurred when GenImageColor was called
  // every time a particle, projectile, or placeholder entity was spawned.
  Texture2D getPlaceholder(int width, int height, Color color) {
    std::string key = "gen:" + std::to_string(width) + "x" +
                      std::to_string(height) + ":" + std::to_string(color.r) +
                      "," + std::to_string(color.g) + "," +
                      std::to_string(color.b) + "," + std::to_string(color.a);
    if (textures.find(key) == textures.end()) {
      Image img = GenImageColor(width, height, color);
      textures[key] = LoadTextureFromImage(img);
      UnloadImage(img);
    }
    return textures[key];
  }

  void unloadAll() {
    for (auto &[key, tex] : textures) {
      UnloadTexture(tex);
    }
    textures.clear();
  }

private:
  ResourceManager() = default;
  ~ResourceManager() { unloadAll(); }

  std::unordered_map<std::string, Texture2D> textures;
};
