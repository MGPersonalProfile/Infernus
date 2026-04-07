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

  // Load a font from file (cached). fontSize = base size for SDF quality.
  Font getFont(const std::string &path, int fontSize = 32) {
    std::string key = path + ":" + std::to_string(fontSize);
    if (fonts.find(key) == fonts.end()) {
      fonts[key] = LoadFontEx(path.c_str(), fontSize, nullptr, 0);
      SetTextureFilter(fonts[key].texture, TEXTURE_FILTER_POINT);
    }
    return fonts[key];
  }

  // Quick access to the game's main fonts (call after init)
  Font &fontHUD() {
    if (!hud.baseSize) {
      hud = LoadFontEx("assets/fonts/PressStart2P-Regular.ttf", 16, nullptr, 0);
      SetTextureFilter(hud.texture, TEXTURE_FILTER_POINT);
    }
    return hud;
  }
  Font &fontTitle() {
    if (!title.baseSize) {
      title = LoadFontEx("assets/fonts/PressStart2P-Regular.ttf", 32, nullptr, 0);
      SetTextureFilter(title.texture, TEXTURE_FILTER_POINT);
    }
    return title;
  }
  Font &fontBig() {
    if (!big.baseSize) {
      big = LoadFontEx("assets/fonts/PressStart2P-Regular.ttf", 48, nullptr, 0);
      SetTextureFilter(big.texture, TEXTURE_FILTER_POINT);
    }
    return big;
  }

  void unloadAll() {
    for (auto &[key, tex] : textures) {
      UnloadTexture(tex);
    }
    textures.clear();
    for (auto &[key, f] : fonts) {
      UnloadFont(f);
    }
    fonts.clear();
    if (hud.baseSize) { UnloadFont(hud); hud = {}; }
    if (title.baseSize) { UnloadFont(title); title = {}; }
    if (big.baseSize) { UnloadFont(big); big = {}; }
  }

private:
  ResourceManager() = default;
  ~ResourceManager() { unloadAll(); }

  std::unordered_map<std::string, Texture2D> textures;
  std::unordered_map<std::string, Font> fonts;
  Font hud = {};
  Font title = {};
  Font big = {};
};
