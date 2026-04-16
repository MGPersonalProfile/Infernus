#pragma once
#include "../core/ResourceManager.h"
#include "raylib.h"
#include <cmath>
#include <string>

// Game-wide text rendering using a pixel font (currently VT323).
// All game UI text goes through here for consistent styling.
//
// SCALE: VT323 chars are ~50% the width of PressStart2P at the same point
// size. We multiply requested sizes by FONT_SCALE so existing call sites
// (which were tuned for PressStart2P widths) keep similar visual weight
// and don't need to change. Adjust this single constant to tune.

namespace TextUtils {

constexpr float SPACING = 1.0f;
constexpr float FONT_SCALE = 1.6f;

inline float scaled(int fontSize) { return (float)fontSize * FONT_SCALE; }

// Draw text with the pixel font.
inline void draw(const char *text, int x, int y, int fontSize, Color color) {
  Font &font = ResourceManager::getInstance().fontHUD();
  DrawTextEx(font, text, {(float)x, (float)y}, scaled(fontSize), SPACING, color);
}

// Measure text width with the pixel font.
inline int measure(const char *text, int fontSize) {
  Font &font = ResourceManager::getInstance().fontHUD();
  Vector2 size = MeasureTextEx(font, text, scaled(fontSize), SPACING);
  return (int)std::ceil(size.x);
}

// Draw centered text.
inline void drawCentered(const char *text, int y, int fontSize, Color color,
                         int screenWidth) {
  int tw = measure(text, fontSize);
  draw(text, (screenWidth - tw) / 2, y, fontSize, color);
}

// Draw with shadow (text + offset shadow behind it).
inline void drawShadow(const char *text, int x, int y, int fontSize,
                       Color color, Color shadowColor = {0, 0, 0, 200}) {
  draw(text, x + 2, y + 2, fontSize, shadowColor);
  draw(text, x + 1, y + 1, fontSize, shadowColor);
  draw(text, x, y, fontSize, color);
}

// Draw text with a thick outline (8-directional) for readability on busy backgrounds.
inline void drawOutlined(const char *text, int x, int y, int fontSize,
                         Color color, int thickness = 2,
                         Color outlineColor = {0, 0, 0, 255}) {
  for (int ox = -thickness; ox <= thickness; ox++) {
    for (int oy = -thickness; oy <= thickness; oy++) {
      if (ox == 0 && oy == 0) continue;
      draw(text, x + ox, y + oy, fontSize, outlineColor);
    }
  }
  draw(text, x, y, fontSize, color);
}

// Draw centered with outline.
inline void drawCenteredOutlined(const char *text, int y, int fontSize,
                                 Color color, int screenWidth, int thickness = 2,
                                 Color outlineColor = {0, 0, 0, 255}) {
  int tw = measure(text, fontSize);
  int x = (screenWidth - tw) / 2;
  drawOutlined(text, x, y, fontSize, color, thickness, outlineColor);
}

// Draw centered with shadow.
inline void drawCenteredShadow(const char *text, int y, int fontSize,
                               Color color, int screenWidth,
                               Color shadowColor = {0, 0, 0, 200}) {
  int tw = measure(text, fontSize);
  int x = (screenWidth - tw) / 2;
  drawShadow(text, x, y, fontSize, color, shadowColor);
}

// Truncate text to fit within maxWidth, adding "..." if needed.
inline std::string truncate(const std::string &text, int fontSize, int maxWidth) {
  if (measure(text.c_str(), fontSize) <= maxWidth) return text;
  std::string result = text;
  while (result.size() > 3 && measure((result + "...").c_str(), fontSize) > maxWidth)
    result.pop_back();
  while (!result.empty() && result.back() == ' ') result.pop_back();
  return result + "...";
}

// Draw wrapped text. Returns the final Y position after drawing all lines.
inline int drawWrapped(const char *text, int x, int y, int fontSize, Color color, int maxWidth) {
    std::string s(text);
    std::string currentLine = "";
    int currentY = y;
    int lineSpacing = fontSize + 4; // approximate line height spacing
    
    size_t pos = 0;
    while(pos < s.length()) {
        size_t nextSpace = s.find(' ', pos);
        if(nextSpace == std::string::npos) nextSpace = s.length();
        
        std::string word = s.substr(pos, nextSpace - pos);
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        
        if (measure(testLine.c_str(), fontSize) > maxWidth && !currentLine.empty()) {
            draw(currentLine.c_str(), x, currentY, fontSize, color);
            currentY += lineSpacing;
            currentLine = word;
        } else {
            currentLine = testLine;
        }
        pos = nextSpace + 1;
    }
    if (!currentLine.empty()) {
        draw(currentLine.c_str(), x, currentY, fontSize, color);
        currentY += lineSpacing;
    }
    return currentY;
}

} // namespace TextUtils
