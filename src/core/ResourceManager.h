#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>
#include "../graphics/PixelArtGenerator.h"

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
      if (path == "assets/sprites/player/warrior_idle.png") {
        textures[path] = PixelArtGenerator::getWarrior();
      } else if (path == "assets/sprites/tiles/floor.png") {
        textures[path] = PixelArtGenerator::getFloor();
      } else if (path == "assets/sprites/tiles/decor_bones.png") {
        textures[path] = PixelArtGenerator::getDecor();
      } else if (path == "assets/sprites/tiles/decor_rune.png") {
        textures[path] = PixelArtGenerator::getDecor(); // fallback
      } else {
        // Real PNG on disk — load it and force nearest-neighbor filter so
        // Antigravity's pixel art doesn't get bilinear-blurred by Raylib's
        // default TEXTURE_FILTER_BILINEAR.
        textures[path] = LoadTexture(path.c_str());
        SetTextureFilter(textures[path], TEXTURE_FILTER_POINT);
      }
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

  // Codepoints to load: ASCII printable (32-126) + Latin-1 supplement
  // (160-255) which covers áéíóúñÁÉÍÓÚÑ¿¡, plus em-dash and a few extras.
  // Without this, LoadFontEx defaults to ASCII-only and Spanish characters
  // render as blank/question marks.
  static const int *getCodepoints(int *outCount) {
    static int cps[256];
    int n = 0;
    for (int c = 32; c <= 126; c++) cps[n++] = c;       // ASCII
    for (int c = 160; c <= 255; c++) cps[n++] = c;      // Latin-1 supplement
    cps[n++] = 0x2014;                                  // em dash —
    cps[n++] = 0x2013;                                  // en dash –
    cps[n++] = 0x2018; cps[n++] = 0x2019;               // ' '
    cps[n++] = 0x201C; cps[n++] = 0x201D;               // " "
    cps[n++] = 0x2026;                                  // …
    *outCount = n;
    return cps;
  }

  // Path to the active UI font (single source of truth)
  static const char *uiFontPath() {
    return "assets/fonts/VT323-Regular.ttf";
  }

  // Load a font from file (cached). fontSize = base size for SDF quality.
  Font getFont(const std::string &path, int fontSize = 32) {
    std::string key = path + ":" + std::to_string(fontSize);
    if (fonts.find(key) == fonts.end()) {
      int n = 0;
      const int *cps = getCodepoints(&n);
      fonts[key] = LoadFontEx(path.c_str(), fontSize, (int *)cps, n);
      SetTextureFilter(fonts[key].texture, TEXTURE_FILTER_POINT);
    }
    return fonts[key];
  }

  // Quick access to the game's main fonts (call after init)
  Font &fontHUD() {
    if (!hud.baseSize) {
      int n = 0; const int *cps = getCodepoints(&n);
      hud = LoadFontEx(uiFontPath(), 32, (int *)cps, n);
      SetTextureFilter(hud.texture, TEXTURE_FILTER_POINT);
    }
    return hud;
  }
  Font &fontTitle() {
    if (!title.baseSize) {
      int n = 0; const int *cps = getCodepoints(&n);
      title = LoadFontEx(uiFontPath(), 48, (int *)cps, n);
      SetTextureFilter(title.texture, TEXTURE_FILTER_POINT);
    }
    return title;
  }
  Font &fontBig() {
    if (!big.baseSize) {
      int n = 0; const int *cps = getCodepoints(&n);
      big = LoadFontEx(uiFontPath(), 64, (int *)cps, n);
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
