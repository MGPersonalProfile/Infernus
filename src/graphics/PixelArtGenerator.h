#pragma once
#include "raylib.h"
#include <vector>

class PixelArtGenerator {
public:
    // --- Player classes (warrior still procedural; others come from PNG assets) ---
    static Texture2D getWarrior();

    // --- Tiles / decor ---
    static Texture2D getFloor();
    static Texture2D getDecor();

private:
    static Texture2D createTextureFromPalette(const std::vector<int>& data, int width, int height, const std::vector<Color>& palette, int targetW = 0, int targetH = 0);
};
