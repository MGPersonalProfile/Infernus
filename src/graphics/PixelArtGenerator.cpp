#include "PixelArtGenerator.h"

Texture2D PixelArtGenerator::createTextureFromPalette(const std::vector<int>& data, int width, int height, const std::vector<Color>& palette, int targetW, int targetH) {
    Image img = GenImageColor(width, height, BLANK);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int colorIndex = data[y * width + x];
            if (colorIndex >= 0 && colorIndex < (int)palette.size()) {
                ImageDrawPixel(&img, x, y, palette[colorIndex]);
            }
        }
    }
    if (targetW > 0 && targetH > 0) {
        ImageResizeNN(&img, targetW, targetH);
    }
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT); // Keep it sharp!
    UnloadImage(img);
    return tex;
}

Texture2D PixelArtGenerator::getWarrior() {
    std::vector<Color> pal = {
        BLANK,                      // 0: Transparent
        Color{30, 30, 35, 255},     // 1: Dark Iron (Armor Outline)
        Color{100, 100, 110, 255},  // 2: Steel Armor Base
        Color{160, 160, 170, 255},  // 3: Steel Armor Highlight
        Color{180, 20, 20, 255},    // 4: Red Plume/Cape
        Color{220, 190, 150, 255},  // 5: Skin/Face
        Color{200, 200, 200, 255}   // 6: Blade Silver
    };
// 16x16 Knight sprite data. 4 Rows.
    // Row 0: Idle (2 frames)
    // Row 1: Walk (6 frames) -> We will make 4 frames loop (Walk cycle)
    // Row 2: Attack (3 frames)
    // Row 3: Dash/Ulti (2 frames)
    // 6 Frames Max width.
    std::vector<int> pix = {
        0,0,0,0,0, 0,4,4,4,4, 0,0,0,0,0,0,
        0,0,0,0,0, 4,4,4,4,1, 0,0,0,0,0,0,
        0,0,0,0,0, 1,2,2,2,1, 0,0,0,0,0,0,
        0,0,0,0,0, 1,5,5,1,1, 0,0,0,1,0,0,
        0,0,0,0,0, 1,3,2,2,1, 0,0,0,1,0,0,
        0,0,0,0,1, 1,1,2,1,1, 1,0,0,1,0,0,
        0,0,0,1,2, 1,2,3,2,1, 2,1,1,6,1,0,
        0,0,0,1,2, 2,2,2,2,2, 2,1,1,6,1,0,
        0,0,0,1,1, 2,2,2,2,2, 1,1,1,6,1,0,
        0,0,0,4,4, 1,2,1,2,1, 0,0,1,6,1,0,
        0,0,4,4,4, 1,1,0,1,1, 0,0,1,6,1,0,
        0,0,4,4,0, 1,2,0,2,1, 0,0,1,6,1,0,
        0,0,4,0,0, 1,1,0,1,1, 0,0,1,1,1,0,
        0,0,0,0,0, 1,2,0,2,1, 0,0,0,0,0,0,
        0,0,0,0,0, 1,1,0,1,1, 0,0,0,0,0,0,
        0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,0
    };
    
    // We will procedurally construct the entire spritesheet by moving the pixels!
    Image img = GenImageColor(16 * 6, 16 * 4, BLANK);
    
    // Helper lambda to draw a frame
    auto drawFrame = [&](int frameX, int frameY, int offsetY, bool bladeUp, bool legForward) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                int c = pix[y * 16 + x];
                if(c > 0 && c < (int)pal.size()) {
                    int drawX = frameX * 16 + x;
                    int drawY = frameY * 16 + y + offsetY;
                    
                    // Simple animation adjustments over base frame
                    if (y >= 13 && legForward && x > 4 && x < 10) drawX += 1; // move leg
                    if (x > 11 && c == 6 && bladeUp) drawY -= 2; // raise blade
                    
                    if (drawX >= 0 && drawX < 16 * 6 && drawY >= 0 && drawY < 16 * 4) {
                        ImageDrawPixel(&img, drawX, drawY, pal[c]);
                    }
                }
            }
        }
    };
    
    // Row 0: Idle (2 frames)
    drawFrame(0, 0, 0, false, false);
    drawFrame(1, 0, 1, false, false); // bob down

    // Row 1: Walk (6 frames) -> loop walking animation
    drawFrame(0, 1, 0, false, true);
    drawFrame(1, 1, -1, true, false); 
    drawFrame(2, 1, 0, false, true);
    drawFrame(3, 1, -1, false, false);
    drawFrame(4, 1, 0, true, true);
    drawFrame(5, 1, -1, false, false);

    // Row 2: Attack (3 frames)
    drawFrame(0, 2, 0, true, false); // windup
    drawFrame(1, 2, 0, true, true);  // swing
    // For hit impact, we just draw red sword 
    drawFrame(2, 2, 1, false, true); 

    // Row 3: Dash/Ulti (2 frames)
    drawFrame(0, 3, 0, true, true);
    drawFrame(1, 3, 0, true, true);

    // After we draw our full 16x16-based animation sheet, 
    // it's 96 x 64 pure pixels.
    // The original Game logic expects frames to be 48 width x 56 height per frame.
    // So if 1 frame = 48x56, then 6 frames = 288x... Wait! 
    // If the game expects each block to be 48x56 natively, we should scale correctly.
    // Our logical frame is 16x16. 
    // Wait, let's scale the whole sheet up by 3x so one frame is 48x48.
    // Then canvas it by 56x vertically.
    // Actually, ImageResizeNN the entire thing by 3x on both axes!
    // 96 * 3 = 288 width. 64 * 3.5 = 224.
    // Let's canvas the frames directly.
    Image finalImg = GenImageColor(48 * 6, 56 * 4, BLANK);
    for (int frameY = 0; frameY < 4; ++frameY) {
        for (int frameX = 0; frameX < 6; ++frameX) {
            // Cut 16x16 from img
            Rectangle srcRec = {(float)(frameX * 16), (float)(frameY * 16), 16.0f, 16.0f};
            Image frameImg = ImageFromImage(img, srcRec);
            // Scale to 48x48
            ImageResizeNN(&frameImg, 48, 48);
            // Draw to final sheet at center
            Rectangle dstRec = {(float)(frameX * 48), (float)(frameY * 56 + 8), 48.0f, 48.0f};
            ImageDraw(&finalImg, frameImg, {0,0,48,48}, dstRec, WHITE);
            UnloadImage(frameImg);
        }
    }
    
    Texture2D tex = LoadTextureFromImage(finalImg);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    UnloadImage(finalImg);
    return tex;
}

// =============================================================
// Shared helpers for new generators
// =============================================================

Texture2D PixelArtGenerator::buildPlayerSheet(const std::vector<int>& pix, const std::vector<Color>& pal, int weaponColorIndex) {
    // Mirrors the warrior pattern: 16x16 base sprite becomes a full 6x4 FSM
    // spritesheet (48x56 per frame). Row0 idle, Row1 walk, Row2 attack, Row3 dash.
    Image img = GenImageColor(16 * 6, 16 * 4, BLANK);

    auto drawFrame = [&](int frameX, int frameY, int offsetY, bool bladeUp, bool legForward) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                int c = pix[y * 16 + x];
                if (c > 0 && c < (int)pal.size()) {
                    int drawX = frameX * 16 + x;
                    int drawY = frameY * 16 + y + offsetY;
                    if (y >= 13 && legForward && x > 4 && x < 10) drawX += 1;
                    if (x > 11 && c == weaponColorIndex && bladeUp) drawY -= 2;
                    if (drawX >= 0 && drawX < 16 * 6 && drawY >= 0 && drawY < 16 * 4) {
                        ImageDrawPixel(&img, drawX, drawY, pal[c]);
                    }
                }
            }
        }
    };

    // Row 0: Idle (2 frames, subtle bob)
    drawFrame(0, 0, 0, false, false);
    drawFrame(1, 0, 1, false, false);
    // Row 1: Walk (6 frames loop)
    drawFrame(0, 1, 0, false, true);
    drawFrame(1, 1, -1, true, false);
    drawFrame(2, 1, 0, false, true);
    drawFrame(3, 1, -1, false, false);
    drawFrame(4, 1, 0, true, true);
    drawFrame(5, 1, -1, false, false);
    // Row 2: Attack (3 frames: windup, swing, recover)
    drawFrame(0, 2, 0, true, false);
    drawFrame(1, 2, 0, true, true);
    drawFrame(2, 2, 1, false, true);
    // Row 3: Dash (2 frames)
    drawFrame(0, 3, 0, true, true);
    drawFrame(1, 3, 0, true, true);

    // Scale to final 48x56 cells
    Image finalImg = GenImageColor(48 * 6, 56 * 4, BLANK);
    for (int frameY = 0; frameY < 4; ++frameY) {
        for (int frameX = 0; frameX < 6; ++frameX) {
            Rectangle srcRec = {(float)(frameX * 16), (float)(frameY * 16), 16.0f, 16.0f};
            Image frameImg = ImageFromImage(img, srcRec);
            ImageResizeNN(&frameImg, 48, 48);
            Rectangle dstRec = {(float)(frameX * 48), (float)(frameY * 56 + 8), 48.0f, 48.0f};
            ImageDraw(&finalImg, frameImg, {0,0,48,48}, dstRec, WHITE);
            UnloadImage(frameImg);
        }
    }

    Texture2D tex = LoadTextureFromImage(finalImg);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    UnloadImage(finalImg);
    return tex;
}

Texture2D PixelArtGenerator::buildSingleFrame(const std::vector<int>& pix, int srcW, int srcH, const std::vector<Color>& pal, int targetW, int targetH) {
    Image src = GenImageColor(srcW, srcH, BLANK);
    for (int y = 0; y < srcH; ++y) {
        for (int x = 0; x < srcW; ++x) {
            int c = pix[y * srcW + x];
            if (c > 0 && c < (int)pal.size()) {
                ImageDrawPixel(&src, x, y, pal[c]);
            }
        }
    }
    ImageResizeNN(&src, targetW, targetH);
    Texture2D tex = LoadTextureFromImage(src);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(src);
    return tex;
}

// =============================================================
// ROGUE — dark hood, purple cloak, twin daggers
// =============================================================
Texture2D PixelArtGenerator::getRogue() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{15, 15, 22, 255},      // 1 dark outline / hood
        Color{72, 38, 100, 255},     // 2 purple cloak base
        Color{115, 70, 150, 255},    // 3 purple highlight
        Color{210, 180, 155, 255},   // 4 pale skin
        Color{220, 220, 230, 255},   // 5 silver (used elsewhere)
        Color{255, 60, 60, 255},     // 6 glowing red eye
        Color{230, 230, 240, 255}    // 7 dagger blade (weapon color)
    };
    std::vector<int> pix = {
        0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,
        0,0,0,0,1,1,3,3,3,1,1,0,0,0,0,0,
        0,0,0,1,3,4,4,4,4,4,1,0,0,0,0,0,
        0,0,0,1,4,4,6,4,4,6,4,1,0,0,0,0,
        0,0,0,1,1,4,4,4,4,4,1,0,0,0,0,0,
        0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,
        0,0,0,2,2,3,2,2,3,2,2,0,7,0,0,0,
        0,0,0,2,3,3,2,2,3,3,2,0,7,7,0,0,
        0,0,0,1,2,2,2,2,2,2,1,1,7,7,7,0,
        0,0,0,1,2,2,3,3,2,2,1,0,0,7,7,0,
        0,0,0,0,2,2,2,2,2,2,0,0,0,0,7,0,
        0,0,0,0,2,2,0,0,2,2,0,0,0,0,0,0,
        0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,
        0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,
        0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildPlayerSheet(pix, pal, 7); // weapon = index 7 (dagger)
}

// =============================================================
// KNIGHT — gold plate, white plume, tall sword
// =============================================================
Texture2D PixelArtGenerator::getKnight() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{25, 20, 10, 255},      // 1 dark outline
        Color{190, 150, 50, 255},    // 2 gold base
        Color{240, 210, 110, 255},   // 3 gold highlight
        Color{245, 245, 240, 255},   // 4 white plume
        Color{220, 190, 155, 255},   // 5 skin (face)
        Color{200, 200, 215, 255}    // 6 steel blade (weapon color)
    };
    std::vector<int> pix = {
        0,0,0,0,0,0,4,4,4,4,0,0,0,0,0,0,
        0,0,0,0,0,4,4,4,4,4,0,0,0,0,0,0,
        0,0,0,0,1,1,2,2,2,2,1,0,0,0,0,0,
        0,0,0,0,1,2,3,3,2,2,1,0,0,0,0,0,
        0,0,0,0,1,2,5,2,2,5,1,0,0,6,0,0,
        0,0,0,0,1,2,2,2,2,2,1,0,0,6,0,0,
        0,0,0,1,1,1,1,1,1,1,1,0,1,6,1,0,
        0,0,0,1,2,3,3,3,3,2,1,1,1,6,1,0,
        0,0,0,1,2,3,3,3,3,2,1,1,1,6,1,0,
        0,0,0,1,2,2,2,2,2,2,1,0,1,6,1,0,
        0,0,0,1,2,3,3,3,3,2,1,0,1,6,1,0,
        0,0,0,1,2,2,2,2,2,2,1,0,1,1,1,0,
        0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,
        0,0,0,0,1,2,0,0,2,1,0,0,0,0,0,0,
        0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildPlayerSheet(pix, pal, 6); // weapon = index 6 (steel blade)
}

// =============================================================
// MINOTAUR BOSS — horns, battle-axe, brown fur, 80x80
// =============================================================
Texture2D PixelArtGenerator::getMinotaur() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{15, 10, 5, 255},       // 1 outline
        Color{92, 58, 32, 255},      // 2 brown fur
        Color{140, 90, 52, 255},     // 3 brown highlight
        Color{235, 215, 170, 255},   // 4 horn cream
        Color{235, 40, 40, 255},     // 5 red eye
        Color{170, 170, 185, 255},   // 6 axe steel
        Color{230, 190, 60, 255}     // 7 nose ring gold
    };
    std::vector<int> pix = {
        0,0,4,4,0,0,0,0,0,0,0,4,4,0,0,0,
        0,4,4,1,0,0,0,0,0,0,1,4,4,0,0,0,
        0,0,1,1,2,2,2,2,2,2,1,1,0,0,0,0,
        0,0,1,2,5,2,2,2,2,5,2,1,0,0,0,0,
        0,0,1,2,2,2,7,7,2,2,2,1,0,0,0,0,
        0,0,0,1,2,2,2,2,2,2,1,0,0,0,0,0,
        0,0,1,1,2,3,2,2,3,2,1,1,0,0,0,0,
        0,0,1,2,2,2,2,2,2,2,2,1,0,0,6,0,
        0,0,1,2,3,3,2,2,3,3,2,1,0,6,6,6,
        0,0,1,2,3,3,2,2,3,3,2,1,0,6,6,6,
        0,0,1,2,2,2,2,2,2,2,2,1,0,6,6,6,
        0,0,1,2,2,2,2,2,2,2,2,1,0,0,6,0,
        0,0,0,1,2,2,0,0,2,2,1,0,0,0,0,0,
        0,0,0,1,2,2,0,0,2,2,1,0,0,0,0,0,
        0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildSingleFrame(pix, 16, 16, pal, 80, 80);
}

// =============================================================
// DEMON — red horned melee, 32x32
// =============================================================
Texture2D PixelArtGenerator::getDemon() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{5, 0, 0, 255},         // 1 black outline
        Color{180, 30, 25, 255},     // 2 red base
        Color{225, 60, 50, 255},     // 3 red highlight
        Color{255, 215, 50, 255},    // 4 yellow eye
        Color{225, 225, 225, 255},   // 5 white claw
        Color{90, 15, 10, 255},      // 6 dark mouth
        Color{240, 240, 220, 255}    // 7 fang
    };
    std::vector<int> pix = {
        0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,
        0,1,2,1,0,0,0,0,0,0,0,1,2,1,0,0,
        0,1,2,2,1,2,2,2,2,2,2,1,2,2,1,0,
        0,1,2,3,4,2,2,2,2,2,2,4,3,2,1,0,
        0,1,2,2,2,2,6,6,6,6,2,2,2,2,1,0,
        0,0,1,2,2,6,7,6,6,7,6,2,2,1,0,0,
        0,0,1,2,2,2,2,2,2,2,2,2,2,1,0,0,
        0,5,5,1,2,3,2,2,2,2,3,2,1,5,5,0,
        0,0,5,1,2,2,2,2,2,2,2,2,1,5,0,0,
        0,0,0,1,2,2,3,2,2,3,2,2,1,0,0,0,
        0,0,0,1,2,2,2,2,2,2,2,2,1,0,0,0,
        0,0,0,0,1,2,2,2,2,2,2,1,0,0,0,0,
        0,0,0,0,1,2,2,0,0,2,2,1,0,0,0,0,
        0,0,0,0,1,2,0,0,0,0,2,1,0,0,0,0,
        0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildSingleFrame(pix, 16, 16, pal, 32, 32);
}

// =============================================================
// LANCER — centaur archer, 24x48
// =============================================================
Texture2D PixelArtGenerator::getLancer() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{20, 15, 10, 255},      // 1 outline
        Color{105, 70, 40, 255},     // 2 brown fur
        Color{145, 100, 60, 255},    // 3 brown highlight
        Color{210, 180, 140, 255},   // 4 skin
        Color{90, 55, 20, 255},      // 5 bow wood
        Color{230, 230, 205, 255}    // 6 bowstring
    };
    // 12x24 source
    std::vector<int> pix = {
        0,0,0,1,1,1,1,0,0,0,0,0,
        0,0,1,4,4,4,4,1,0,0,0,0,
        0,0,1,4,4,4,4,1,0,0,0,0,
        0,0,0,1,4,4,1,0,0,0,0,0,
        0,0,0,1,1,1,1,0,0,0,0,0,
        0,0,6,0,2,2,0,0,0,0,0,0,
        0,5,6,2,2,2,2,0,0,0,0,0,
        0,5,0,2,3,3,2,0,0,0,0,0,
        0,5,6,2,2,2,2,0,0,0,0,0,
        0,0,6,2,2,2,2,1,0,0,0,0,
        0,0,1,2,2,2,2,2,1,0,0,0,
        0,1,2,3,3,3,3,3,2,1,0,0,
        0,1,2,3,3,3,3,3,3,2,1,0,
        0,1,2,2,2,2,2,2,2,2,1,0,
        0,1,2,3,2,2,2,2,3,2,1,0,
        0,1,1,2,2,2,2,2,2,1,1,0,
        0,0,1,2,2,0,0,2,2,1,0,0,
        0,0,1,2,0,0,0,0,2,1,0,0,
        0,0,1,2,0,0,0,0,2,1,0,0,
        0,0,1,2,0,0,0,0,2,1,0,0,
        0,0,1,2,0,0,0,0,2,1,0,0,
        0,0,1,1,0,0,0,0,1,1,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildSingleFrame(pix, 12, 24, pal, 24, 48);
}

// =============================================================
// BRUTE — rotted tank with iron mace, 48x48
// =============================================================
Texture2D PixelArtGenerator::getBrute() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{15, 10, 10, 255},      // 1 outline
        Color{60, 80, 45, 255},      // 2 rotted green
        Color{95, 120, 65, 255},     // 3 green highlight
        Color{0, 0, 0, 255},         // 4 (unused placeholder)
        Color{250, 220, 70, 255},    // 5 yellow eye
        Color{130, 80, 40, 255},     // 6 rusty iron mace
        Color{180, 180, 80, 255}     // 7 pus
    };
    std::vector<int> pix = {
        0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,
        0,0,0,0,1,2,2,2,2,1,0,0,0,0,0,0,
        0,0,0,1,2,5,2,2,5,2,1,0,0,0,0,0,
        0,0,0,1,2,2,7,7,2,2,1,0,0,0,0,0,
        0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
        0,0,1,2,2,3,2,2,3,2,2,1,1,0,0,0,
        0,0,1,2,3,3,3,3,3,3,2,1,6,6,0,0,
        0,1,2,3,3,2,3,3,2,3,3,2,6,6,1,0,
        0,1,2,3,2,2,3,3,2,2,3,2,6,1,1,0,
        0,1,2,3,3,2,2,2,2,3,3,2,1,1,0,0,
        0,1,2,3,3,3,3,3,3,3,3,2,1,0,0,0,
        0,0,1,2,2,2,2,2,2,2,2,1,0,0,0,0,
        0,0,0,1,2,2,0,0,2,2,1,0,0,0,0,0,
        0,0,0,1,2,2,0,0,2,2,1,0,0,0,0,0,
        0,0,0,1,1,1,0,0,1,1,1,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildSingleFrame(pix, 16, 16, pal, 48, 48);
}

// =============================================================
// ASSASSIN — harpy with bone talons, 24x40
// =============================================================
Texture2D PixelArtGenerator::getAssassin() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{10, 5, 15, 255},       // 1 black
        Color{40, 35, 55, 255},      // 2 dark feather
        Color{70, 60, 85, 255},      // 3 feather highlight
        Color{220, 30, 30, 255},     // 4 red eye
        Color{220, 210, 180, 255},   // 5 bone talon
        Color{25, 20, 35, 255}       // 6 wing tip
    };
    // 12x20 source
    std::vector<int> pix = {
        0,0,0,1,1,1,1,0,0,0,0,0,
        0,0,1,2,2,2,2,1,0,0,0,0,
        0,0,1,2,4,4,2,1,0,0,0,0,
        0,0,1,2,2,2,2,1,0,0,0,0,
        0,0,0,1,1,5,1,0,0,0,0,0,
        0,0,0,1,2,2,2,1,0,0,0,0,
        0,6,6,2,2,3,3,2,2,6,6,0,
        6,6,2,2,3,3,3,3,2,2,6,6,
        6,6,2,2,2,2,2,2,2,2,6,6,
        0,6,2,3,2,2,2,2,3,2,6,0,
        0,0,2,2,2,2,2,2,2,2,0,0,
        0,0,2,2,3,3,3,3,2,2,0,0,
        0,0,0,2,2,2,2,2,2,0,0,0,
        0,0,0,2,2,0,0,2,2,0,0,0,
        0,0,0,5,5,0,0,5,5,0,0,0,
        0,0,5,1,1,0,0,1,1,5,0,0,
        0,0,5,0,0,0,0,0,0,5,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildSingleFrame(pix, 12, 20, pal, 24, 40);
}

// =============================================================
// BOMBER — round body with glowing unstable orb, 32x32
// =============================================================
Texture2D PixelArtGenerator::getBomber() {
    std::vector<Color> pal = {
        BLANK,                       // 0
        Color{15, 10, 5, 255},       // 1 outline
        Color{90, 90, 100, 255},     // 2 grey hide
        Color{180, 80, 20, 255},     // 3 orange dark
        Color{255, 200, 50, 255},    // 4 glow core
        Color{255, 30, 30, 255}      // 5 red eye
    };
    std::vector<int> pix = {
        0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,
        0,0,0,0,1,2,2,2,2,1,0,0,0,0,0,0,
        0,0,0,1,2,5,2,2,5,2,1,0,0,0,0,0,
        0,0,0,1,2,2,2,2,2,2,1,0,0,0,0,0,
        0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,
        0,0,1,2,2,3,3,3,3,2,2,1,0,0,0,0,
        0,1,2,2,3,3,4,4,3,3,2,2,1,0,0,0,
        0,1,2,3,3,4,4,4,4,3,3,2,1,0,0,0,
        0,1,2,3,3,4,4,4,4,3,3,2,1,0,0,0,
        0,1,2,2,3,3,4,4,3,3,2,2,1,0,0,0,
        0,0,1,2,2,3,3,3,3,2,2,1,0,0,0,0,
        0,0,0,1,1,2,2,2,2,1,1,0,0,0,0,0,
        0,0,0,0,1,2,2,2,2,1,0,0,0,0,0,0,
        0,0,0,0,1,2,0,0,2,1,0,0,0,0,0,0,
        0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return buildSingleFrame(pix, 16, 16, pal, 32, 32);
}

Texture2D PixelArtGenerator::getFloor() {
    std::vector<Color> pal = {
        Color{15, 15, 17, 255}, // 0: Gap/Shadows
        Color{28, 28, 30, 255}, // 1: Dark Stone Base
        Color{35, 35, 40, 255}, // 2: Mid Stone Highlight
        Color{22, 22, 25, 255}  // 3: Cracked Stone
    };
    // 16x16 Dungeon Floor Brick Pattern
    std::vector<int> pix = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,1,1,1,1,1,1,0,2,2,2,2,1,1,0,
        0,1,2,2,1,1,3,1,0,2,1,1,2,1,1,0,
        0,1,2,1,3,1,1,1,0,2,1,3,1,1,1,0,
        0,1,1,1,1,1,2,1,0,1,1,1,1,1,1,0,
        0,1,1,2,1,1,1,1,0,1,1,2,1,2,1,0,
        0,1,1,1,1,2,1,1,0,1,1,1,1,1,1,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,2,2,1,1,1,1,0,1,1,1,1,1,2,2,0,
        0,2,1,1,3,1,1,0,1,2,2,1,1,1,1,0,
        0,1,1,1,1,1,1,0,1,2,1,1,3,1,1,0,
        0,1,3,1,2,1,1,0,1,1,1,1,1,1,1,0,
        0,1,1,1,1,1,1,0,1,1,3,1,1,2,1,0,
        0,1,2,1,1,2,1,0,1,1,1,1,1,1,1,0,
        0,1,1,1,1,1,1,0,1,2,1,1,1,1,1,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return createTextureFromPalette(pix, 16, 16, pal, 64, 64);
}

Texture2D PixelArtGenerator::getDecor() {
    std::vector<Color> pal = {
        BLANK,                   // 0
        Color{20, 20, 20, 150},  // 1: Shadow
        Color{60, 60, 60, 255},  // 2: Altar Base
        Color{90, 90, 90, 255},  // 3: Altar Top
        Color{170, 20, 20, 255}, // 4: Fresh Blood
        Color{90, 10, 10, 255}   // 5: Dried Blood
    };
    // 16x16 Bloody Altar
    std::vector<int> pix = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,4,4,0,0,0,4,4,0,0,0,0,0,
        0,0,0,3,3,3,3,3,3,3,3,3,3,0,0,0,
        0,0,0,3,3,4,4,4,4,3,3,4,3,0,0,0,
        0,0,0,2,2,3,4,4,3,2,2,3,2,0,0,0,
        0,0,0,2,2,2,4,2,2,2,2,2,2,0,0,0,
        0,0,0,0,2,2,5,2,2,2,2,2,0,0,0,0,
        0,0,0,0,2,2,5,2,2,5,5,2,0,0,0,0,
        0,0,0,0,2,2,2,2,2,2,5,2,0,0,0,0,
        0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,
        0,0,0,2,2,2,5,2,2,2,2,2,2,0,0,0,
        0,0,0,2,2,2,2,2,2,2,5,2,2,0,0,0,
        0,0,2,2,2,2,2,2,2,2,2,2,2,2,0,0,
        0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
        0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0
    };
    return createTextureFromPalette(pix, 16, 16, pal, 64, 64);
}
