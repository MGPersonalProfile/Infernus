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
