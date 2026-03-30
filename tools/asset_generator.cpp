// =============================================================================
// INFERNUS Asset Generator
// Generates all sprite PNGs, SFX WAVs, and music WAVs programmatically.
// Build and run once to populate assets/ directory.
// =============================================================================
#include "raylib.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// ---- Color palette ----------------------------------------------------------
static const Color PAL_BLACK    = {10, 10, 15, 255};
static const Color PAL_DARK     = {43, 43, 43, 255};
static const Color PAL_SKIN     = {138, 123, 107, 255};
static const Color PAL_BLOOD    = {139, 0, 0, 255};
static const Color PAL_FIRE     = {204, 68, 0, 255};
static const Color PAL_EYE      = {255, 34, 0, 255};
static const Color PAL_BONE     = {212, 197, 169, 255};
static const Color PAL_WARRIOR  = {107, 16, 16, 255};
static const Color PAL_ROGUE    = {30, 74, 30, 255};
static const Color PAL_KNIGHT   = {26, 26, 94, 255};
static const Color PAL_HP       = {204, 51, 102, 255};
static const Color PAL_STAM     = {51, 204, 204, 255};
static const Color PAL_WALL     = {58, 53, 64, 255};
static const Color PAL_WALLHI   = {74, 69, 80, 255};
static const Color PAL_FLOOR    = {26, 26, 31, 255};
static const Color PAL_CRACK    = {35, 35, 40, 255};

// Soul palette (player redesign)
static const Color PAL_SOUL     = {140, 140, 170, 255};  // ghostly pale body
static const Color PAL_SOUL_HI  = {180, 180, 210, 255};  // skull highlight
static const Color PAL_SOUL_DK  = {55, 50, 70, 255};     // tattered cloth
static const Color PAL_SOUL_GLOW= {100, 120, 180, 120};  // wispy aura
static const Color PAL_WOUND    = {160, 0, 0, 220};      // bleeding wound

// ---- Helpers ----------------------------------------------------------------
static void drawPixel(Image &img, int x, int y, Color c) {
  if (x >= 0 && x < img.width && y >= 0 && y < img.height)
    ImageDrawPixel(&img, x, y, c);
}

static void drawRect(Image &img, int x, int y, int w, int h, Color c) {
  for (int dy = 0; dy < h; dy++)
    for (int dx = 0; dx < w; dx++)
      drawPixel(img, x + dx, y + dy, c);
}

static void drawCircle(Image &img, int cx, int cy, int r, Color c) {
  for (int dy = -r; dy <= r; dy++)
    for (int dx = -r; dx <= r; dx++)
      if (dx * dx + dy * dy <= r * r)
        drawPixel(img, cx + dx, cy + dy, c);
}

// Draw a single frame of a humanoid character
static void drawHumanoid(Image &img, int frameX, int frameW, int frameH,
                         Color bodyCol, Color skinCol, Color legCol,
                         int legOffset1, int legOffset2, int armDx,
                         int armDy) {
  int cx = frameX + frameW / 2;

  // Head (6x6)
  drawRect(img, cx - 3, 4, 6, 6, skinCol);
  // Eyes
  drawPixel(img, cx - 1, 6, PAL_BLACK);
  drawPixel(img, cx + 1, 6, PAL_BLACK);

  // Torso (8x10)
  drawRect(img, cx - 4, 10, 8, 10, bodyCol);
  // Shoulders
  drawRect(img, cx - 5, 10, 10, 3, bodyCol);

  // Arms
  drawRect(img, cx - 6 + armDx, 13 + armDy, 2, 8, skinCol);
  drawRect(img, cx + 4 + armDx, 13 + armDy, 2, 8, skinCol);

  // Legs
  drawRect(img, cx - 3, 20 + legOffset1, 2, 10, legCol);
  drawRect(img, cx + 1, 20 + legOffset2, 2, 10, legCol);

  // Feet
  drawRect(img, cx - 4, 29 + legOffset1, 3, 2, legCol);
  drawRect(img, cx + 1, 29 + legOffset2, 3, 2, legCol);
}

// ---- Sprite Generators ------------------------------------------------------

// Draw the player as a desperate, bleeding soul with a bone sword
static void drawSoul(Image &img, int frameX, int frameW, int /*frameH*/,
                     int legOff1, int legOff2, int armDx, int armDy,
                     int bodyBob) {
  int cx = frameX + frameW / 2;
  int by = bodyBob;

  // --- Wispy aura (ghostly glow around body) ---
  drawPixel(img, cx - 8, 5 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx + 8, 4 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx - 9, 10 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx + 9, 12 + by, PAL_SOUL_GLOW);

  // --- Skull head (12x9) ---
  drawRect(img, cx - 5, 2 + by, 10, 2, PAL_SOUL);    // cranium top
  drawRect(img, cx - 6, 4 + by, 12, 7, PAL_SOUL);    // main skull
  drawPixel(img, cx - 6, 9 + by, PAL_SOUL_DK);       // jaw shadow L
  drawPixel(img, cx + 5, 9 + by, PAL_SOUL_DK);       // jaw shadow R
  // Eye sockets (dark hollows)
  drawRect(img, cx - 4, 5 + by, 3, 3, PAL_BLACK);
  drawRect(img, cx + 1, 5 + by, 3, 3, PAL_BLACK);
  // Ember eyes (glowing)
  drawPixel(img, cx - 3, 6 + by, PAL_EYE);
  drawPixel(img, cx + 2, 6 + by, PAL_EYE);
  // Anguished mouth
  drawRect(img, cx - 3, 9 + by, 6, 2, {40, 35, 50, 255});
  drawPixel(img, cx - 1, 10 + by, PAL_BLACK);
  drawPixel(img, cx, 10 + by, PAL_BLACK);

  // --- Neck ---
  drawRect(img, cx - 2, 11 + by, 4, 3, PAL_SOUL);

  // --- Torso (tattered robes) ---
  drawRect(img, cx - 7, 14 + by, 14, 16, PAL_SOUL_DK);
  drawRect(img, cx - 6, 14 + by, 12, 2, PAL_SOUL);     // collar
  // Ragged torn edges
  drawPixel(img, cx - 8, 17 + by, PAL_SOUL_DK);
  drawPixel(img, cx - 8, 21 + by, PAL_SOUL_DK);
  drawPixel(img, cx - 8, 25 + by, PAL_SOUL_DK);
  drawPixel(img, cx + 7, 19 + by, PAL_SOUL_DK);
  drawPixel(img, cx + 7, 23 + by, PAL_SOUL_DK);
  drawRect(img, cx - 9, 26 + by, 2, 4, PAL_SOUL_DK);   // hanging cloth L
  drawRect(img, cx + 7, 27 + by, 2, 3, PAL_SOUL_DK);   // hanging cloth R

  // --- Chest wound (bleeding gash) ---
  drawRect(img, cx - 2, 18 + by, 4, 3, PAL_WOUND);
  drawPixel(img, cx - 1, 21 + by, PAL_BLOOD);
  drawPixel(img, cx, 23 + by, PAL_BLOOD);
  drawPixel(img, cx - 1, 25 + by, PAL_BLOOD);
  drawPixel(img, cx + 4, 22 + by, PAL_BLOOD);
  drawPixel(img, cx + 4, 24 + by, PAL_BLOOD);
  drawPixel(img, cx - 4, 20 + by, PAL_BLOOD);

  // --- Left arm (thin, ghostly) ---
  drawRect(img, cx - 9 + armDx, 16 + armDy + by, 2, 12, PAL_SOUL);
  drawPixel(img, cx - 9 + armDx, 28 + armDy + by, PAL_BLOOD); // blood hand

  // --- Right arm (holds bone sword) ---
  drawRect(img, cx + 7 + armDx, 16 + armDy + by, 2, 12, PAL_SOUL);

  // --- Bone sword (dragged from right hand) ---
  int sx = cx + 9 + armDx;
  int sy = 24 + armDy + by;
  drawRect(img, sx - 1, sy - 1, 4, 2, PAL_BONE);    // crossguard
  drawRect(img, sx, sy + 1, 2, 16, PAL_BONE);        // blade down
  drawPixel(img, sx, sy + 16, WHITE);                 // blade tip glow
  drawPixel(img, sx + 1, sy + 15, PAL_SOUL_HI);      // blade shine
  drawRect(img, sx, sy - 3, 2, 3, PAL_SOUL_DK);      // handle

  // --- Legs (tattered, fading) ---
  drawRect(img, cx - 5, 30 + legOff1 + by, 3, 14, PAL_SOUL_DK);
  drawRect(img, cx + 2, 30 + legOff2 + by, 3, 14, PAL_SOUL_DK);
  // Wispy fade at feet
  drawPixel(img, cx - 4, 44 + legOff1 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx - 6, 46 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx + 3, 44 + legOff2 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx + 5, 47 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx, 48 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx - 2, 50 + by, PAL_SOUL_GLOW);
  drawPixel(img, cx + 1, 52 + by, PAL_SOUL_GLOW);
}

static void generatePlayerSprites() {
  // IDLE: 2 frames, 48x56 each → 96x56
  {
    Image img = GenImageColor(96, 56, BLANK);
    drawSoul(img, 0, 48, 56, 0, 0, 0, 0, 0);
    drawSoul(img, 48, 48, 56, 0, 0, 0, -1, -1);  // breathing bob
    ExportImage(img, "assets/sprites/player/player_idle.png");
    UnloadImage(img);
  }

  // RUN: 4 frames, 48x56 each → 192x56 (dramatic stride + body bob)
  {
    Image img = GenImageColor(192, 56, BLANK);
    drawSoul(img, 0, 48, 56, -4, 3, -2, 0, -2);     // left stride, bob up
    drawSoul(img, 48, 48, 56, -1, 1, 0, 0, 0);       // passing center
    drawSoul(img, 96, 48, 56, 3, -4, 2, 0, -2);      // right stride, bob up
    drawSoul(img, 144, 48, 56, 1, -1, 0, 0, 0);      // passing center
    ExportImage(img, "assets/sprites/player/player_run.png");
    UnloadImage(img);
  }

  // ATTACK: 3 frames → 144x56
  {
    Image img = GenImageColor(144, 56, BLANK);
    // Windup: sword pulled back
    drawSoul(img, 0, 48, 56, 0, 0, -5, -3, 0);
    // Active: sword swung forward
    drawSoul(img, 48, 48, 56, 0, 0, 6, 1, 0);
    // Draw slash arc over the attack frame
    int acx = 48 + 24;
    for (int i = 0; i < 12; i++) {
      int ax = acx + 10 + i;
      int ay = 12 + (i < 6 ? -i : i - 12);
      drawPixel(img, ax, ay, WHITE);
      drawPixel(img, ax, ay + 1, {220, 220, 255, 200});
    }
    // Recovery
    drawSoul(img, 96, 48, 56, 0, 0, 1, 1, 0);
    ExportImage(img, "assets/sprites/player/player_attack.png");
    UnloadImage(img);
  }

  // DEATH: 3 frames → 144x56
  {
    Image img = GenImageColor(144, 56, BLANK);
    // Frame 1: stagger (leaning, sword falling)
    drawSoul(img, 0, 48, 56, 1, 1, 3, 2, 2);

    // Frame 2: collapsing (custom drawn)
    int cx = 48 + 24;
    int by = 10;
    drawRect(img, cx - 5, by, 10, 7, PAL_SOUL);        // skull tilted
    drawRect(img, cx - 3, by + 2, 2, 2, PAL_BLACK);     // eye socket
    drawPixel(img, cx - 2, by + 3, PAL_EYE);            // fading eye
    drawRect(img, cx - 6, by + 8, 12, 12, PAL_SOUL_DK); // body crumpling
    drawRect(img, cx - 1, by + 12, 3, 2, PAL_WOUND);    // wound
    drawRect(img, cx - 4, by + 20, 3, 8, PAL_SOUL_DK);  // leg
    drawRect(img, cx + 2, by + 21, 3, 6, PAL_SOUL_DK);  // leg
    drawRect(img, cx + 8, by + 14, 2, 10, PAL_BONE);    // sword falling

    // Frame 3: dissolved into particles
    cx = 96 + 24;
    for (int i = 0; i < 30; i++) {
      int px = cx - 12 + (i * 7 % 24);
      int py = 20 + (i * 11 % 25);
      Color c = (i % 4 == 0) ? PAL_EYE : (i % 3 == 0) ? PAL_SOUL_GLOW : PAL_SOUL_DK;
      drawPixel(img, px, py, c);
    }
    // Last ember of the eyes
    drawPixel(img, cx - 2, 30, PAL_EYE);
    drawPixel(img, cx + 1, 31, PAL_EYE);
    // Bone sword on ground
    drawRect(img, cx - 8, 42, 14, 2, PAL_BONE);

    ExportImage(img, "assets/sprites/player/player_death.png");
    UnloadImage(img);
  }

  printf("[OK] Player sprites (soul) generated\n");
}

// =============================================================================
// Class-specific character sprites for select screen and gameplay
// Each class is a distinct soul with unique silhouette and weapon.
// =============================================================================

// --- GUERRERO (Warrior): bulky soul, heavy bone greatsword, red tint ---
static void drawWarriorSoul(Image &img, int frameX, int frameW,
                            int legOff1, int legOff2, int armDx, int armDy,
                            int bodyBob) {
  int cx = frameX + frameW / 2;
  int by = bodyBob;
  Color body = {160, 100, 100, 255};
  Color bodyDk = {80, 30, 30, 255};
  Color bodyHi = {200, 140, 140, 255};

  // Wispy aura (red)
  drawPixel(img, cx - 9, 6 + by, {180, 50, 50, 80});
  drawPixel(img, cx + 9, 5 + by, {180, 50, 50, 80});

  // Skull (wider, heavier jaw)
  drawRect(img, cx - 6, 2 + by, 12, 2, body);
  drawRect(img, cx - 7, 4 + by, 14, 8, body);
  drawRect(img, cx - 4, 5 + by, 3, 3, PAL_BLACK);
  drawRect(img, cx + 1, 5 + by, 3, 3, PAL_BLACK);
  drawPixel(img, cx - 3, 6 + by, PAL_EYE);
  drawPixel(img, cx + 2, 6 + by, PAL_EYE);
  drawRect(img, cx - 3, 10 + by, 6, 2, {40, 20, 20, 255});

  // Neck
  drawRect(img, cx - 3, 12 + by, 6, 3, body);

  // Torso (wide, armored look)
  drawRect(img, cx - 8, 15 + by, 16, 16, bodyDk);
  drawRect(img, cx - 7, 15 + by, 14, 3, body); // pauldrons
  drawRect(img, cx - 3, 19 + by, 6, 4, bodyHi); // chest plate
  // Wound
  drawRect(img, cx - 1, 24 + by, 3, 2, PAL_WOUND);
  drawPixel(img, cx, 26 + by, PAL_BLOOD);

  // Left arm
  drawRect(img, cx - 10 + armDx, 17 + armDy + by, 3, 12, body);

  // Right arm (holds greatsword)
  drawRect(img, cx + 7 + armDx, 17 + armDy + by, 3, 12, body);

  // Greatsword (large bone blade)
  int sx = cx + 10 + armDx;
  int sy = 22 + armDy + by;
  drawRect(img, sx - 2, sy - 2, 6, 3, PAL_BONE); // wide crossguard
  drawRect(img, sx - 1, sy + 1, 4, 20, PAL_BONE); // wide blade
  drawPixel(img, sx, sy + 20, WHITE);
  drawPixel(img, sx + 1, sy + 19, PAL_SOUL_HI);
  drawRect(img, sx, sy - 5, 2, 4, bodyDk); // handle

  // Legs
  drawRect(img, cx - 5, 31 + legOff1 + by, 4, 14, bodyDk);
  drawRect(img, cx + 2, 31 + legOff2 + by, 4, 14, bodyDk);
  drawPixel(img, cx - 4, 45 + legOff1 + by, {180, 50, 50, 60});
  drawPixel(img, cx + 3, 45 + legOff2 + by, {180, 50, 50, 60});
  drawPixel(img, cx, 48 + by, {180, 50, 50, 60});
}

// --- PICARO (Rogue): thin hooded soul, dual bone daggers, green tint ---
static void drawRogueSoul(Image &img, int frameX, int frameW,
                          int legOff1, int legOff2, int armDx, int armDy,
                          int bodyBob) {
  int cx = frameX + frameW / 2;
  int by = bodyBob;
  Color body = {100, 160, 100, 255};
  Color bodyDk = {30, 70, 30, 255};
  Color bodyHi = {140, 200, 140, 255};

  // Wispy aura (green)
  drawPixel(img, cx - 8, 3 + by, {50, 180, 50, 80});
  drawPixel(img, cx + 8, 4 + by, {50, 180, 50, 80});

  // Hood (pointed)
  drawRect(img, cx - 1, 0 + by, 2, 3, bodyDk);
  drawRect(img, cx - 4, 2 + by, 8, 3, bodyDk);

  // Skull (narrower, shadowed by hood)
  drawRect(img, cx - 5, 4 + by, 10, 7, body);
  drawRect(img, cx - 3, 5 + by, 2, 2, PAL_BLACK);
  drawRect(img, cx + 1, 5 + by, 2, 2, PAL_BLACK);
  drawPixel(img, cx - 2, 5 + by, {50, 255, 50, 255}); // green eyes
  drawPixel(img, cx + 1, 5 + by, {50, 255, 50, 255});
  drawRect(img, cx - 2, 9 + by, 4, 1, {30, 50, 30, 255});

  // Neck
  drawRect(img, cx - 2, 11 + by, 4, 2, body);

  // Torso (thin, wrapped)
  drawRect(img, cx - 5, 13 + by, 10, 15, bodyDk);
  drawRect(img, cx - 4, 13 + by, 8, 2, body); // collar
  // Diagonal wraps
  drawPixel(img, cx - 3, 17 + by, bodyHi);
  drawPixel(img, cx - 1, 19 + by, bodyHi);
  drawPixel(img, cx + 1, 21 + by, bodyHi);
  drawPixel(img, cx + 3, 23 + by, bodyHi);

  // Arms (thin)
  drawRect(img, cx - 7 + armDx, 15 + armDy + by, 2, 10, body);
  drawRect(img, cx + 5 + armDx, 15 + armDy + by, 2, 10, body);

  // Dual bone daggers
  int dl = cx - 8 + armDx;
  int dr = cx + 6 + armDx;
  int dy = 24 + armDy + by;
  drawRect(img, dl, dy, 1, 8, PAL_BONE);
  drawPixel(img, dl, dy + 8, WHITE);
  drawRect(img, dr + 1, dy, 1, 8, PAL_BONE);
  drawPixel(img, dr + 1, dy + 8, WHITE);

  // Legs (thin, agile)
  drawRect(img, cx - 4, 28 + legOff1 + by, 2, 14, bodyDk);
  drawRect(img, cx + 2, 28 + legOff2 + by, 2, 14, bodyDk);
  drawPixel(img, cx - 3, 42 + legOff1 + by, {50, 180, 50, 60});
  drawPixel(img, cx + 3, 42 + legOff2 + by, {50, 180, 50, 60});
  drawPixel(img, cx, 46 + by, {50, 180, 50, 60});
}

// --- CABALLERO (Knight): wide tanky soul, bone shield + mace, blue tint ---
static void drawKnightSoul(Image &img, int frameX, int frameW,
                           int legOff1, int legOff2, int armDx, int armDy,
                           int bodyBob) {
  int cx = frameX + frameW / 2;
  int by = bodyBob;
  Color body = {100, 110, 180, 255};
  Color bodyDk = {30, 35, 90, 255};
  Color bodyHi = {140, 150, 220, 255};

  // Wispy aura (blue)
  drawPixel(img, cx - 9, 5 + by, {50, 70, 200, 80});
  drawPixel(img, cx + 10, 6 + by, {50, 70, 200, 80});

  // Skull (helm-like, with visor)
  drawRect(img, cx - 6, 1 + by, 12, 2, bodyDk); // helm crest
  drawRect(img, cx - 7, 3 + by, 14, 9, body);
  drawRect(img, cx - 5, 5 + by, 10, 1, bodyDk); // visor slit
  drawPixel(img, cx - 3, 5 + by, {80, 120, 255, 255}); // blue eyes
  drawPixel(img, cx + 2, 5 + by, {80, 120, 255, 255});
  drawRect(img, cx - 4, 9 + by, 8, 2, bodyDk); // jaw guard

  // Neck
  drawRect(img, cx - 3, 12 + by, 6, 2, body);

  // Torso (very wide, heavy plate)
  drawRect(img, cx - 9, 14 + by, 18, 17, bodyDk);
  drawRect(img, cx - 8, 14 + by, 16, 3, body); // pauldrons
  drawRect(img, cx - 4, 18 + by, 8, 6, bodyHi); // chest plate
  drawRect(img, cx - 2, 20 + by, 4, 2, body); // emblem

  // Left arm (holds shield)
  drawRect(img, cx - 11 + armDx, 16 + armDy + by, 3, 12, body);
  // Bone shield (oval-ish)
  int shx = cx - 15 + armDx;
  int shy = 18 + armDy + by;
  drawRect(img, shx, shy, 6, 12, PAL_BONE);
  drawRect(img, shx + 1, shy - 1, 4, 14, PAL_BONE);
  drawRect(img, shx + 2, shy + 3, 2, 6, bodyDk); // shield boss
  drawPixel(img, shx + 2, shy + 5, bodyHi);

  // Right arm (holds mace)
  drawRect(img, cx + 8 + armDx, 16 + armDy + by, 3, 12, body);
  // Bone mace
  int mx = cx + 11 + armDx;
  int my = 22 + armDy + by;
  drawRect(img, mx, my, 2, 12, PAL_BONE); // handle
  drawRect(img, mx - 2, my + 11, 6, 5, PAL_BONE); // head
  drawPixel(img, mx - 2, my + 12, bodyHi);
  drawPixel(img, mx + 3, my + 14, bodyHi);

  // Legs (wide, armored)
  drawRect(img, cx - 6, 31 + legOff1 + by, 4, 14, bodyDk);
  drawRect(img, cx + 2, 31 + legOff2 + by, 4, 14, bodyDk);
  drawRect(img, cx - 6, 40 + legOff1 + by, 4, 4, body); // greaves
  drawRect(img, cx + 2, 40 + legOff2 + by, 4, 4, body);
  drawPixel(img, cx, 48 + by, {50, 70, 200, 60});
}

// Generate class-specific sprite sheets
static void generateClassSprites() {
  // Warrior idle (2 frames, 48x56 → 96x56)
  {
    Image img = GenImageColor(96, 56, BLANK);
    drawWarriorSoul(img, 0, 48, 0, 0, 0, 0, 0);
    drawWarriorSoul(img, 48, 48, 0, 0, 0, -1, -1);
    ExportImage(img, "assets/sprites/player/warrior_idle.png");
    UnloadImage(img);
  }
  // Warrior run (4 frames → 192x56)
  {
    Image img = GenImageColor(192, 56, BLANK);
    drawWarriorSoul(img, 0, 48, -4, 3, -2, 0, -2);
    drawWarriorSoul(img, 48, 48, -1, 1, 0, 0, 0);
    drawWarriorSoul(img, 96, 48, 3, -4, 2, 0, -2);
    drawWarriorSoul(img, 144, 48, 1, -1, 0, 0, 0);
    ExportImage(img, "assets/sprites/player/warrior_run.png");
    UnloadImage(img);
  }
  // Warrior attack (3 frames → 144x56)
  {
    Image img = GenImageColor(144, 56, BLANK);
    drawWarriorSoul(img, 0, 48, 0, 0, -5, -3, 0);
    drawWarriorSoul(img, 48, 48, 0, 0, 6, 1, 0);
    drawWarriorSoul(img, 96, 48, 0, 0, 1, 1, 0);
    ExportImage(img, "assets/sprites/player/warrior_attack.png");
    UnloadImage(img);
  }

  // Rogue idle (2 frames → 96x56)
  {
    Image img = GenImageColor(96, 56, BLANK);
    drawRogueSoul(img, 0, 48, 0, 0, 0, 0, 0);
    drawRogueSoul(img, 48, 48, 0, 0, 0, -1, -1);
    ExportImage(img, "assets/sprites/player/rogue_idle.png");
    UnloadImage(img);
  }
  // Rogue run (4 frames → 192x56)
  {
    Image img = GenImageColor(192, 56, BLANK);
    drawRogueSoul(img, 0, 48, -5, 4, -2, 0, -2);
    drawRogueSoul(img, 48, 48, -1, 1, 0, 0, 0);
    drawRogueSoul(img, 96, 48, 4, -5, 2, 0, -2);
    drawRogueSoul(img, 144, 48, 1, -1, 0, 0, 0);
    ExportImage(img, "assets/sprites/player/rogue_run.png");
    UnloadImage(img);
  }
  // Rogue attack (3 frames → 144x56)
  {
    Image img = GenImageColor(144, 56, BLANK);
    drawRogueSoul(img, 0, 48, 0, 0, -4, -2, 0);
    drawRogueSoul(img, 48, 48, 0, 0, 5, 2, 0);
    drawRogueSoul(img, 96, 48, 0, 0, 1, 0, 0);
    ExportImage(img, "assets/sprites/player/rogue_attack.png");
    UnloadImage(img);
  }

  // Knight idle (2 frames → 96x56)
  {
    Image img = GenImageColor(96, 56, BLANK);
    drawKnightSoul(img, 0, 48, 0, 0, 0, 0, 0);
    drawKnightSoul(img, 48, 48, 0, 0, 0, -1, -1);
    ExportImage(img, "assets/sprites/player/knight_idle.png");
    UnloadImage(img);
  }
  // Knight run (4 frames → 192x56)
  {
    Image img = GenImageColor(192, 56, BLANK);
    drawKnightSoul(img, 0, 48, -3, 2, -1, 0, -1);
    drawKnightSoul(img, 48, 48, -1, 1, 0, 0, 0);
    drawKnightSoul(img, 96, 48, 2, -3, 1, 0, -1);
    drawKnightSoul(img, 144, 48, 1, -1, 0, 0, 0);
    ExportImage(img, "assets/sprites/player/knight_run.png");
    UnloadImage(img);
  }
  // Knight attack (3 frames → 144x56)
  {
    Image img = GenImageColor(144, 56, BLANK);
    drawKnightSoul(img, 0, 48, 0, 0, -4, -3, 0);
    drawKnightSoul(img, 48, 48, 0, 0, 5, 2, 0);
    drawKnightSoul(img, 96, 48, 0, 0, 1, 1, 0);
    ExportImage(img, "assets/sprites/player/knight_attack.png");
    UnloadImage(img);
  }

  printf("[OK] Class sprites (warrior/rogue/knight) generated\n");
}

static void drawDemon(Image &img, int fx, int fw, int fh, int legOff1,
                      int legOff2, int armOff) {
  int cx = fx + fw / 2;
  // Horns
  drawRect(img, cx - 6, 2, 2, 4, PAL_BLOOD);
  drawRect(img, cx + 4, 2, 2, 4, PAL_BLOOD);
  // Head
  drawRect(img, cx - 5, 5, 10, 7, PAL_DARK);
  // Eyes
  drawPixel(img, cx - 2, 8, PAL_EYE);
  drawPixel(img, cx + 2, 8, PAL_EYE);
  // Body (wide)
  drawRect(img, cx - 6, 12, 12, 8, PAL_DARK);
  // Arms
  drawRect(img, cx - 8 + armOff, 13, 2, 6, PAL_DARK);
  drawRect(img, cx + 6 + armOff, 13, 2, 6, PAL_DARK);
  // Legs
  drawRect(img, cx - 4, 20 + legOff1, 3, 5, PAL_DARK);
  drawRect(img, cx + 1, 20 + legOff2, 3, 5, PAL_DARK);
}

static void generateEnemySprites() {
  // --- DEMON (melee, 32x32) ---
  {
    Image img = GenImageColor(64, 32, BLANK);
    drawDemon(img, 0, 32, 32, 0, 0, 0);
    drawDemon(img, 32, 32, 32, 0, 0, 0); // frame 2: breathe
    // shift body up 1px in frame 2
    drawPixel(img, 32 + 16, 4, PAL_BLOOD);
    ExportImage(img, "assets/sprites/enemies/demon_idle.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(96, 32, BLANK);
    drawDemon(img, 0, 32, 32, -3, 3, -1);
    drawDemon(img, 32, 32, 32, 0, 0, 0);
    drawDemon(img, 64, 32, 32, 3, -3, 1);
    ExportImage(img, "assets/sprites/enemies/demon_move.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(64, 32, BLANK);
    drawDemon(img, 0, 32, 32, 0, 0, -2);  // windup
    drawDemon(img, 32, 32, 32, 0, 0, 4);   // lunge
    // add claw marks
    drawRect(img, 32 + 24, 14, 4, 2, PAL_EYE);
    ExportImage(img, "assets/sprites/enemies/demon_attack.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(64, 32, BLANK);
    drawDemon(img, 0, 32, 32, 0, 0, 0);
    // frame 2: scattered pixels (disintegrate)
    for (int i = 0; i < 20; i++) {
      int px = 32 + 8 + (i * 7 % 16);
      int py = 4 + (i * 11 % 24);
      drawPixel(img, px, py, PAL_DARK);
    }
    ExportImage(img, "assets/sprites/enemies/demon_death.png");
    UnloadImage(img);
  }

  // --- LANCER (ranged, 24x48) ---
  auto drawLancer = [](Image &img, int fx, int armOff, int robeOff) {
    int cx = fx + 12;
    // Hood
    drawRect(img, cx - 3, 2, 6, 3, PAL_DARK);
    drawRect(img, cx - 1, 0, 2, 3, PAL_DARK); // point
    // Face
    drawRect(img, cx - 2, 5, 4, 4, PAL_DARK);
    drawPixel(img, cx, 7, PAL_FIRE); // one eye glowing
    // Torso
    drawRect(img, cx - 3, 9, 6, 8, PAL_DARK);
    // Robe (long)
    drawRect(img, cx - 4, 17, 8, 12 + robeOff, {50, 45, 55, 255});
    // Staff/arm
    drawRect(img, cx + 3 + armOff, 10, 2, 14, PAL_FIRE);
    // Feet
    drawRect(img, cx - 2, 29 + robeOff, 2, 2, PAL_DARK);
    drawRect(img, cx + 1, 29 + robeOff, 2, 2, PAL_DARK);
  };

  {
    Image img = GenImageColor(48, 48, BLANK);
    drawLancer(img, 0, 0, 0);
    drawLancer(img, 24, 0, 1);
    ExportImage(img, "assets/sprites/enemies/lancer_idle.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(72, 48, BLANK);
    drawLancer(img, 0, -1, -2);
    drawLancer(img, 24, 0, 0);
    drawLancer(img, 48, 1, 2);
    ExportImage(img, "assets/sprites/enemies/lancer_move.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(48, 48, BLANK);
    drawLancer(img, 0, -1, 0);
    drawLancer(img, 24, 4, 0);
    // Fireball at tip in frame 2
    drawCircle(img, 24 + 12 + 8, 16, 3, PAL_FIRE);
    drawCircle(img, 24 + 12 + 8, 16, 1, {255, 170, 0, 255});
    ExportImage(img, "assets/sprites/enemies/lancer_attack.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(48, 48, BLANK);
    drawLancer(img, 0, 0, 0);
    for (int i = 0; i < 15; i++) {
      drawPixel(img, 24 + 4 + (i * 7 % 16), 4 + (i * 13 % 30), PAL_DARK);
    }
    ExportImage(img, "assets/sprites/enemies/lancer_death.png");
    UnloadImage(img);
  }

  // --- BRUTE (tank, 48x48) ---
  auto drawBrute = [](Image &img, int fx, int legOff1, int legOff2,
                      int armOff) {
    int cx = fx + 24;
    // Head (wide)
    drawRect(img, cx - 6, 4, 12, 8, PAL_DARK);
    drawPixel(img, cx - 3, 7, PAL_EYE);
    drawPixel(img, cx + 3, 7, PAL_EYE);
    // Massive body
    drawRect(img, cx - 10, 12, 20, 14, PAL_DARK);
    drawRect(img, cx - 8, 12, 16, 2, {55, 50, 50, 255}); // shoulder highlight
    // Arms
    drawRect(img, cx - 13 + armOff, 14, 3, 10, PAL_DARK);
    drawRect(img, cx + 10 + armOff, 14, 3, 10, PAL_DARK);
    // Legs
    drawRect(img, cx - 7, 26 + legOff1, 5, 10, PAL_DARK);
    drawRect(img, cx + 2, 26 + legOff2, 5, 10, PAL_DARK);
    // Feet
    drawRect(img, cx - 8, 35 + legOff1, 6, 3, PAL_DARK);
    drawRect(img, cx + 2, 35 + legOff2, 6, 3, PAL_DARK);
  };

  {
    Image img = GenImageColor(96, 48, BLANK);
    drawBrute(img, 0, 0, 0, 0);
    drawBrute(img, 48, 0, 0, 0);
    ExportImage(img, "assets/sprites/enemies/brute_idle.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(144, 48, BLANK);
    drawBrute(img, 0, -3, 3, -1);
    drawBrute(img, 48, 0, 0, 0);
    drawBrute(img, 96, 3, -3, 1);
    ExportImage(img, "assets/sprites/enemies/brute_move.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(144, 48, BLANK);
    drawBrute(img, 0, 0, 0, -2);   // windup
    drawBrute(img, 48, 0, 0, 5);    // smash forward
    drawRect(img, 48 + 24 + 14, 24, 6, 3, PAL_EYE); // impact
    drawBrute(img, 96, 0, 0, 1);    // recovery
    ExportImage(img, "assets/sprites/enemies/brute_attack.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(144, 48, BLANK);
    drawBrute(img, 0, 0, 0, 0);
    drawBrute(img, 48, 2, 2, 0); // sinking
    // Frame 3: rubble
    for (int i = 0; i < 30; i++) {
      drawPixel(img, 96 + 6 + (i * 7 % 36), 30 + (i * 3 % 12), PAL_DARK);
    }
    ExportImage(img, "assets/sprites/enemies/brute_death.png");
    UnloadImage(img);
  }

  // --- ASSASSIN (fast melee, 24x40) ---
  auto drawAssassin = [](Image &img, int fx, int legOff, int armOff) {
    int cx = fx + 12;
    // Hooded head
    drawRect(img, cx - 4, 2, 8, 3, {50, 20, 80, 255}); // hood
    drawRect(img, cx - 3, 5, 6, 5, {80, 40, 120, 255}); // face
    drawPixel(img, cx - 1, 7, {200, 0, 200, 255}); // eye
    drawPixel(img, cx + 1, 7, {200, 0, 200, 255});
    // Thin body
    drawRect(img, cx - 3, 10, 6, 10, {50, 20, 80, 255});
    // Arms
    drawRect(img, cx - 5 + armOff, 12, 2, 8, {80, 40, 120, 255});
    drawRect(img, cx + 3 + armOff, 12, 2, 8, {80, 40, 120, 255});
    // Daggers
    drawRect(img, cx - 6 + armOff, 19, 1, 6, PAL_BONE);
    drawRect(img, cx + 5 + armOff, 19, 1, 6, PAL_BONE);
    // Legs
    drawRect(img, cx - 3, 20 + legOff, 2, 10, {40, 15, 60, 255});
    drawRect(img, cx + 1, 20 - legOff, 2, 10, {40, 15, 60, 255});
    // Wispy trail
    drawPixel(img, cx - 4, 30 + legOff, {100, 50, 150, 80});
    drawPixel(img, cx + 4, 32, {100, 50, 150, 80});
  };

  {
    Image img = GenImageColor(48, 40, BLANK);
    drawAssassin(img, 0, 0, 0);
    drawAssassin(img, 24, 0, 0);
    ExportImage(img, "assets/sprites/enemies/assassin_idle.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(72, 40, BLANK);
    drawAssassin(img, 0, -3, -1);
    drawAssassin(img, 24, 0, 0);
    drawAssassin(img, 48, 3, 1);
    ExportImage(img, "assets/sprites/enemies/assassin_move.png");
    UnloadImage(img);
  }

  // --- BOMBER (ranged exploder, 32x32) ---
  auto drawBomber = [](Image &img, int fx, int bobOff) {
    int cx = fx + 16;
    // Spherical body (soul orb)
    drawCircle(img, cx, 14 + bobOff, 10, {200, 100, 0, 255});
    drawCircle(img, cx, 14 + bobOff, 7, {230, 150, 30, 255});
    // Eyes (angry)
    drawPixel(img, cx - 3, 12 + bobOff, PAL_BLACK);
    drawPixel(img, cx + 3, 12 + bobOff, PAL_BLACK);
    drawPixel(img, cx - 2, 13 + bobOff, PAL_EYE);
    drawPixel(img, cx + 2, 13 + bobOff, PAL_EYE);
    // Mouth (gaping)
    drawRect(img, cx - 2, 17 + bobOff, 4, 2, PAL_BLACK);
    // Inner glow (unstable energy)
    drawPixel(img, cx, 14 + bobOff, {255, 255, 100, 200});
    drawPixel(img, cx - 1, 15 + bobOff, {255, 200, 50, 150});
    // Floating sparks
    drawPixel(img, cx - 8, 5, PAL_FIRE);
    drawPixel(img, cx + 7, 8, PAL_FIRE);
    drawPixel(img, cx - 5, 24, {255, 150, 0, 120});
    drawPixel(img, cx + 6, 22, {255, 150, 0, 120});
  };

  {
    Image img = GenImageColor(64, 32, BLANK);
    drawBomber(img, 0, 0);
    drawBomber(img, 32, -1);
    ExportImage(img, "assets/sprites/enemies/bomber_idle.png");
    UnloadImage(img);
  }
  {
    Image img = GenImageColor(96, 32, BLANK);
    drawBomber(img, 0, -2);
    drawBomber(img, 32, 0);
    drawBomber(img, 64, 2);
    ExportImage(img, "assets/sprites/enemies/bomber_move.png");
    UnloadImage(img);
  }

  printf("[OK] Enemy sprites generated\n");
}

static void generateBossSprites() {
  auto drawMinotaur = [](Image &img, int fx, int fw, int legOff, int armOff,
                         int headTilt) {
    int cx = fx + fw / 2;
    // Horns (large, bone colored)
    drawRect(img, cx - 18, 4 + headTilt, 4, 12, PAL_BONE);
    drawRect(img, cx - 16, 2 + headTilt, 4, 4, PAL_BONE);
    drawRect(img, cx + 14, 4 + headTilt, 4, 12, PAL_BONE);
    drawRect(img, cx + 14, 2 + headTilt, 4, 4, PAL_BONE);
    // Head
    Color bossBody = {107, 32, 32, 255};
    drawRect(img, cx - 12, 10 + headTilt, 24, 14, bossBody);
    // Snout
    drawRect(img, cx - 6, 18 + headTilt, 12, 6, {90, 25, 25, 255});
    // Eyes
    drawRect(img, cx - 8, 14 + headTilt, 3, 3, PAL_EYE);
    drawRect(img, cx + 5, 14 + headTilt, 3, 3, PAL_EYE);
    // Neck
    drawRect(img, cx - 8, 24, 16, 6, bossBody);
    // Torso (massive)
    drawRect(img, cx - 16, 30, 32, 20, bossBody);
    drawRect(img, cx - 14, 30, 28, 3, {120, 40, 40, 255}); // chest highlight
    // Arms
    drawRect(img, cx - 22 + armOff, 32, 6, 16, bossBody);
    drawRect(img, cx + 16 + armOff, 32, 6, 16, bossBody);
    // Fists
    drawRect(img, cx - 23 + armOff, 48, 8, 6, {90, 25, 25, 255});
    drawRect(img, cx + 15 + armOff, 48, 8, 6, {90, 25, 25, 255});
    // Legs
    drawRect(img, cx - 12, 50 + legOff, 8, 14, bossBody);
    drawRect(img, cx + 4, 50 + legOff, 8, 14, bossBody);
    // Hooves
    drawRect(img, cx - 14, 63 + legOff, 10, 5, PAL_DARK);
    drawRect(img, cx + 4, 63 + legOff, 10, 5, PAL_DARK);
  };

  // IDLE: 2 frames 80x80
  {
    Image img = GenImageColor(160, 80, BLANK);
    drawMinotaur(img, 0, 80, 0, 0, 0);
    drawMinotaur(img, 80, 80, 0, 0, -1);
    ExportImage(img, "assets/sprites/bosses/minotaur_idle.png");
    UnloadImage(img);
  }
  // CHARGE: 3 frames
  {
    Image img = GenImageColor(240, 80, BLANK);
    drawMinotaur(img, 0, 80, 0, -3, 4);    // lean forward
    drawMinotaur(img, 80, 80, -2, 0, 6);    // charging (head down)
    drawMinotaur(img, 160, 80, 2, 2, 0);    // stopping
    ExportImage(img, "assets/sprites/bosses/minotaur_charge.png");
    UnloadImage(img);
  }
  // SLAM: 3 frames
  {
    Image img = GenImageColor(240, 80, BLANK);
    drawMinotaur(img, 0, 80, 0, -4, -3);    // arms up
    drawMinotaur(img, 80, 80, 0, 6, 2);     // arms down (slam)
    // Shockwave ring in frame 2
    for (int x = 80 + 10; x < 80 + 70; x += 2)
      drawPixel(img, x, 72, PAL_FIRE);
    for (int x = 80 + 5; x < 80 + 75; x += 2)
      drawPixel(img, x, 74, {255, 170, 0, 255});
    drawMinotaur(img, 160, 80, 0, 1, 0);    // recovery
    ExportImage(img, "assets/sprites/bosses/minotaur_slam.png");
    UnloadImage(img);
  }
  // DEATH: 4 frames
  {
    Image img = GenImageColor(320, 80, BLANK);
    drawMinotaur(img, 0, 80, 0, 0, 0);       // stagger
    drawMinotaur(img, 80, 80, 4, 2, 2);       // falling
    // Frame 3: on knees
    int cx = 160 + 40;
    Color bc = {107, 32, 32, 255};
    drawRect(img, cx - 12, 30, 24, 14, bc);   // head lowered
    drawRect(img, cx - 16, 44, 32, 16, bc);   // body
    drawRect(img, cx - 12, 60, 8, 10, bc);
    drawRect(img, cx + 4, 60, 8, 10, bc);
    // Frame 4: scattered remains
    for (int i = 0; i < 40; i++) {
      int px = 240 + 10 + (i * 7 % 60);
      int py = 40 + (i * 11 % 35);
      Color c = (i % 3 == 0) ? PAL_FIRE : bc;
      drawRect(img, px, py, 3, 3, c);
    }
    ExportImage(img, "assets/sprites/bosses/minotaur_death.png");
    UnloadImage(img);
  }

  printf("[OK] Boss sprites generated\n");
}

static void generateTileSprites() {
  // FLOOR 64x64
  {
    Image img = GenImageColor(64, 64, PAL_FLOOR);
    // Cracks
    for (int i = 0; i < 8; i++) {
      int x1 = (i * 17 + 3) % 60;
      int y1 = (i * 23 + 7) % 60;
      drawRect(img, x1, y1, (i % 3) + 1, 1, PAL_CRACK);
      drawRect(img, x1, y1 + 1, 1, (i % 2) + 1, PAL_CRACK);
    }
    // Blood splatters
    drawPixel(img, 15, 20, PAL_BLOOD);
    drawPixel(img, 16, 21, PAL_BLOOD);
    drawPixel(img, 45, 40, PAL_BLOOD);
    drawPixel(img, 30, 50, PAL_BLOOD);
    // Subtle border
    for (int i = 0; i < 64; i++) {
      drawPixel(img, i, 63, PAL_CRACK);
      drawPixel(img, 63, i, PAL_CRACK);
    }
    ExportImage(img, "assets/sprites/tiles/floor.png");
    UnloadImage(img);
  }

  // WALL 64x64
  {
    Image img = GenImageColor(64, 64, PAL_WALL);
    // Stone texture
    for (int i = 0; i < 20; i++) {
      int x = (i * 13 + 5) % 62;
      int y = (i * 19 + 3) % 62;
      drawPixel(img, x, y, PAL_WALLHI);
      drawPixel(img, x + 1, y, PAL_WALLHI);
    }
    // Bottom highlight (depth)
    for (int x = 0; x < 64; x++) {
      drawPixel(img, x, 62, PAL_WALLHI);
      drawPixel(img, x, 63, PAL_WALLHI);
    }
    // Mortar lines
    for (int x = 0; x < 64; x++) {
      drawPixel(img, x, 20, {48, 43, 54, 255});
      drawPixel(img, x, 42, {48, 43, 54, 255});
    }
    for (int y = 0; y < 64; y++) {
      if (y < 20)
        drawPixel(img, 32, y, {48, 43, 54, 255});
      else if (y < 42) {
        drawPixel(img, 16, y, {48, 43, 54, 255});
        drawPixel(img, 48, y, {48, 43, 54, 255});
      } else
        drawPixel(img, 32, y, {48, 43, 54, 255});
    }
    ExportImage(img, "assets/sprites/tiles/wall.png");
    UnloadImage(img);
  }

  // SPIKES 64x64
  {
    Image img = GenImageColor(64, 64, PAL_FLOOR);
    // Draw spike triangles
    for (int s = 0; s < 4; s++) {
      int baseX = s * 16 + 2;
      Color spikeCol = (s % 2 == 0) ? PAL_BLOOD : PAL_FIRE;
      for (int row = 0; row < 20; row++) {
        int halfW = (20 - row) * 6 / 20;
        for (int dx = -halfW; dx <= halfW; dx++)
          drawPixel(img, baseX + 6 + dx, 50 - row, spikeCol);
      }
    }
    // Blood on tips
    for (int s = 0; s < 4; s++) {
      drawPixel(img, s * 16 + 8, 30, PAL_BLOOD);
      drawPixel(img, s * 16 + 8, 31, PAL_BLOOD);
    }
    ExportImage(img, "assets/sprites/tiles/spikes.png");
    UnloadImage(img);
  }

  // --- FLOOR DECORATION: CRACK (64x64, transparent bg) ---
  {
    Image img = GenImageColor(64, 64, BLANK);
    Color crackDk = {20, 18, 22, 200};
    Color crackLt = {30, 28, 33, 150};
    // Main diagonal crack
    for (int i = 0; i < 28; i++) {
      int x = 10 + i;
      int y = 8 + i + (i % 5 == 0 ? 1 : 0);
      drawPixel(img, x, y, crackDk);
      drawPixel(img, x + 1, y, crackLt);
    }
    // Branch crack 1 (from mid, going right)
    for (int i = 0; i < 12; i++) {
      int x = 24 + i;
      int y = 22 - (i / 3);
      drawPixel(img, x, y, crackDk);
    }
    // Branch crack 2 (from mid, going down-left)
    for (int i = 0; i < 10; i++) {
      int x = 22 - (i / 2);
      int y = 24 + i;
      drawPixel(img, x, y, crackDk);
    }
    // Small secondary crack
    for (int i = 0; i < 8; i++) {
      drawPixel(img, 45 + i, 50 + (i % 3), crackDk);
    }
    ExportImage(img, "assets/sprites/tiles/decor_crack.png");
    UnloadImage(img);
  }

  // --- FLOOR DECORATION: BLOOD (64x64, transparent bg) ---
  {
    Image img = GenImageColor(64, 64, BLANK);
    Color bloodDk = {100, 0, 0, 200};
    Color bloodMd = {139, 0, 0, 180};
    Color bloodLt = {80, 0, 0, 120};
    // Central splotch (irregular blob)
    drawCircle(img, 32, 32, 8, bloodMd);
    drawCircle(img, 30, 30, 5, bloodDk);
    drawCircle(img, 35, 34, 4, bloodDk);
    // Splatter droplets
    drawCircle(img, 22, 26, 2, bloodLt);
    drawCircle(img, 40, 24, 3, bloodLt);
    drawCircle(img, 26, 40, 2, bloodLt);
    drawCircle(img, 42, 38, 2, bloodLt);
    // Trailing drips
    drawPixel(img, 32, 41, bloodDk);
    drawPixel(img, 33, 43, bloodMd);
    drawPixel(img, 31, 44, bloodLt);
    drawPixel(img, 20, 30, bloodLt);
    drawPixel(img, 44, 30, bloodLt);
    ExportImage(img, "assets/sprites/tiles/decor_blood.png");
    UnloadImage(img);
  }

  // --- FLOOR DECORATION: BONES (64x64, transparent bg) ---
  {
    Image img = GenImageColor(64, 64, BLANK);
    Color boneHi = {220, 210, 190, 255};
    Color boneLo = {180, 170, 150, 200};
    // Bone 1: small femur shape (horizontal)
    drawRect(img, 14, 20, 12, 2, PAL_BONE);     // shaft
    drawRect(img, 13, 19, 3, 4, boneHi);          // knob L
    drawRect(img, 24, 19, 3, 4, boneHi);          // knob R
    // Bone 2: small bone (angled via pixels)
    drawPixel(img, 38, 30, boneHi);
    drawPixel(img, 39, 31, PAL_BONE);
    drawPixel(img, 40, 32, PAL_BONE);
    drawPixel(img, 41, 33, PAL_BONE);
    drawPixel(img, 42, 34, PAL_BONE);
    drawPixel(img, 43, 35, boneHi);
    // Bone 3: crossed bones (small X)
    for (int i = 0; i < 8; i++) {
      drawPixel(img, 28 + i, 40 + i, PAL_BONE);
      drawPixel(img, 35 - i, 40 + i, boneLo);
    }
    // Skull fragment
    drawRect(img, 46, 18, 4, 3, boneHi);
    drawRect(img, 47, 17, 2, 1, boneLo);
    drawPixel(img, 47, 19, PAL_BLACK);             // eye socket
    ExportImage(img, "assets/sprites/tiles/decor_bones.png");
    UnloadImage(img);
  }

  // --- FLOOR DECORATION: RUNE (64x64, transparent bg) ---
  {
    Image img = GenImageColor(64, 64, BLANK);
    Color runeCore = {100, 80, 200, 220};
    Color runeGlow = {80, 60, 180, 140};
    Color runeHi   = {140, 120, 255, 255};
    // Outer circle (ring of dots)
    for (int i = 0; i < 32; i++) {
      float angle = (float)i * 6.2832f / 32.0f;
      int rx = 32 + (int)(14.0f * cosf(angle));
      int ry = 32 + (int)(14.0f * sinf(angle));
      drawPixel(img, rx, ry, runeGlow);
    }
    // Inner circle
    for (int i = 0; i < 20; i++) {
      float angle = (float)i * 6.2832f / 20.0f;
      int rx = 32 + (int)(8.0f * cosf(angle));
      int ry = 32 + (int)(8.0f * sinf(angle));
      drawPixel(img, rx, ry, runeCore);
    }
    // Cross lines inside
    for (int i = -6; i <= 6; i++) {
      drawPixel(img, 32 + i, 32, runeHi);
      drawPixel(img, 32, 32 + i, runeHi);
    }
    // Diagonal accents
    for (int i = -4; i <= 4; i++) {
      drawPixel(img, 32 + i, 32 + i, runeGlow);
      drawPixel(img, 32 + i, 32 - i, runeGlow);
    }
    // Center glow
    drawCircle(img, 32, 32, 2, runeHi);
    // Corner sigils
    drawPixel(img, 20, 20, runeCore);
    drawPixel(img, 44, 20, runeCore);
    drawPixel(img, 20, 44, runeCore);
    drawPixel(img, 44, 44, runeCore);
    ExportImage(img, "assets/sprites/tiles/decor_rune.png");
    UnloadImage(img);
  }

  // --- TORCH SPRITE SHEET: 4 frames, 16x32 each → 64x32 ---
  {
    Image img = GenImageColor(64, 32, BLANK);
    Color stick   = {100, 70, 40, 255};
    Color stickDk = {70, 50, 30, 255};
    Color flameY  = {255, 200, 50, 255};
    Color flameO  = {255, 130, 0, 255};
    Color flameR  = {220, 80, 0, 200};
    Color flameHi = {255, 255, 180, 255};

    for (int f = 0; f < 4; f++) {
      int fx = f * 16;
      int cx = fx + 8;
      // Stick/handle
      drawRect(img, cx - 1, 14, 2, 16, stick);
      drawRect(img, cx - 2, 13, 4, 2, stickDk); // bracket
      drawPixel(img, cx, 30, stickDk);            // base
      // Flame (varies per frame)
      int flameH = 10 + (f % 2 == 0 ? 0 : 2);
      int flameW = 4 + (f == 1 || f == 3 ? 1 : 0);
      int flameTop = 13 - flameH;
      // Outer flame
      for (int fy = 0; fy < flameH; fy++) {
        float ratio = (float)fy / flameH;
        int hw = (int)(flameW * (1.0f - ratio * 0.7f));
        Color fc = (fy < flameH / 3) ? flameR : flameO;
        for (int dx = -hw; dx <= hw; dx++)
          drawPixel(img, cx + dx + (f == 1 ? 1 : f == 3 ? -1 : 0),
                    flameTop + fy, fc);
      }
      // Inner bright core
      for (int fy = flameH / 3; fy < flameH - 1; fy++) {
        int hw = (int)(2 * (1.0f - (float)fy / flameH));
        for (int dx = -hw; dx <= hw; dx++)
          drawPixel(img, cx + dx, flameTop + fy, flameY);
      }
      // Tip highlight
      drawPixel(img, cx + (f == 0 ? 0 : f == 1 ? 1 : f == 2 ? 0 : -1),
                flameTop, flameHi);
      drawPixel(img, cx, flameTop + 1, flameHi);
    }
    ExportImage(img, "assets/sprites/tiles/torch.png");
    UnloadImage(img);
  }

  // --- FIRE TRAP SPRITE SHEET: 2 frames, 64x64 each → 128x64 ---
  {
    Image img = GenImageColor(128, 64, BLANK);
    Color grate    = {80, 75, 70, 255};
    Color grateDk  = {50, 45, 40, 255};
    Color grateHi  = {100, 95, 90, 255};
    Color trapFloor = {30, 28, 35, 255};

    // Frame 1: floor with fire grating (inactive)
    {
      int fx = 0;
      // Floor base
      drawRect(img, fx, 0, 64, 64, trapFloor);
      // Grate frame (outer border)
      drawRect(img, fx + 8, 8, 48, 48, grateDk);
      drawRect(img, fx + 10, 10, 44, 44, grate);
      // Grate bars (horizontal)
      for (int i = 0; i < 5; i++) {
        int y = 14 + i * 9;
        drawRect(img, fx + 10, y, 44, 2, grateDk);
      }
      // Grate bars (vertical)
      for (int i = 0; i < 5; i++) {
        int x = fx + 14 + i * 9;
        drawRect(img, x, 10, 2, 44, grateDk);
      }
      // Corner rivets
      drawPixel(img, fx + 10, 10, grateHi);
      drawPixel(img, fx + 52, 10, grateHi);
      drawPixel(img, fx + 10, 52, grateHi);
      drawPixel(img, fx + 52, 52, grateHi);
    }

    // Frame 2: fire erupting from grating
    {
      int fx = 64;
      // Floor base
      drawRect(img, fx, 0, 64, 64, trapFloor);
      // Grate (same as frame 1)
      drawRect(img, fx + 8, 8, 48, 48, grateDk);
      drawRect(img, fx + 10, 10, 44, 44, grate);
      for (int i = 0; i < 5; i++) {
        int y = 14 + i * 9;
        drawRect(img, fx + 10, y, 44, 2, grateDk);
      }
      for (int i = 0; i < 5; i++) {
        int x = fx + 14 + i * 9;
        drawRect(img, x, 10, 2, 44, grateDk);
      }
      drawPixel(img, fx + 10, 10, grateHi);
      drawPixel(img, fx + 52, 10, grateHi);
      drawPixel(img, fx + 10, 52, grateHi);
      drawPixel(img, fx + 52, 52, grateHi);
      // Fire columns erupting from grate holes
      Color fireY = {255, 200, 50, 230};
      Color fireO = {255, 130, 0, 200};
      Color fireR = {200, 60, 0, 160};
      // Multiple fire columns in grate cells
      int cellCenters[][2] = {
        {19, 19}, {28, 19}, {37, 19}, {46, 19},
        {19, 28}, {28, 28}, {37, 28}, {46, 28},
        {19, 37}, {28, 37}, {37, 37}, {46, 37},
        {19, 46}, {28, 46}, {37, 46}, {46, 46}
      };
      for (int c = 0; c < 16; c++) {
        int ccx = fx + cellCenters[c][0];
        int ccy = cellCenters[c][1];
        // Small flame per cell
        drawPixel(img, ccx, ccy - 2, fireR);
        drawPixel(img, ccx, ccy - 1, fireO);
        drawPixel(img, ccx, ccy, fireY);
        drawPixel(img, ccx - 1, ccy, fireO);
        drawPixel(img, ccx + 1, ccy, fireO);
        drawPixel(img, ccx, ccy + 1, fireO);
      }
      // Large central flame burst
      for (int fy = 0; fy < 20; fy++) {
        float ratio = (float)fy / 20.0f;
        int hw = (int)(10.0f * (1.0f - ratio));
        Color fc = (fy < 5) ? fireY : (fy < 12) ? fireO : fireR;
        for (int dx = -hw; dx <= hw; dx++)
          drawPixel(img, fx + 32 + dx, 30 - fy + 10, fc);
      }
    }
    ExportImage(img, "assets/sprites/tiles/fire_trap.png");
    UnloadImage(img);
  }

  // --- PIT: 64x64, dark hole in the floor ---
  {
    Image img = GenImageColor(64, 64, PAL_FLOOR);
    Color pitEdge  = {35, 33, 40, 255};
    Color pitMid   = {18, 16, 22, 255};
    Color pitDeep  = {5, 3, 8, 255};
    // Outer ring (floor-edge transition)
    drawCircle(img, 32, 32, 28, pitEdge);
    // Mid darkness
    drawCircle(img, 32, 32, 22, pitMid);
    // Deep center
    drawCircle(img, 32, 32, 14, pitDeep);
    // Abyss core
    drawCircle(img, 32, 32, 8, PAL_BLACK);
    // Edge highlights (top lit)
    for (int i = 0; i < 20; i++) {
      float angle = 3.14159f + (float)i * 0.08f - 0.8f; // top arc
      int ex = 32 + (int)(26.0f * cosf(angle));
      int ey = 32 + (int)(26.0f * sinf(angle));
      drawPixel(img, ex, ey, PAL_CRACK);
    }
    ExportImage(img, "assets/sprites/tiles/pit.png");
    UnloadImage(img);
  }

  // --- PILLAR: 32x48, destructible stone pillar ---
  {
    Image img = GenImageColor(32, 48, BLANK);
    Color stoneBase = {120, 115, 110, 255};
    Color stoneLt   = {150, 145, 140, 255};
    Color stoneDk   = {80, 75, 70, 255};
    Color stoneVDk  = {60, 55, 50, 255};
    // Base (wider)
    drawRect(img, 4, 42, 24, 6, stoneDk);
    drawRect(img, 5, 43, 22, 4, stoneBase);
    // Column shaft
    drawRect(img, 7, 8, 18, 34, stoneBase);
    // Left shadow edge
    drawRect(img, 7, 8, 3, 34, stoneDk);
    // Right highlight edge
    drawRect(img, 22, 8, 3, 34, stoneLt);
    // Capital (top, wider)
    drawRect(img, 5, 4, 22, 5, stoneBase);
    drawRect(img, 6, 3, 20, 2, stoneLt);
    drawRect(img, 5, 4, 22, 1, stoneLt);
    // Cracks
    // Crack 1: diagonal from upper-left
    drawPixel(img, 12, 12, stoneVDk);
    drawPixel(img, 13, 13, stoneVDk);
    drawPixel(img, 14, 14, stoneVDk);
    drawPixel(img, 14, 15, stoneVDk);
    drawPixel(img, 15, 16, stoneVDk);
    // Crack 2: horizontal mid
    drawPixel(img, 10, 26, stoneVDk);
    drawPixel(img, 11, 26, stoneVDk);
    drawPixel(img, 12, 26, stoneVDk);
    drawPixel(img, 13, 27, stoneVDk);
    drawPixel(img, 14, 27, stoneVDk);
    // Crack 3: small chip lower
    drawPixel(img, 18, 34, stoneVDk);
    drawPixel(img, 19, 35, stoneVDk);
    drawPixel(img, 19, 36, stoneVDk);
    // Stone texture specks
    for (int i = 0; i < 12; i++) {
      int sx = 9 + (i * 11 % 14);
      int sy = 10 + (i * 17 % 28);
      drawPixel(img, sx, sy, (i % 2 == 0) ? stoneLt : stoneDk);
    }
    ExportImage(img, "assets/sprites/tiles/pillar.png");
    UnloadImage(img);
  }

  printf("[OK] Tile sprites generated\n");
}

static void generateFXSprites() {
  // Fireball 2 frames, 12x12 → 24x12
  {
    Image img = GenImageColor(24, 12, BLANK);
    drawCircle(img, 6, 6, 5, PAL_FIRE);
    drawCircle(img, 6, 6, 2, {255, 170, 0, 255});
    drawCircle(img, 18, 6, 5, {255, 100, 0, 255});
    drawCircle(img, 18, 6, 2, {255, 200, 50, 255});
    ExportImage(img, "assets/sprites/fx/fireball.png");
    UnloadImage(img);
  }

  // Health orb 2 frames → 32x16
  {
    Image img = GenImageColor(32, 16, BLANK);
    drawCircle(img, 8, 8, 6, PAL_HP);
    drawCircle(img, 8, 8, 2, {255, 150, 180, 255});
    drawCircle(img, 24, 8, 7, PAL_HP);
    drawCircle(img, 24, 8, 3, {255, 150, 180, 255});
    ExportImage(img, "assets/sprites/fx/orb_health.png");
    UnloadImage(img);
  }

  // Stamina orb 2 frames → 32x16
  {
    Image img = GenImageColor(32, 16, BLANK);
    drawCircle(img, 8, 8, 6, PAL_STAM);
    drawCircle(img, 8, 8, 2, {150, 255, 255, 255});
    drawCircle(img, 24, 8, 7, PAL_STAM);
    drawCircle(img, 24, 8, 3, {150, 255, 255, 255});
    ExportImage(img, "assets/sprites/fx/orb_stamina.png");
    UnloadImage(img);
  }

  // Hit particle 4x4
  {
    Image img = GenImageColor(4, 4, BLANK);
    drawRect(img, 0, 0, 4, 4, WHITE);
    ExportImage(img, "assets/sprites/fx/hit_particle.png");
    UnloadImage(img);
  }

  // Ember particle 4x4
  {
    Image img = GenImageColor(4, 4, BLANK);
    drawRect(img, 1, 0, 2, 4, PAL_FIRE);
    drawPixel(img, 2, 1, {255, 200, 50, 255});
    ExportImage(img, "assets/sprites/fx/ember.png");
    UnloadImage(img);
  }

  // Slash arc — curved attack trail, 3 frames 32x32 → 96x32
  {
    Image img = GenImageColor(96, 32, BLANK);
    // Frame 1: arc starting (small curve)
    for (int i = 0; i < 8; i++) {
      float angle = 0.5f + i * 0.15f;
      int ax = 16 + (int)(10.0f * cosf(angle));
      int ay = 16 + (int)(10.0f * sinf(angle));
      drawPixel(img, ax, ay, WHITE);
    }
    // Frame 2: full arc (bright sweep)
    for (int i = 0; i < 16; i++) {
      float angle = 0.3f + i * 0.15f;
      int ax = 32 + 16 + (int)(13.0f * cosf(angle));
      int ay = 16 + (int)(13.0f * sinf(angle));
      drawPixel(img, ax, ay, WHITE);
      drawPixel(img, ax + 1, ay, {255, 255, 255, 180});
      // Inner glow
      int ix = 32 + 16 + (int)(10.0f * cosf(angle));
      int iy = 16 + (int)(10.0f * sinf(angle));
      drawPixel(img, ix, iy, {200, 220, 255, 140});
    }
    // Frame 3: fading arc
    for (int i = 0; i < 16; i++) {
      float angle = 0.3f + i * 0.15f;
      int ax = 64 + 16 + (int)(14.0f * cosf(angle));
      int ay = 16 + (int)(14.0f * sinf(angle));
      drawPixel(img, ax, ay, {200, 200, 255, 100});
    }
    ExportImage(img, "assets/sprites/fx/slash_arc.png");
    UnloadImage(img);
  }

  // Dash ghost — afterimage silhouette, 8x16 (small, spawned multiple times)
  {
    Image img = GenImageColor(8, 16, BLANK);
    // Ghostly silhouette (simplified soul shape)
    drawRect(img, 2, 0, 4, 3, PAL_SOUL_GLOW);  // head
    drawRect(img, 1, 3, 6, 6, PAL_SOUL_GLOW);   // body
    drawRect(img, 2, 9, 2, 5, PAL_SOUL_GLOW);   // leg L
    drawRect(img, 4, 9, 2, 5, PAL_SOUL_GLOW);   // leg R
    drawPixel(img, 3, 1, {150, 170, 220, 100});  // faint eye
    ExportImage(img, "assets/sprites/fx/dash_ghost.png");
    UnloadImage(img);
  }

  // Item pickup — glowing golden gem/orb, 16x16
  {
    Image img = GenImageColor(16, 16, BLANK);
    Color gemGlow  = {255, 200, 50, 100};
    Color gemOuter = {220, 170, 30, 255};
    Color gemMid   = {255, 200, 50, 255};
    Color gemCore  = {255, 240, 150, 255};
    Color gemHi    = {255, 255, 220, 255};
    // Outer glow aura
    drawCircle(img, 8, 8, 7, gemGlow);
    // Gem body
    drawCircle(img, 8, 8, 5, gemOuter);
    drawCircle(img, 8, 8, 3, gemMid);
    // Inner bright core
    drawCircle(img, 8, 8, 1, gemCore);
    // Specular highlight (top-left)
    drawPixel(img, 6, 5, gemHi);
    drawPixel(img, 7, 5, gemHi);
    drawPixel(img, 6, 6, gemHi);
    // Sparkle accents
    drawPixel(img, 8, 2, gemHi);
    drawPixel(img, 14, 8, gemHi);
    drawPixel(img, 8, 14, gemHi);
    drawPixel(img, 2, 8, gemHi);
    ExportImage(img, "assets/sprites/fx/item_pickup.png");
    UnloadImage(img);
  }

  printf("[OK] FX sprites generated\n");
}

// ---- WAV Generation ---------------------------------------------------------
struct WavWriter {
  std::vector<int16_t> samples;
  int sampleRate = 44100;

  void addSine(float freq, float duration, float volume, float decay = 0.0f) {
    int n = (int)(duration * sampleRate);
    for (int i = 0; i < n; i++) {
      float t = (float)i / sampleRate;
      float env = volume * (decay > 0 ? expf(-decay * t) : 1.0f);
      float val = env * sinf(2.0f * 3.14159f * freq * t);
      samples.push_back((int16_t)(val * 32000));
    }
  }

  void addNoise(float duration, float volume, float decay = 0.0f) {
    int n = (int)(duration * sampleRate);
    for (int i = 0; i < n; i++) {
      float t = (float)i / sampleRate;
      float env = volume * (decay > 0 ? expf(-decay * t) : 1.0f);
      float val = env * ((float)(rand() % 20000 - 10000) / 10000.0f);
      samples.push_back((int16_t)(val * 32000));
    }
  }

  void addSweep(float freqStart, float freqEnd, float duration, float volume,
                float decay = 0.0f) {
    int n = (int)(duration * sampleRate);
    for (int i = 0; i < n; i++) {
      float t = (float)i / sampleRate;
      float frac = (float)i / n;
      float freq = freqStart + (freqEnd - freqStart) * frac;
      float env = volume * (decay > 0 ? expf(-decay * t) : 1.0f);
      float val = env * sinf(2.0f * 3.14159f * freq * t);
      samples.push_back((int16_t)(val * 32000));
    }
  }

  void addSquare(float freq, float duration, float volume, float decay = 0.0f) {
    int n = (int)(duration * sampleRate);
    for (int i = 0; i < n; i++) {
      float t = (float)i / sampleRate;
      float env = volume * (decay > 0 ? expf(-decay * t) : 1.0f);
      float phase = fmodf(freq * t, 1.0f);
      float val = env * (phase < 0.5f ? 1.0f : -1.0f);
      samples.push_back((int16_t)(val * 32000));
    }
  }

  void addSilence(float duration) {
    int n = (int)(duration * sampleRate);
    for (int i = 0; i < n; i++)
      samples.push_back(0);
  }

  void save(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
      printf("[ERR] Cannot write %s\n", path);
      return;
    }
    uint32_t dataSize = (uint32_t)(samples.size() * 2);
    uint32_t fileSize = 36 + dataSize;
    // RIFF header
    fwrite("RIFF", 1, 4, f);
    fwrite(&fileSize, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    // fmt chunk
    fwrite("fmt ", 1, 4, f);
    uint32_t fmtSize = 16;
    fwrite(&fmtSize, 4, 1, f);
    uint16_t audioFormat = 1; // PCM
    fwrite(&audioFormat, 2, 1, f);
    uint16_t channels = 1;
    fwrite(&channels, 2, 1, f);
    uint32_t sr = sampleRate;
    fwrite(&sr, 4, 1, f);
    uint32_t byteRate = sr * 2;
    fwrite(&byteRate, 4, 1, f);
    uint16_t blockAlign = 2;
    fwrite(&blockAlign, 2, 1, f);
    uint16_t bitsPerSample = 16;
    fwrite(&bitsPerSample, 2, 1, f);
    // data chunk
    fwrite("data", 1, 4, f);
    fwrite(&dataSize, 4, 1, f);
    fwrite(samples.data(), 2, samples.size(), f);
    fclose(f);
  }

  void clear() { samples.clear(); }
};

static void generateSFX() {
  WavWriter wav;

  // 1. attack_light — short percussive hit
  wav.addNoise(0.05f, 0.6f, 30.0f);
  wav.addSine(400, 0.05f, 0.4f, 20.0f);
  wav.save("assets/audio/sfx/attack_light.wav");
  wav.clear();

  // 2. attack_heavy — heavier, longer
  wav.addNoise(0.08f, 0.7f, 15.0f);
  wav.addSine(200, 0.1f, 0.5f, 10.0f);
  wav.save("assets/audio/sfx/attack_heavy.wav");
  wav.clear();

  // 3. hit_enemy — meaty impact
  wav.addNoise(0.04f, 0.8f, 40.0f);
  wav.addSine(300, 0.08f, 0.5f, 15.0f);
  wav.addSine(150, 0.05f, 0.3f, 20.0f);
  wav.save("assets/audio/sfx/hit_enemy.wav");
  wav.clear();

  // 4. hit_player — sharper, painful
  wav.addSine(800, 0.03f, 0.6f, 25.0f);
  wav.addNoise(0.06f, 0.5f, 20.0f);
  wav.addSine(600, 0.05f, 0.3f, 15.0f);
  wav.save("assets/audio/sfx/hit_player.wav");
  wav.clear();

  // 5. dash — whoosh sweep
  wav.addSweep(600, 150, 0.15f, 0.5f, 8.0f);
  wav.addNoise(0.1f, 0.2f, 15.0f);
  wav.save("assets/audio/sfx/dash.wav");
  wav.clear();

  // 6. player_death — heavy, dramatic
  wav.addSine(120, 0.3f, 0.7f, 3.0f);
  wav.addNoise(0.2f, 0.4f, 5.0f);
  wav.addSine(80, 0.5f, 0.5f, 2.0f);
  wav.save("assets/audio/sfx/player_death.wav");
  wav.clear();

  // 7. enemy_death — shorter explosion
  wav.addNoise(0.1f, 0.7f, 12.0f);
  wav.addSine(150, 0.15f, 0.5f, 8.0f);
  wav.save("assets/audio/sfx/enemy_death.wav");
  wav.clear();

  // 8. pickup_health — pleasant ascending
  wav.addSine(440, 0.08f, 0.4f, 5.0f);
  wav.addSine(554, 0.08f, 0.4f, 5.0f);
  wav.addSine(660, 0.12f, 0.5f, 4.0f);
  wav.save("assets/audio/sfx/pickup_health.wav");
  wav.clear();

  // 9. pickup_stamina — brighter ascending
  wav.addSine(523, 0.06f, 0.4f, 5.0f);
  wav.addSine(659, 0.06f, 0.4f, 5.0f);
  wav.addSine(784, 0.1f, 0.5f, 4.0f);
  wav.save("assets/audio/sfx/pickup_stamina.wav");
  wav.clear();

  // 10. menu_select — soft click
  wav.addSine(800, 0.03f, 0.3f, 30.0f);
  wav.addSine(600, 0.02f, 0.2f, 40.0f);
  wav.save("assets/audio/sfx/menu_select.wav");
  wav.clear();

  // 11. menu_confirm — ascending confirmation
  wav.addSine(400, 0.06f, 0.4f, 10.0f);
  wav.addSine(600, 0.06f, 0.4f, 10.0f);
  wav.addSine(800, 0.1f, 0.5f, 6.0f);
  wav.save("assets/audio/sfx/menu_confirm.wav");
  wav.clear();

  // 12. boss_roar — deep, intimidating
  wav.addSine(60, 0.4f, 0.8f, 2.0f);
  wav.addNoise(0.3f, 0.5f, 3.0f);
  wav.addSine(40, 0.5f, 0.6f, 1.5f);
  wav.addSweep(80, 30, 0.3f, 0.4f, 3.0f);
  wav.save("assets/audio/sfx/boss_roar.wav");
  wav.clear();

  // 13. boss_charge — building intensity
  wav.addSweep(100, 500, 0.3f, 0.5f, 2.0f);
  wav.addNoise(0.15f, 0.3f, 5.0f);
  wav.save("assets/audio/sfx/boss_charge.wav");
  wav.clear();

  // 14. boss_slam — massive impact
  wav.addNoise(0.08f, 0.9f, 15.0f);
  wav.addSine(60, 0.3f, 0.8f, 3.0f);
  wav.addSine(40, 0.2f, 0.5f, 4.0f);
  wav.addNoise(0.15f, 0.3f, 8.0f);
  wav.save("assets/audio/sfx/boss_slam.wav");
  wav.clear();

  // 15. phase_transition — dramatic crescendo
  wav.addSweep(100, 800, 0.5f, 0.4f, 1.0f);
  wav.addSine(800, 0.3f, 0.6f, 3.0f);
  wav.addNoise(0.2f, 0.3f, 5.0f);
  wav.addSine(400, 0.4f, 0.5f, 2.0f);
  wav.save("assets/audio/sfx/phase_transition.wav");
  wav.clear();

  // game_start
  wav.addSweep(200, 600, 0.3f, 0.4f, 3.0f);
  wav.addSine(600, 0.15f, 0.5f, 5.0f);
  wav.save("assets/audio/sfx/game_start.wav");
  wav.clear();

  // death (alias for player_death — shorter version)
  wav.addSine(150, 0.2f, 0.6f, 4.0f);
  wav.addNoise(0.15f, 0.4f, 6.0f);
  wav.save("assets/audio/sfx/death.wav");
  wav.clear();

  printf("[OK] SFX generated (17 files)\n");
}

static void generateMusic() {
  WavWriter wav;

  // --- MENU MUSIC: dark ambient drone, ~16 seconds loop ---
  {
    float bpm = 80;
    float beatLen = 60.0f / bpm;
    // 16 bars of 4 beats = 64 beats
    int totalBeats = 32; // ~24 seconds
    for (int beat = 0; beat < totalBeats; beat++) {
      float t = beat * beatLen;
      (void)t;
      // Bass drone: C2 (65 Hz) with slow wobble
      float freq = 65.0f + 3.0f * sinf(beat * 0.3f);
      wav.addSine(freq, beatLen * 0.9f, 0.25f, 0.5f);
      wav.addSilence(beatLen * 0.1f);
    }
    // Overlay: higher eerie tone
    WavWriter overlay;
    for (int beat = 0; beat < totalBeats; beat++) {
      if (beat % 8 < 4) {
        overlay.addSine(196.0f, beatLen, 0.1f, 1.0f); // G3
      } else {
        overlay.addSine(185.0f, beatLen, 0.1f, 1.0f); // F#3
      }
    }
    // Mix overlay into main
    for (int i = 0; i < (int)overlay.samples.size() && i < (int)wav.samples.size(); i++) {
      int mixed = wav.samples[i] + overlay.samples[i];
      if (mixed > 32000) mixed = 32000;
      if (mixed < -32000) mixed = -32000;
      wav.samples[i] = (int16_t)mixed;
    }
    wav.save("assets/audio/music/menu.wav");
    wav.clear();
    printf("[OK] Menu music generated\n");
  }

  // --- CIRCLE 7 EXPLORATION: tense, rhythmic, tribal ---
  {
    float bpm = 100;
    float beatLen = 60.0f / bpm;
    int totalBeats = 64; // ~38 seconds

    // Bass line
    float bassNotes[] = {65.0f, 65.0f, 73.0f, 65.0f}; // C2, C2, D2, C2
    for (int beat = 0; beat < totalBeats; beat++) {
      float note = bassNotes[beat % 4];
      wav.addSquare(note, beatLen * 0.7f, 0.15f, 3.0f);
      wav.addSilence(beatLen * 0.3f);
    }

    // Percussion overlay
    WavWriter perc;
    for (int beat = 0; beat < totalBeats; beat++) {
      if (beat % 4 == 0) {
        // Kick
        perc.addSweep(150, 40, 0.08f, 0.6f, 15.0f);
        perc.addSilence(beatLen - 0.08f);
      } else if (beat % 4 == 2) {
        // Snare
        perc.addNoise(0.06f, 0.3f, 20.0f);
        perc.addSilence(beatLen - 0.06f);
      } else if (beat % 2 == 1) {
        // Hi-hat
        perc.addNoise(0.02f, 0.15f, 50.0f);
        perc.addSilence(beatLen - 0.02f);
      } else {
        perc.addSilence(beatLen);
      }
    }

    // Melody overlay (sparse, eerie)
    WavWriter melody;
    // Minor scale notes: C3, Eb3, F3, G3, Bb3
    float melNotes[] = {130.8f, 155.6f, 174.6f, 196.0f, 233.1f, 0, 0, 0};
    for (int beat = 0; beat < totalBeats; beat++) {
      int idx = beat % 8;
      if (melNotes[idx] > 0 && (beat / 8) % 2 == 0) {
        melody.addSine(melNotes[idx], beatLen * 0.8f, 0.12f, 4.0f);
        melody.addSilence(beatLen * 0.2f);
      } else {
        melody.addSilence(beatLen);
      }
    }

    // Mix all layers
    size_t maxLen = wav.samples.size();
    if (perc.samples.size() > maxLen) maxLen = perc.samples.size();
    if (melody.samples.size() > maxLen) maxLen = melody.samples.size();
    wav.samples.resize(maxLen, 0);
    perc.samples.resize(maxLen, 0);
    melody.samples.resize(maxLen, 0);

    for (size_t i = 0; i < maxLen; i++) {
      int mixed = wav.samples[i] + perc.samples[i] + melody.samples[i];
      if (mixed > 32000) mixed = 32000;
      if (mixed < -32000) mixed = -32000;
      wav.samples[i] = (int16_t)mixed;
    }

    wav.save("assets/audio/music/circle_7.wav");
    wav.clear();
    printf("[OK] Circle 7 music generated\n");
  }

  // --- BOSS MUSIC: fast, aggressive, intense ---
  {
    float bpm = 140;
    float beatLen = 60.0f / bpm;
    int totalBeats = 96; // ~41 seconds

    // Heavy bass
    float bassNotes[] = {55.0f, 55.0f, 61.7f, 55.0f, 65.4f, 55.0f, 61.7f, 49.0f};
    for (int beat = 0; beat < totalBeats; beat++) {
      float note = bassNotes[beat % 8];
      wav.addSquare(note, beatLen * 0.8f, 0.2f, 4.0f);
      wav.addSilence(beatLen * 0.2f);
    }

    // Aggressive percussion
    WavWriter perc;
    for (int beat = 0; beat < totalBeats; beat++) {
      if (beat % 2 == 0) {
        // Kick on every other beat
        perc.addSweep(200, 40, 0.06f, 0.7f, 20.0f);
        perc.addSilence(beatLen - 0.06f);
      } else {
        // Snare
        perc.addNoise(0.05f, 0.5f, 25.0f);
        perc.addSilence(beatLen - 0.05f);
      }
    }

    // Intense melody (phrygian scale: C, Db, Eb, F, G, Ab, Bb)
    WavWriter melody;
    float phrygian[] = {261.6f, 277.2f, 311.1f, 349.2f, 392.0f, 415.3f, 466.2f, 0};
    for (int beat = 0; beat < totalBeats; beat++) {
      int bar = beat / 8;
      int pos = beat % 8;
      if (bar % 2 == 0 && pos < 6) {
        melody.addSine(phrygian[pos], beatLen * 0.6f, 0.15f, 6.0f);
        melody.addSilence(beatLen * 0.4f);
      } else if (bar % 2 == 1 && pos >= 4) {
        melody.addSine(phrygian[7 - pos], beatLen * 0.5f, 0.12f, 8.0f);
        melody.addSilence(beatLen * 0.5f);
      } else {
        melody.addSilence(beatLen);
      }
    }

    // Mix
    size_t maxLen = wav.samples.size();
    if (perc.samples.size() > maxLen) maxLen = perc.samples.size();
    if (melody.samples.size() > maxLen) maxLen = melody.samples.size();
    wav.samples.resize(maxLen, 0);
    perc.samples.resize(maxLen, 0);
    melody.samples.resize(maxLen, 0);

    for (size_t i = 0; i < maxLen; i++) {
      int mixed = wav.samples[i] + perc.samples[i] + melody.samples[i];
      if (mixed > 32000) mixed = 32000;
      if (mixed < -32000) mixed = -32000;
      wav.samples[i] = (int16_t)mixed;
    }

    wav.save("assets/audio/music/boss.wav");
    wav.clear();
    printf("[OK] Boss music generated\n");
  }
}

// ---- Main -------------------------------------------------------------------
int main() {
  // Need Raylib window for Image operations
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(1, 1, "Asset Generator");

  printf("=== INFERNUS Asset Generator ===\n\n");

  generatePlayerSprites();
  generateClassSprites();
  generateEnemySprites();
  generateBossSprites();
  generateTileSprites();
  generateFXSprites();
  generateSFX();
  generateMusic();

  printf("\n=== All assets generated! ===\n");

  CloseWindow();
  return 0;
}
