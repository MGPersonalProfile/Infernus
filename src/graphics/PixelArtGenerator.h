#pragma once
#include "raylib.h"
#include <vector>

class PixelArtGenerator {
public:
    // --- Player classes (full 6x4 FSM sheet, frame 48x56) ---
    static Texture2D getWarrior();
    static Texture2D getRogue();
    static Texture2D getKnight();

    // --- Boss (single-frame, 80x80) ---
    static Texture2D getMinotaur();

    // --- Enemies (single-frame, size varies per type) ---
    static Texture2D getDemon();    // 32x32
    static Texture2D getLancer();   // 24x48
    static Texture2D getBrute();    // 48x48
    static Texture2D getAssassin(); // 24x40
    static Texture2D getBomber();   // 32x32

    // --- Tiles / decor ---
    static Texture2D getFloor();
    static Texture2D getDecor();

private:
    static Texture2D createTextureFromPalette(const std::vector<int>& data, int width, int height, const std::vector<Color>& palette, int targetW = 0, int targetH = 0);
    // Builds a full 6x4 player spritesheet (48x56 per frame) from a 16x16
    // base, using the same walk/attack/dash parameteric animation as warrior.
    // weaponColorIndex: palette index of the "weapon" pixels that will bob
    // up/down during attack frames (x>11 region).
    static Texture2D buildPlayerSheet(const std::vector<int>& pix16, const std::vector<Color>& pal, int weaponColorIndex);
    // Builds a single-frame texture sized targetW x targetH, placing the
    // sprite centered in the target rect. Source pix is srcW x srcH.
    static Texture2D buildSingleFrame(const std::vector<int>& pix, int srcW, int srcH, const std::vector<Color>& pal, int targetW, int targetH);
};
