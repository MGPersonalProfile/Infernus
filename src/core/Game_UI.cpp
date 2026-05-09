#include "Game.h"
#include "../debug/DebugPanel.h"
#include "../debug/Profiler.h"
#include "../scripting/LuaEngine.h"
#include "../systems/PartikelEmitters.h"
#include "../world/LDtkRoomLoader.h"
#include "../components/AIBehavior.h"
#include "../components/AnimState.h"
#include "../components/Animation.h"
#include "../components/BossPhase.h"
#include "../components/DamageNumber.h"
#include "../components/ItemPickup.h"
#include "../components/Lifetime.h"
#include "../components/Loot.h"
#include "../components/MiniBoss.h"
#include "../components/Particle.h"
#include "../components/PlayerStats.h"
#include "../utils/Constants.h"
#include "../utils/TextUtils.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <json.hpp>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

// Defined in Game.cpp
extern int infoMenuTab;


// === UI: all draw* functions for menus, HUD, screens ===

// =============================================================================
// HUD — minimalist: HP, Stamina, special cooldown indicator only.
// Everything else (controls, synergies, items, abilities, room) lives
// in the INFO menu (TAB).
// =============================================================================
void Game::drawHUD() {
  if (!registry.isAlive(playerEntity))
    return;

  if (registry.hasComponent<Health>(playerEntity) &&
      registry.hasComponent<Stamina>(playerEntity)) {
    auto &health = registry.getComponent<Health>(playerEntity);
    auto &stam = registry.getComponent<Stamina>(playerEntity);

    // HP Bar — bigger, more present (was 300x22 → 360x28). Dark border for
    // contrast vs the warm fill (was gold border which blended with stamina).
    float hpRatio = std::max(0.0f, (float)health.currentHP / health.maxHP);
    const int HP_W = 360, HP_H = 28;
    DrawRectangle(20, 20, HP_W, HP_H, Color{40, 5, 5, 220});
    int hpFill = (int)(HP_W * hpRatio);
    DrawRectangleGradientV(20, 20, hpFill, HP_H / 2, Color{230, 70, 50, 255},
                           Color{170, 30, 25, 255});
    DrawRectangleGradientV(20, 20 + HP_H / 2, hpFill, HP_H / 2, Color{150, 25, 20, 255},
                           Color{100, 10, 10, 255});
    DrawRectangleLinesEx({20, 20, (float)HP_W, (float)HP_H}, 2.0f,
                         Color{60, 20, 15, 255}); // dark crimson border
    TextUtils::drawOutlined(TextFormat("HP %d/%d", health.currentHP, health.maxHP), 28, 25, 14,
             WHITE, 2);

    // Stamina Bar — proportional to HP (240x16). Border darkened to contrast
    // against the amber fill (was {150,130,70} which blended).
    float stRatio = stam.currentStamina / stam.maxStamina;
    const int ST_W = 240, ST_H = 16;
    int ST_Y = 20 + HP_H + 6;
    DrawRectangle(20, ST_Y, ST_W, ST_H, Color{25, 18, 8, 220});
    int stFill = (int)(ST_W * stRatio);
    DrawRectangleGradientV(20, ST_Y, stFill, ST_H / 2, Color{230, 190, 90, 255},
                           Color{180, 140, 60, 255});
    DrawRectangleGradientV(20, ST_Y + ST_H / 2, stFill, ST_H / 2, Color{160, 120, 50, 255},
                           Color{110, 80, 30, 255});
    DrawRectangleLinesEx({20, (float)ST_Y, (float)ST_W, (float)ST_H}, 2.0f,
                         Color{50, 30, 10, 255}); // dark amber border
  }

  // Combo counter (show when combo is active)
  if (registry.hasComponent<Combat>(playerEntity)) {
    auto &combat = registry.getComponent<Combat>(playerEntity);
    if (combat.comboCount > 0 && combat.comboTimer > 0.0f) {
      unsigned char alpha = (unsigned char)(combat.comboTimer / Constants::COMBO_WINDOW * 255);
      Color comboColor = combat.comboCount >= 3
        ? Color{255, 200, 50, alpha}   // gold = finisher ready
        : Color{255, 255, 255, alpha}; // white = chaining
      TextUtils::drawOutlined(TextFormat("COMBO x%d", combat.comboCount), 20, 68, 12,
               comboColor, 2);
      if (combat.comboCount >= 3)
        TextUtils::drawOutlined("K -> FINISHER!", 20, 84, 10,
                 Color{255, 180, 30, alpha}, 1);
    }
    // Parry indicator
    if (combat.currentState == AttackState::PARRY_ACTIVE) {
      TextUtils::drawOutlined("PARRY!", 20, 68, 14,
               Color{200, 230, 255, 255}, 2);
    }
  }

  // Special cooldown indicator (L key) retired in v3 fix — class signature
  // ability is now in Q/E slots which already show their own cooldown overlay.

  // === Active abilities HUD (Q / E slots, bottom-right) ===
  if (registry.hasComponent<ActiveAbilities>(playerEntity)) {
    auto &actives = registry.getComponent<ActiveAbilities>(playerEntity);
    int slotSize = 56;
    int gap = 8;
    int totalW = slotSize * 2 + gap;
    int slotsY = screenHeight - slotSize - 20;
    int slotsX = screenWidth - totalW - 20;

    auto drawSlot = [&](int idx, const char *keyLabel, bool has,
                        const ActiveAbilityData &a, float cooldown) {
      int x = slotsX + idx * (slotSize + gap);
      int y = slotsY;

      // Frame background
      DrawRectangle(x, y, slotSize, slotSize, Color{20, 12, 8, 220});
      // Border: dimmed when on cooldown / unequipped, gold-pulsing when ready
      Color borderCol;
      if (!has) {
        borderCol = Color{60, 50, 40, 255};
      } else if (cooldown > 0.0f) {
        borderCol = Color{120, 90, 50, 220};
      } else {
        // Ready: pulse gold ↔ bright gold using sine wave
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
        unsigned char g = (unsigned char)(160 + 70 * pulse);
        borderCol = Color{230, g, 80, 255};
      }
      DrawRectangleLinesEx({(float)x, (float)y, (float)slotSize, (float)slotSize},
                           2.0f, borderCol);

      if (!has) {
        // Empty slot — show key label only
        TextUtils::draw(keyLabel, x + slotSize / 2 - 4, y + slotSize / 2 - 8,
                        16, Color{80, 70, 60, 255});
        return;
      }

      // Icon (preferred) — fallback to truncated name + key letter big
      Texture2D icon = ResourceManager::getInstance().getTexture(a.iconPath);
      bool hasIcon = (icon.id > 0);
      if (hasIcon) {
        Rectangle src = {0, 0, (float)icon.width, (float)icon.height};
        Rectangle dst = {(float)(x + 4), (float)(y + 4),
                         (float)(slotSize - 8), (float)(slotSize - 8)};
        DrawTexturePro(icon, src, dst, {0, 0}, 0.0f, WHITE);
      } else {
        // Fallback: big key letter centered + truncated name below.
        // Truncate keeps slot width sacred; "Lanza de Flegetonte" → "Lanza..."
        TextUtils::drawOutlined(keyLabel, x + slotSize / 2 - 6,
                                y + slotSize / 2 - 14, 22,
                                Color{220, 180, 100, 255}, 1);
        std::string truncName = TextUtils::truncate(a.name, 8, slotSize - 8);
        int nw = TextUtils::measure(truncName.c_str(), 7);
        TextUtils::draw(truncName.c_str(), x + (slotSize - nw) / 2,
                        y + slotSize - 12, 7,
                        Color{200, 180, 140, 255});
      }

      // Cooldown overlay
      if (cooldown > 0.0f) {
        float ratio = cooldown / a.cooldown;
        int overlayH = (int)(slotSize * ratio);
        DrawRectangle(x, y + slotSize - overlayH, slotSize, overlayH,
                      Color{0, 0, 0, 180});
        TextUtils::drawOutlined(TextFormat("%.1f", cooldown),
                                x + slotSize / 2 - 8, y + slotSize / 2 - 6, 12,
                                Color{220, 200, 160, 255}, 1);
      }

      // Key label (top-left corner)
      TextUtils::drawOutlined(keyLabel, x + 3, y + 3, 10,
                              Color{220, 180, 100, 255}, 1);
    };

    drawSlot(0, "Q", actives.hasQ, actives.slotQ, actives.cooldownQ);
    drawSlot(1, "E", actives.hasE, actives.slotE, actives.cooldownE);
  }
}


void Game::drawBossHealthBar() {
  if (!registry.hasComponent<Health>(bossEntity) ||
      !registry.hasComponent<BossPhase>(bossEntity))
    return;

  auto &health = registry.getComponent<Health>(bossEntity);
  auto &bp = registry.getComponent<BossPhase>(bossEntity);

  float ratio = std::max(0.0f, (float)health.currentHP / health.maxHP);
  int barW = 500, barH = 26;
  int barX = (screenWidth - barW) / 2;
  int barY = screenHeight - 60;

  // Dark background
  DrawRectangle(barX - 2, barY - 2, barW + 4, barH + 4, Color{20, 10, 5, 220});
  DrawRectangle(barX, barY, barW, barH, Color{40, 5, 0, 255});
  int fill = (int)(barW * ratio);
  if (bp.enraged) {
    DrawRectangleGradientV(barX, barY, fill, barH / 2,
                           Color{255, 120, 0, 255}, Color{255, 60, 0, 255});
    DrawRectangleGradientV(barX, barY + barH / 2, fill, barH / 2,
                           Color{200, 40, 0, 255}, Color{140, 20, 0, 255});
  } else {
    DrawRectangleGradientV(barX, barY, fill, barH / 2,
                           Color{220, 50, 40, 255}, Color{180, 30, 25, 255});
    DrawRectangleGradientV(barX, barY + barH / 2, fill, barH / 2,
                           Color{150, 20, 15, 255}, Color{100, 10, 5, 255});
  }
  DrawRectangleLinesEx({(float)barX, (float)barY, (float)barW, (float)barH},
                       2.0f, Color{200, 170, 100, 255});

  const char *name = bp.bossName.c_str();
  TextUtils::drawCenteredOutlined(name, barY - 25, 14, Color{230, 190, 100, 255}, screenWidth, 2, Color{0, 0, 0, 255});

  TextUtils::draw(TextFormat("Fase %d/%d", bp.currentPhase + 1, bp.totalPhases),
           barX + barW + 10, barY + 5, 10, Color{200, 200, 200, 255});
}


void Game::drawMainMenu() {
  // Background art
  Texture2D bgTex = ResourceManager::getInstance().getTexture(
      "assets/art/title_bg.png");
  if (bgTex.id > 0) {
    float scaleX = (float)screenWidth / bgTex.width;
    float scaleY = (float)screenHeight / bgTex.height;
    float scale = std::max(scaleX, scaleY);
    float drawW = bgTex.width * scale;
    float drawH = bgTex.height * scale;
    DrawTexturePro(bgTex, {0, 0, (float)bgTex.width, (float)bgTex.height},
                   {(screenWidth - drawW) / 2.0f, (screenHeight - drawH) / 2.0f,
                    drawW, drawH},
                   {0, 0}, 0.0f, WHITE);
    // Dark overlay for readability
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 120});
  }

  // Vignette overlay
  DrawRectangleGradientV(0, 0, screenWidth, screenHeight / 4,
                         Color{0, 0, 0, 200}, Color{0, 0, 0, 0});
  DrawRectangleGradientV(0, screenHeight * 3 / 4, screenWidth, screenHeight / 4,
                         Color{0, 0, 0, 0}, Color{0, 0, 0, 220});

  // Title with shadow/outline
  int ty = screenHeight / 4 - 20;
  TextUtils::drawCenteredOutlined("INFERNUS", ty, 48,
           Color{200, 40, 20, 255}, screenWidth, 3, Color{0,0,0,255});

  // Subtitle
  TextUtils::drawCenteredOutlined("\"Abandonad toda esperanza\"",
           ty + 64, 10, Color{180, 150, 100, 255}, screenWidth, 1);

  // Menu items with glow effect
  int menuY = screenHeight / 2 + 40;
  float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
  unsigned char alpha = (unsigned char)(180 + 75 * pulse);
  TextUtils::drawCenteredOutlined("[ENTER] Comenzar", menuY, 18,
           Color{255, 220, 150, alpha}, screenWidth, 2);
  TextUtils::drawCentered("[ESC] Opciones", menuY + 40, 14,
           Color{140, 130, 120, 255}, screenWidth);
  TextUtils::drawCentered("[ALT+F4] Salir", menuY + 70, 10,
           Color{90, 85, 80, 255}, screenWidth);

  // Version
  TextUtils::draw("v0.1.0 Alfa — Circulo VII", 10, screenHeight - 18, 8,
           Color{60, 60, 60, 180});
}


void Game::drawPauseMenu() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 180});

  int panelW = 500, panelH = 280;
  int px = (screenWidth - panelW) / 2, py = screenHeight / 3 - 40;
  drawUIPanel(px, py, panelW, panelH, Color{15, 15, 25, 240},
              Color{180, 160, 100, 255});

  TextUtils::drawCenteredOutlined("PAUSA", py + 35, 28, Color{220, 200, 150, 255}, screenWidth, 2);

  int itemY = py + 80;
  int itemH = 50;
  const char *items[] = {"Continuar", "Opciones", "Abandonar Run"};
  int cx = screenWidth / 2;

  for (int i = 0; i < 3; i++) {
    int y = itemY + i * itemH;
    bool sel = (i == pauseSelected);

    // Highlight bar for selected item
    if (sel) {
      DrawRectangle(cx - 180, y - 6, 360, 36, Color{30, 25, 40, 255});
      DrawRectangleLinesEx({(float)(cx - 180), (float)(y - 6), 360, 36}, 1.0f,
                           Color{220, 180, 100, 120});
    }

    float pulse = sel ? (0.5f + 0.5f * sinf((float)GetTime() * 4.0f)) : 0.0f;
    unsigned char alpha = sel ? (unsigned char)(200 + 55 * pulse) : 160;
    Color col = sel ? Color{255, 220, 140, alpha} : Color{160, 160, 160, alpha};
    int fontSize = sel ? 14 : 12;

    TextUtils::drawCenteredOutlined(items[i], y + 4, fontSize, col, screenWidth, 1);

    // Selection arrow
    if (sel) {
      int tw = TextUtils::measure(items[i], fontSize);
      TextUtils::draw(">", cx - tw / 2 - 20, y + 4, fontSize, Color{220, 180, 100, 255});
    }
  }

  TextUtils::drawCentered("[ESC] Volver al juego", py + panelH - 30, 8,
           Color{80, 80, 80, 200}, screenWidth);
}


void Game::drawOptions() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 5, 15, 255});

  int mainW = 650, mainH = 550;
  int mainX = (screenWidth - mainW) / 2, mainY = 20;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{12, 12, 22, 230},
              Color{180, 140, 60, 255});

  TextUtils::drawCenteredOutlined("OPCIONES", mainY + 55, 24, Color{220, 180, 100, 255}, screenWidth, 2);

  auto &audio = AudioManager::getInstance();
  int cx = screenWidth / 2;
  int startY = mainY + 110;
  int rowH = 42;

  // Options: SFX, Music, Fullscreen, ScreenShake, DamageNumbers, Back
  int optionCount = 6;

  for (int i = 0; i < optionCount; i++) {
    int y = startY + i * rowH;
    bool sel = (i == optionSelected);
    Color labelCol = sel ? Color{255, 220, 140, 255} : Color{180, 180, 180, 255};
    Color valCol = sel ? WHITE : Color{150, 150, 150, 255};

    // Highlight bar
    if (sel) {
      DrawRectangle(cx - 230, y - 5, 460, 34, Color{30, 30, 50, 255});
      DrawRectangleLinesEx({(float)(cx - 230), (float)(y - 5), 460, 34}, 1.0f,
                           Color{220, 180, 100, 120});
    }

    if (i < 2) {
      // Volume sliders
      const char *label = (i == 0) ? "Volumen SFX" : "Volumen Musica";
      float vol = (i == 0) ? audio.getSFXVolume() : audio.getMusicVolume();
      TextUtils::draw(label, cx - 210, y + 5, 10, labelCol);

      int barX = cx + 50;
      int barW = 120;
      int barY = y + 7;
      DrawRectangle(barX, barY, barW, 12, Color{40, 40, 50, 255});
      int fillW = (int)(vol * barW);
      DrawRectangle(barX, barY, fillW, 12, sel ? Color{220, 180, 100, 255} : Color{120, 120, 140, 255});
      DrawRectangleLines(barX, barY, barW, 12, Color{80, 80, 100, 255});
      TextUtils::draw(TextFormat("%d%%", (int)(vol * 100)),
               barX + barW + 10, y + 5, 10, valCol);
      if (sel) {
        TextUtils::draw("<", barX - 14, y + 5, 10, Color{220, 180, 100, 255});
        TextUtils::draw(">", barX + barW + 45, y + 5, 10, Color{220, 180, 100, 255});
      }
    } else if (i == 2) {
      TextUtils::draw("Pantalla Completa", cx - 210, y + 5, 10, labelCol);
      const char *val = IsWindowFullscreen() ? "SI" : "NO";
      TextUtils::draw(val, cx + 100, y + 5, 10, valCol);
    } else if (i == 3) {
      TextUtils::draw("Screen Shake", cx - 210, y + 5, 10, labelCol);
      const char *val = screenShakeEnabled ? "SI" : "NO";
      TextUtils::draw(val, cx + 100, y + 5, 10, val[0] == 'S' ? Color{100, 255, 100, 255} : Color{255, 80, 80, 255});
    } else if (i == 4) {
      TextUtils::draw("Numeros de Dano", cx - 210, y + 5, 10, labelCol);
      const char *val = damageNumbersEnabled ? "SI" : "NO";
      TextUtils::draw(val, cx + 100, y + 5, 10, val[0] == 'S' ? Color{100, 255, 100, 255} : Color{255, 80, 80, 255});
    } else {
      TextUtils::drawCentered("< Volver >", y + 5, 12, labelCol, screenWidth);
    }
  }

  // Keybinds reference
  int kbY = startY + optionCount * rowH + 10;
  DrawRectangle(cx - 230, kbY, 460, 1, Color{60, 50, 40, 200});
  TextUtils::drawCentered("CONTROLES", kbY + 8, 10, Color{180, 150, 100, 200}, screenWidth);
  int ky = kbY + 26;
  Color kc = Color{120, 120, 130, 200};
  TextUtils::draw("WASD  Movimiento", cx - 190, ky, 10, kc);
  TextUtils::draw("J     Ataque (combo x3)", cx + 30, ky, 10, kc);
  ky += 16;
  TextUtils::draw("K     Pesado / Finisher", cx - 190, ky, 10, kc);
  TextUtils::draw("F     Parry", cx + 30, ky, 10, kc);
  ky += 16;
  TextUtils::draw("SPACE Dash", cx - 190, ky, 10, kc);
  TextUtils::draw("L     Especial", cx + 30, ky, 10, kc);
  ky += 16;
  TextUtils::draw("I     Inventario", cx - 190, ky, 10, kc);
  TextUtils::draw("TAB   Info / H Habs", cx + 30, ky, 10, kc);

  TextUtils::drawCentered("[ESC] Volver", screenHeight - 30, 8,
           Color{80, 80, 80, 255}, screenWidth);
}


void Game::drawGameOver() {
  // Dark red background
  Texture2D bgTex = ResourceManager::getInstance().getTexture(
      "assets/art/title_bg.png");
  if (bgTex.id > 0) {
    float scale = std::max((float)screenWidth / bgTex.width,
                           (float)screenHeight / bgTex.height);
    DrawTexturePro(bgTex, {0, 0, (float)bgTex.width, (float)bgTex.height},
                   {0, 0, bgTex.width * scale, bgTex.height * scale},
                   {0, 0}, 0.0f, Color{40, 40, 40, 255}); // Removed red tint
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 180});
  }

  // Pulsing title
  float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 2.0f);
  unsigned char titleAlpha = (unsigned char)(180 + 75 * pulse);
  TextUtils::drawCenteredOutlined("HAS MUERTO", screenHeight / 6, 36,
           Color{200, 30, 20, titleAlpha}, screenWidth, 3, Color{0, 0, 0, 255});

  // Decorative line
  int lineY = screenHeight / 6 + 50;
  DrawRectangleGradientH(screenWidth / 2 - 200, lineY, 200, 1,
           Color{0, 0, 0, 0}, Color{140, 20, 10, 200});
  DrawRectangleGradientH(screenWidth / 2, lineY, 200, 1,
           Color{140, 20, 10, 200}, Color{0, 0, 0, 0});

  drawRunStats();

  // Action buttons with visual focus
  float btnPulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.5f);
  unsigned char enterAlpha = (unsigned char)(180 + 75 * btnPulse);
  TextUtils::drawCenteredOutlined("[ ENTER ] Reintentar", screenHeight - 85, 12,
           Color{220, 180, 100, enterAlpha}, screenWidth, 2);
  TextUtils::drawCentered("[ ESC ] Menu Principal", screenHeight - 60, 10,
           Color{120, 110, 100, 200}, screenWidth);
}


void Game::drawVictory() {
  // Golden-tinted background
  Texture2D bgTex = ResourceManager::getInstance().getTexture(
      "assets/art/title_bg.png");
  if (bgTex.id > 0) {
    float scale = std::max((float)screenWidth / bgTex.width,
                           (float)screenHeight / bgTex.height);
    DrawTexturePro(bgTex, {0, 0, (float)bgTex.width, (float)bgTex.height},
                   {0, 0, bgTex.width * scale, bgTex.height * scale},
                   {0, 0}, 0.0f, Color{130, 110, 60, 255});
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 140});
  }

  // Pulsing golden title
  float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 2.5f);
  unsigned char titleAlpha = (unsigned char)(200 + 55 * pulse);
  TextUtils::drawCenteredOutlined("VICTORIA", screenHeight / 8, 36,
           Color{255, 210, 60, titleAlpha}, screenWidth, 3, Color{0, 0, 0, 255});

  // Decorative gold lines
  int lineY = screenHeight / 8 + 50;
  DrawRectangleGradientH(screenWidth / 2 - 220, lineY, 220, 1,
           Color{0, 0, 0, 0}, Color{220, 180, 60, 220});
  DrawRectangleGradientH(screenWidth / 2, lineY, 220, 1,
           Color{220, 180, 60, 220}, Color{0, 0, 0, 0});

  TextUtils::drawCentered("Has escapado del Circulo VII",
           screenHeight / 4, 12, Color{240, 220, 170, 255}, screenWidth);

  drawRunStats();

  // Pulsing button
  float btnPulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.5f);
  unsigned char alpha = (unsigned char)(180 + 75 * btnPulse);
  TextUtils::drawCenteredOutlined("[ ENTER ] Menu Principal", screenHeight - 70, 12,
           Color{255, 220, 140, alpha}, screenWidth, 2);
}


void Game::drawBossIntro() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 200});

  // Boss portrait — derive from boss name
  std::string bossPortrait = "assets/art/portrait_minotaur.png";
  if (registry.hasComponent<BossPhase>(bossEntity)) {
    auto &bp = registry.getComponent<BossPhase>(bossEntity);
    // Map boss name to portrait file
    if (bp.bossName.find("Minotaur") != std::string::npos ||
        bp.bossName.find("minotaur") != std::string::npos)
      bossPortrait = "assets/art/portrait_minotaur.png";
  }
  Texture2D portrait = ResourceManager::getInstance().getTexture(bossPortrait);
  if (portrait.id > 0) {
    int pH = 300, pW = 300;
    Rectangle src = {0, 0, (float)portrait.width, (float)portrait.height};
    Rectangle dst = {(float)(screenWidth / 2 - pW / 2),
                     (float)(screenHeight / 2 - pH / 2 - 30),
                     (float)pW, (float)pH};
    // Red vignette behind portrait
    DrawRectangleGradientH(dst.x - 80, dst.y - 20, pW + 160, pH + 40,
                           Color{0, 0, 0, 0}, Color{80, 0, 0, 100});
    DrawTexturePro(portrait, src, dst, {0, 0}, 0.0f, WHITE);
    DrawRectangleLinesEx(dst, 3.0f, Color{200, 150, 50, 255});
  }

  if (registry.hasComponent<BossPhase>(bossEntity)) {
    auto &bp = registry.getComponent<BossPhase>(bossEntity);
    const char *name = bp.bossName.c_str();
    TextUtils::drawCenteredOutlined(name, screenHeight / 2 + 145, 28,
             Color{220, 180, 100, 255}, screenWidth, 2);
  }

  float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 2.5f);
  unsigned char alpha = (unsigned char)(120 + 135 * pulse);
  TextUtils::drawCentered("Preparate...", screenHeight / 2 + 190, 14,
           Color{180, 160, 140, alpha}, screenWidth);
}


void Game::drawCharacterSelect() {
  // Background art (reuse title bg dimmed)
  Texture2D bgTex = ResourceManager::getInstance().getTexture(
      "assets/art/title_bg.png");
  if (bgTex.id > 0) {
    float scale = std::max((float)screenWidth / bgTex.width,
                           (float)screenHeight / bgTex.height);
    DrawTexturePro(bgTex, {0, 0, (float)bgTex.width, (float)bgTex.height},
                   {0, 0, bgTex.width * scale, bgTex.height * scale},
                   {0, 0}, 0.0f, Color{120, 120, 120, 255});
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 160});
  } else {
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 5, 15, 255});
  }

  const char *title = "ELIGE TU GUERRERO";
  int titleSize = 28;
  int tw = TextUtils::measure(title, titleSize);
  // Auto-shrink if too wide for screen
  while (tw > screenWidth - 80 && titleSize > 16) {
    titleSize -= 2;
    tw = TextUtils::measure(title, titleSize);
  }
  TextUtils::drawOutlined(title, (screenWidth - tw) / 2, screenHeight / 10, titleSize,
           Color{220, 180, 100, 255}, 2);

  using json = nlohmann::json;
  std::ifstream file("assets/data/characters.json");
  json data;
  if (!file.is_open())
    return;
  file >> data;

  auto &chars = data["characters"];
  std::string portraitPaths[] = {
      "assets/art/portrait_warrior.png",
      "assets/art/portrait_rogue.png",
      "assets/art/portrait_knight.png"};

  int numChars = (int)chars.size();
  int cardW = 280, cardH = 420;
  int gap = 25;
  int totalW = numChars * cardW + (numChars - 1) * gap;
  // If too wide for screen, shrink cards
  if (totalW > screenWidth - 40) {
    cardW = (screenWidth - 40 - (numChars - 1) * gap) / numChars;
    totalW = numChars * cardW + (numChars - 1) * gap;
  }
  int startX = (screenWidth - totalW) / 2;
  int startY = screenHeight / 6;
  int innerPad = 32; // Padding inside card for majestic borders

  for (int i = 0; i < numChars; i++) {
    auto &c = chars[i];
    int cx = startX + i * (cardW + gap);
    int cy = startY;
    int contentW = cardW - innerPad * 2; // Usable text width

    bool selected = (i == selectedCharacter);

    // Card panel with art texture
    Color bg = selected ? Color{35, 30, 50, 240} : Color{15, 15, 25, 220};
    Color border = selected ? Color{220, 180, 100, 255} : Color{60, 55, 70, 255};
    drawUIPanel(cx, cy, cardW, cardH, bg, border);

    // Character portrait (Gemini art)
    if (i < 3) {
      Texture2D portrait = ResourceManager::getInstance().getTexture(portraitPaths[i]);
      if (portrait.id > 0) {
        int portraitH = 180;
        int portraitW = contentW;
        Rectangle src = {0, 0, (float)portrait.width, (float)portrait.height};
        Rectangle dst = {(float)(cx + innerPad), (float)(cy + innerPad),
                         (float)portraitW, (float)portraitH};
        DrawTexturePro(portrait, src, dst, {0, 0}, 0.0f,
                       selected ? WHITE : Color{160, 160, 160, 255});
        DrawRectangleLinesEx(dst, 2.0f, border);
      }
    }

    int textY = cy + innerPad + 180 + 15; // right below portrait with extra margin

    // Name — centered, auto-shrink if too wide
    std::string name = c.value("name", "???");
    int nameSize = 18;
    while (TextUtils::measure(name.c_str(), nameSize) > contentW && nameSize > 10)
      nameSize -= 2;
    int nw = TextUtils::measure(name.c_str(), nameSize);
    TextUtils::draw(name.c_str(), cx + (cardW - nw) / 2 + 1, textY + 1, nameSize,
             Color{0, 0, 0, 180});
    TextUtils::draw(name.c_str(), cx + (cardW - nw) / 2, textY, nameSize,
             selected ? Color{255, 230, 160, 255} : Color{180, 180, 180, 255});
    textY += nameSize + 12;

    // Stats — 2 rows, colored values
    int statSize = 10;
    const char *statLine1 = TextFormat("HP:%d  DMG:%d",
        c.value("hp", 100), c.value("damage", 15));
    const char *statLine2 = TextFormat("SPD:%.0f  STA:%.0f",
        c.value("speed", 250.0f), c.value("stamina", 100.0f));
    // Auto-shrink stats if needed
    int sfs = statSize;
    while (TextUtils::measure(statLine1, sfs) > contentW && sfs > 6) sfs--;
    
    int st1W = TextUtils::measure(statLine1, sfs);
    TextUtils::draw(statLine1, cx + (cardW - st1W) / 2, textY, sfs, Color{180, 180, 180, 255});
    textY += sfs + 3;
    
    int st2W = TextUtils::measure(statLine2, sfs);
    TextUtils::draw(statLine2, cx + (cardW - st2W) / 2, textY, sfs, Color{180, 180, 180, 255});
    textY += sfs + 14;

    // Description — dynamically wrapped
    int descSize = 8;
    std::string desc = c.value("description", "");
    textY = TextUtils::drawWrapped(desc.c_str(), cx + innerPad, textY, descSize,
                                   Color{160, 160, 160, 255}, contentW);

    textY += 6;

    // Special ability — dynamically wrapped
    std::string special = c.value("special", "");
    if (!special.empty()) {
      textY = TextUtils::drawWrapped(special.c_str(), cx + innerPad, textY, descSize,
                                     Color{220, 190, 110, 255}, contentW);
    }

    // Selection glow
    if (selected) {
      float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
      unsigned char a = (unsigned char)(40 * pulse);
      DrawRectangle(cx, cy, cardW, cardH, Color{220, 180, 100, a});
      TextUtils::draw("v", cx + cardW / 2 - 5, cy - 22, 24,
               Color{220, 180, 100, 255});
    }
  }

  const char *hint = "[A/D] Seleccionar  [ENTER] Confirmar  [ESC] Volver";
  int hintSize = 12;
  int hintW = TextUtils::measure(hint, hintSize);
  while (hintW > screenWidth - 40 && hintSize > 8) {
    hintSize -= 2;
    hintW = TextUtils::measure(hint, hintSize);
  }
  TextUtils::draw(hint, (screenWidth - hintW) / 2, screenHeight - 40, hintSize,
           Color{130, 125, 120, 255});
}


void Game::drawAbilitySelect() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 5, 15, 255});

  // Background panel
  int mainW = 420, mainH = 520;
  int mainX = (screenWidth - mainW) / 2, mainY = (screenHeight - mainH) / 2;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{12, 12, 22, 230},
              Color{160, 130, 60, 255});

  TextUtils::drawCenteredOutlined("ELIGE UNA HABILIDAD", mainY + 35, 22,
           Color{220, 180, 100, 255}, screenWidth, 2);

  int cardW = 330, cardH = 100;
  int startY = mainY + 80;

  for (int i = 0; i < (int)abilityChoices.size(); i++) {
    auto &ab = abilityChoices[i];
    int cardX = (screenWidth - cardW) / 2;
    int cardY = startY + i * (cardH + 15);
    int contentW = cardW - 30;

    // Rarity color
    Color rarityCol = Color{180, 180, 180, 255};
    const char *rarityText = "COMUN";
    if (ab.rarity == AbilityRarity::RARE) {
      rarityCol = Color{80, 140, 255, 255};
      rarityText = "RARO";
    } else if (ab.rarity == AbilityRarity::EPIC) {
      rarityCol = Color{200, 80, 255, 255};
      rarityText = "EPICO";
    }

    // Card background + border
    bool sel = (i == selectedAbility);
    DrawRectangle(cardX, cardY, cardW, cardH,
                  sel ? Color{40, 40, 60, 255} : Color{20, 20, 30, 255});
    DrawRectangleLines(cardX, cardY, cardW, cardH,
                       sel ? Color{220, 180, 100, 255} : rarityCol);

    if (sel)
      TextUtils::draw(">", cardX - 12, cardY + cardH / 2 - 8, 16,
               Color{220, 180, 100, 255});

    // Name + rarity tag
    std::string nameStr = TextUtils::truncate(ab.name, 14, contentW - 80);
    TextUtils::draw(nameStr.c_str(), cardX + 15, cardY + 10, 14, rarityCol);
    int rw = TextUtils::measure(rarityText, 10);
    TextUtils::draw(rarityText, cardX + cardW - rw - 15, cardY + 12, 10, rarityCol);

    // Description — wrapped to fit
    TextUtils::drawWrapped(ab.description.c_str(), cardX + 15, cardY + 35, 10, WHITE, contentW - 30);

    // Tags
    int tagX = cardX + 15;
    for (auto &tag : ab.tags) {
      TextUtils::draw(tag.c_str(), tagX, cardY + cardH - 22, 8,
               Color{100, 100, 120, 255});
      tagX += TextUtils::measure(tag.c_str(), 8) + 10;
    }
  }

  // Controls
  TextUtils::drawCentered("[W/S] Seleccionar  [ENTER] Confirmar",
           screenHeight - 45, 12, Color{120, 120, 120, 255}, screenWidth);

  // Active abilities + synergies sidebar
  if (registry.hasComponent<AbilityHolder>(playerEntity)) {
    auto &holder = registry.getComponent<AbilityHolder>(playerEntity);
    TextUtils::draw(TextFormat("Habilidades: %d", (int)holder.abilities.size()), 15,
             15, 10, Color{150, 150, 150, 255});

    auto &synStates = synergySystem.getStates();
    auto &synDefs = synergySystem.getDefs();
    int sy = 32;
    for (int i = 0; i < (int)synStates.size(); i++) {
      Color col = synStates[i].active ? Color{230, 190, 100, 255}
                                      : Color{80, 80, 80, 255};
      TextUtils::draw(synDefs[i].name.c_str(), 15, sy, 10, col);
      sy += 14;
    }
  }
}


void Game::drawRunStats() {
  auto &run = saveManager.getCurrentRun();
  auto &prog = saveManager.getProgress();

  int sy = screenHeight / 2 + 30;

  int statSize = 12;
  int lifeSize = 8;
  TextUtils::drawCentered(TextFormat("Salas: %d/%d", run.roomsCleared, run.totalRooms),
           sy, statSize, Color{200, 200, 200, 255}, screenWidth);
  TextUtils::drawCentered(TextFormat("Enemigos: %d", run.enemiesKilled),
           sy + 20, statSize, Color{200, 200, 200, 255}, screenWidth);
  TextUtils::drawCentered(TextFormat("Tiempo: %.1fs", run.timePlayed),
           sy + 40, statSize, Color{200, 200, 200, 255}, screenWidth);
  TextUtils::drawCentered(TextFormat("Habilidades: %d", run.abilitiesCollected),
           sy + 60, statSize, Color{200, 200, 200, 255}, screenWidth);

  // Lifetime stats
  TextUtils::drawCentered(TextFormat("Runs: %d  Victorias: %d  Muertes: %d", prog.totalRuns,
                      prog.totalVictories, prog.totalDeaths),
           sy + 95, lifeSize, Color{120, 120, 120, 255}, screenWidth);
  TextUtils::drawCentered(TextFormat("Enemigos totales: %d  Mejor sala: %d",
                      prog.totalEnemiesKilled, prog.bestRoom),
           sy + 110, lifeSize, Color{120, 120, 120, 255}, screenWidth);
  if (prog.bestTime > 0.0f)
    TextUtils::drawCentered(TextFormat("Mejor tiempo: %.1fs", prog.bestTime),
             sy + 125, lifeSize, Color{120, 120, 120, 255}, screenWidth);
}


// =============================================================================
// Mini-Boss Health Bar
// =============================================================================
void Game::drawMiniBossHealthBar() {
  auto mbView = registry.view<MiniBoss, Health>();
  for (Entity e : mbView) {
    auto &health = registry.getComponent<Health>(e);
    if (health.isDead())
      continue;
    auto &mb = registry.getComponent<MiniBoss>(e);

    float ratio = std::max(0.0f, (float)health.currentHP / health.maxHP);
    int barW = 420, barH = 22;

    // Portrait on the left side
    int portraitSize = 56;
    std::string portraitPath;
    if (mb.specialType == 0)
      portraitPath = "assets/art/portrait_infernal_knight.png";
    else if (mb.specialType == 1)
      portraitPath = "assets/art/portrait_soul_archer.png";
    else
      portraitPath = "assets/art/portrait_pit_fiend.png";

    Texture2D portrait =
        ResourceManager::getInstance().getTexture(portraitPath);

    int totalW = barW + (portrait.id > 0 ? portraitSize + 8 : 0);
    int startX = (screenWidth - totalW) / 2;
    int barY = screenHeight - 90;

    // Draw portrait if available
    if (portrait.id > 0) {
      int px = startX;
      int py = barY - (portraitSize - barH) / 2;
      // Portrait frame background
      DrawRectangle(px - 2, py - 2, portraitSize + 4, portraitSize + 4,
                    Color{20, 10, 5, 220});
      DrawTexturePro(
          portrait, {0, 0, (float)portrait.width, (float)portrait.height},
          {(float)px, (float)py, (float)portraitSize, (float)portraitSize},
          {0, 0}, 0.0f, WHITE);
      DrawRectangleLinesEx(
          {(float)(px - 2), (float)(py - 2), (float)(portraitSize + 4),
           (float)(portraitSize + 4)},
          2.0f, Color{180, 140, 60, 255});
    }

    int barX = startX + (portrait.id > 0 ? portraitSize + 8 : 0);

    // Dark background padding
    DrawRectangle(barX - 2, barY - 2, barW + 4, barH + 4,
                  Color{20, 10, 5, 220});
    DrawRectangle(barX, barY, barW, barH, Color{40, 5, 0, 255});

    // Gradient fill
    int fill = (int)(barW * ratio);
    DrawRectangleGradientV(barX, barY, fill, barH / 2,
                           Color{255, 140, 40, 255}, Color{220, 90, 20, 255});
    DrawRectangleGradientV(barX, barY + barH / 2, fill, barH / 2,
                           Color{180, 60, 10, 255}, Color{130, 30, 5, 255});

    // Gold border
    DrawRectangleLinesEx(
        {(float)barX, (float)barY, (float)barW, (float)barH}, 2.0f,
        Color{200, 170, 100, 255});

    // Name with drop shadow
    const char *name = mb.name.c_str();
    int nameW = TextUtils::measure(name, 12);
    TextUtils::draw(name, barX + (barW - nameW) / 2 + 1, barY - 20, 12,
             Color{0, 0, 0, 180});
    TextUtils::draw(name, barX + (barW - nameW) / 2, barY - 21, 12,
             Color{220, 180, 100, 255});
    break; // Only show one
  }
}


// =============================================================================
// Inventory Window (I key)
// =============================================================================
void Game::drawInventory() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 200});

  // Main panel background
  int mainW = 540, mainH = 530;
  int mainX = (screenWidth - mainW) / 2, mainY = 25;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{15, 15, 25, 230},
              Color{180, 140, 60, 255});

  const char *title = "INVENTARIO";
  TextUtils::drawCenteredShadow(title, 38, 24, Color{220, 180, 100, 255}, screenWidth);

  if (!registry.hasComponent<ItemHolder>(playerEntity))
    return;
  auto &ih = registry.getComponent<ItemHolder>(playerEntity);

  int panelW = 500, slotH = 55;
  int px = (screenWidth - panelW) / 2;
  int py = 100;

  for (int i = 0; i < ih.maxItems; i++) {
    int sy = py + i * (slotH + 5);
    bool sel = (i == inventorySelectedSlot);
    Color bg = sel ? Color{40, 35, 55, 240} : Color{18, 18, 28, 220};
    Color border = sel ? Color{220, 180, 100, 255} : Color{50, 50, 65, 255};

    drawUIPanel(px, sy, panelW, slotH, bg, border);

    if (i < (int)ih.equippedItems.size()) {
      auto &item = ih.equippedItems[i];

      // Rarity color
      Color rarCol = {180, 180, 180, 255};
      const char *rarText = "Comun";
      if (item.rarity == ItemRarity::UNCOMMON) {
        rarCol = {80, 200, 80, 255};
        rarText = "Poco Comun";
      } else if (item.rarity == ItemRarity::RARE) {
        rarCol = {80, 140, 255, 255};
        rarText = "Raro";
      } else if (item.rarity == ItemRarity::EPIC) {
        rarCol = {200, 80, 255, 255};
        rarText = "Epico";
      } else if (item.rarity == ItemRarity::LEGENDARY) {
        rarCol = {255, 180, 0, 255};
        rarText = "Legendario";
      }

      std::string truncName = TextUtils::truncate(item.name, 12, panelW - 100);
      TextUtils::draw(truncName.c_str(), px + 10, sy + 5, 12, rarCol);
      int rw = TextUtils::measure(rarText, 8);
      TextUtils::draw(rarText, px + panelW - rw - 10, sy + 7, 8, rarCol);
      std::string truncDesc = TextUtils::truncate(item.description, 10, panelW - 20);
      TextUtils::draw(truncDesc.c_str(), px + 10, sy + 24, 10,
               Color{150, 150, 150, 255});
    } else {
      TextUtils::draw("[ Vacio ]", px + 10, sy + 18, 10, Color{60, 60, 60, 255});
    }
  }

  TextUtils::drawCentered("[W/S] Navegar  [X] Descartar  [ESC] Cerrar",
           screenHeight - 45, 8, Color{120, 120, 120, 255}, screenWidth);
}


// =============================================================================
// Info Menu (TAB) — single screen with controls, run progress, abilities,
// active synergies, equipped items, and core stats. The HUD is intentionally
// minimal so the player comes here when they want context.
// =============================================================================
void Game::drawInfoMenu() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 210});

  // Two-column layout inside one panel.
  int mainW = std::min(screenWidth - 60, 980);
  int mainH = std::min(screenHeight - 60, 640);
  int mainX = (screenWidth - mainW) / 2;
  int mainY = (screenHeight - mainH) / 2;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{15, 15, 25, 235},
              Color{180, 140, 60, 255});

  TextUtils::drawCenteredShadow("INFORMACION", mainY + 35, 22,
                                Color{220, 180, 100, 255}, screenWidth);

  int colW = (mainW - 70) / 2;
  int leftX = mainX + 35;
  int topY = mainY + 65;
  int bottomLimit = mainY + mainH - 35;

  auto sectionHeader = [&](int x, int y, const char *label) {
    TextUtils::draw(label, x, y, 12, Color{220, 180, 100, 255});
    DrawLine(x, y + 18, x + colW, y + 18, Color{120, 90, 40, 200});
  };

  // ---------- LEFT COLUMN: Controls + Run + Stats ----------
  if (infoMenuTab == 0) {
    int ly = topY;

    sectionHeader(leftX, ly, "CONTROLES");
    ly += 24;
    struct Bind { const char *key; const char *desc; };
    Bind binds[] = {
      {"WASD",   "Moverse"},
      {"J",      "Ataque ligero (combo x3)"},
      {"K",      "Ataque pesado / Finisher"},
      {"F",      "Parry (anula dano)"},
      {"L",      "Especial de clase"},
      {"SPACE",  "Esquivar (i-frames)"},
      {"E",      "Interactuar"},
      {"I",      "Inventario (descartar)"},
      {"TAB",    "Estadisticas"},
      {"H",      "Habilidades"},
      {"ESC/P",  "Pausa"},
    };
    for (auto &b : binds) {
      if (ly + 14 > bottomLimit) break;
      TextUtils::draw(b.key, leftX, ly, 10, Color{180, 220, 100, 255});
      TextUtils::draw(b.desc, leftX + 90, ly, 10, Color{200, 200, 200, 255});
      ly += 14;
    }

  ly += 10;
  sectionHeader(leftX, ly, "RUN");
  ly += 24;
  TextUtils::draw(TextFormat("Sala: %d / %d", currentRoom + 1, totalRooms + 1),
                  leftX, ly, 10, Color{220, 220, 220, 255});
  ly += 14;
  TextUtils::draw(TextFormat("Tiempo: %.0fs",
                             saveManager.getCurrentRun().timePlayed),
                  leftX, ly, 10, Color{220, 220, 220, 255});
  ly += 14;
  if (registry.hasComponent<PlayerStats>(playerEntity)) {
    auto &ps = registry.getComponent<PlayerStats>(playerEntity);
    TextUtils::draw(TextFormat("Clase: %s", ps.classId.c_str()), leftX, ly, 10,
                    Color{220, 180, 100, 255});
    ly += 18;

    sectionHeader(leftX, ly, "STATS");
    ly += 24;
    auto stat = [&](const char *label, const char *value, Color col) {
      if (ly + 14 > bottomLimit) return;
      TextUtils::draw(label, leftX, ly, 10, Color{170, 170, 170, 255});
      TextUtils::draw(value, leftX + 130, ly, 10, col);
      ly += 14;
    };
    stat("HP Max", TextFormat("%d", ps.finalMaxHP), Color{220, 80, 80, 255});
    stat("Dano", TextFormat("%d", ps.finalDamage), Color{220, 170, 60, 255});
    stat("Velocidad", TextFormat("%.0f", ps.finalSpeed), Color{220, 200, 140, 255});
    stat("Stamina", TextFormat("%.0f", ps.finalMaxStamina),
         Color{200, 160, 90, 255});
    stat("Crit", TextFormat("%.0f%%", ps.finalCritChance * 100.0f),
         Color{255, 200, 60, 255});
    if (ps.finalLifesteal > 0.0f)
      stat("Robo Vida", TextFormat("%.0f%%", ps.finalLifesteal * 100.0f),
           Color{220, 130, 80, 255});
    if (ps.finalThorns > 0.0f)
      stat("Espinas", TextFormat("%.0f%%", ps.finalThorns * 100.0f),
           Color{200, 120, 80, 255});
  }

  } // Cerramos if(infoMenuTab == 0)

  if (infoMenuTab == 1) {
    int rx = mainX + 35; 
    int ry = topY;
    int fullW = mainW - 70;

    auto fullHeader = [&](int x, int y, const char *label) {
      TextUtils::draw(label, x, y, 12, Color{220, 180, 100, 255});
      DrawLine(x, y + 18, x + fullW, y + 18, Color{120, 90, 40, 200});
    };

    fullHeader(rx, ry, "HABILIDADES ESPECIALES");
    ry += 24;
    if (registry.hasComponent<AbilityHolder>(playerEntity)) {
      auto &ah = registry.getComponent<AbilityHolder>(playerEntity);
      if (ah.abilities.empty()) {
        TextUtils::draw("Sin habilidades activas.", rx, ry, 10, Color{150, 150, 150, 255});
        ry += 30;
      } else {
        int halfW = fullW / 2;
        int col = 0;
        int startRy = ry;
        for (auto &ab : ah.abilities) {
          if (ry + 20 > bottomLimit - 100 && col == 0) {
            col = 1; ry = startRy; // salta a segunda columna
          }
          if (ry + 20 > bottomLimit - 100 && col == 1) break;
          
          Color nameCol = (ab.rarity == AbilityRarity::EPIC) ? Color{200, 100, 255, 255} : (ab.rarity == AbilityRarity::RARE) ? Color{100, 180, 255, 255} : Color{220, 220, 220, 255};
          TextUtils::draw(ab.name.c_str(), rx + col * halfW, ry, 11, nameCol);
          TextUtils::draw(ab.description.c_str(), rx + 160 + col * halfW, ry, 10, Color{180, 180, 180, 255});
          ry += 20;
        }
        ry = (col == 1 ? ry : ry + 20); 
        // Force Ry to bottom grid if abilities aren't many
        if (ry < bottomLimit - 120) ry = bottomLimit - 120;
      }
    }

    ry += 10;
    fullHeader(rx, ry, "SINERGIAS ACTIVAS");
    ry += 24;
    {
      auto &states = synergySystem.getStates();
      auto &defs = synergySystem.getDefs();
      for (int i = 0; i < (int)states.size(); i++) {
        if (ry + 14 > bottomLimit) break;
        bool act = states[i].active;
        if (!act) continue; // Only show active synergies!
        Color col = Color{230, 190, 100, 255}; // active synergy: amber/gold
        DrawRectangle(rx, ry + 2, 6, 8, col);
        std::string n = TextUtils::truncate(defs[i].name, 10, fullW - 14);
        TextUtils::draw(n.c_str(), rx + 15, ry, 11, col);
        ry += 16;
      }
      if (ry == bottomLimit - 96 || states.empty()) {
        TextUtils::draw("(Ninguna sinergia activa)", rx, ry, 10, Color{100, 100, 100, 255});
      }
    }
  }

  // Common footer navigation for all tabs
  TextUtils::drawCentered("[A/D] Cambiar Pestana   [TAB/ESC] Cerrar", mainY + mainH - 22, 10,
                          Color{160, 160, 160, 255}, screenWidth);
}


// =============================================================================
// Item Swap Window (when inventory is full)
// =============================================================================
void Game::drawItemSwap() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 200});
  int mainW = 500, mainH = 500;
  int mainX = (screenWidth - mainW) / 2, mainY = 20;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{15, 15, 25, 230},
              Color{200, 160, 60, 255});

  TextUtils::drawCenteredOutlined("INTERCAMBIAR ITEM", mainY + 35, 18, Color{255, 200, 80, 255}, screenWidth, 2);

  // Show new item
  int newBoxW = 420, newBoxH = 60;
  int nbx = (screenWidth - newBoxW) / 2;
  int nby = mainY + 70;
  DrawRectangle(nbx, nby, newBoxW, newBoxH, Color{30, 40, 20, 255});
  DrawRectangleLines(nbx, nby, newBoxW, newBoxH, Color{180, 220, 80, 255});
  TextUtils::draw("Nuevo:", nbx + 10, nby + 5, 10, Color{180, 220, 80, 255});
  std::string truncNewName = TextUtils::truncate(pendingItem.name, 12, newBoxW - 20);
  TextUtils::draw(truncNewName.c_str(), nbx + 10, nby + 20, 12,
           Color{255, 220, 100, 255});
  std::string truncNewDesc = TextUtils::truncate(pendingItem.description, 8, newBoxW - 20);
  TextUtils::draw(truncNewDesc.c_str(), nbx + 10, nby + 40, 8,
           Color{150, 150, 150, 255});

  // Show current inventory to choose which to replace
  if (!registry.hasComponent<ItemHolder>(playerEntity))
    return;
  auto &ih = registry.getComponent<ItemHolder>(playerEntity);

  int panelW = 420, slotH = 45;
  int px = (screenWidth - panelW) / 2;
  int py = mainY + 140;

  for (int i = 0; i < (int)ih.equippedItems.size(); i++) {
    int sy = py + i * (slotH + 4);
    bool sel = (i == inventorySelectedSlot);
    Color bg = sel ? Color{50, 30, 30, 255} : Color{20, 20, 30, 255};
    Color border = sel ? Color{255, 100, 100, 255} : Color{60, 60, 80, 255};

    DrawRectangle(px, sy, panelW, slotH, bg);
    DrawRectangleLines(px, sy, panelW, slotH, border);

    auto &item = ih.equippedItems[i];
    std::string truncSlotName = TextUtils::truncate(item.name, 12, panelW - 20);
    TextUtils::draw(truncSlotName.c_str(), px + 10, sy + 5, 12, WHITE);
    std::string truncSlotDesc = TextUtils::truncate(item.description, 8, panelW - 20);
    TextUtils::draw(truncSlotDesc.c_str(), px + 10, sy + 22, 8,
             Color{130, 130, 130, 255});

    if (sel) TextUtils::draw(">", px - 14, sy + 10, 14, Color{255, 100, 100, 255});
  }

  TextUtils::drawCentered("[W/S] Elegir [ENTER] Reemplazar [ESC] Descartar",
           screenHeight - 40, 8, Color{120, 120, 120, 255}, screenWidth);
}


// =============================================================================
// Debug Overlay
// =============================================================================
void Game::drawDebugOverlay() {
#ifndef NDEBUG
  DrawFPS(10, 10);
  int entityCount = 0;
  auto all = registry.view<Transform2D>();
  entityCount = (int)all.size();

  char buf[256];
  snprintf(buf, sizeof(buf), "Entities: %d  Room: %d/%d  Enemies: %d",
           entityCount, currentRoom + 1, totalRooms, enemiesAlive);
  TextUtils::draw(buf, 10, 35, 16, Color{180, 180, 180, 200});

  if (playerEntity != NULL_ENTITY && registry.hasComponent<Health>(playerEntity)) {
    auto &h = registry.getComponent<Health>(playerEntity);
    auto &s = registry.getComponent<Stamina>(playerEntity);
    snprintf(buf, sizeof(buf), "HP: %d/%d  Stam: %.0f/%.0f",
             h.currentHP, h.maxHP, s.currentStamina, s.maxStamina);
    TextUtils::draw(buf, 10, 55, 16, Color{180, 180, 180, 200});

    // Physics debug
    if (registry.hasComponent<PhysicsBody>(playerEntity)) {
      auto &pb = registry.getComponent<PhysicsBody>(playerEntity);
      auto &vel = registry.getComponent<Velocity>(playerEntity);
      float speed = sqrtf(vel.vx * vel.vx + vel.vy * vel.vy);
      snprintf(buf, sizeof(buf), "Mass:%.1f Accel:%.0f Fric:%.0f",
               pb.mass, pb.acceleration, pb.friction);
      TextUtils::draw(buf, 10, 75, 16, Color{180, 180, 180, 200});
      snprintf(buf, sizeof(buf), "Speed: %.0f  Imp: %.0f", speed, pb.maxSpeed);
      TextUtils::draw(buf, 10, 95, 16, Color{180, 180, 180, 200});
    }

    // Combat debug
    if (registry.hasComponent<Combat>(playerEntity) &&
        registry.hasComponent<AbilityHolder>(playerEntity)) {
      auto &c = registry.getComponent<Combat>(playerEntity);
      auto &ah = registry.getComponent<AbilityHolder>(playerEntity);
      snprintf(buf, sizeof(buf), "Stun:%.2f AtkMult:%.1f Dash:%s",
               c.stateTimer, ah.windupMultiplier,
               (h.isInvulnerable() ? "yes" : "no"));
      TextUtils::draw(buf, 10, 115, 16, Color{180, 180, 180, 200});
    }
  }

  // Boss phase indicator
  if (bossEntity != NULL_ENTITY && registry.isAlive(bossEntity) &&
      registry.hasComponent<BossPhase>(bossEntity)) {
    auto &bp = registry.getComponent<BossPhase>(bossEntity);
    snprintf(buf, sizeof(buf), "Phase %d/%d", bp.currentPhase + 1,
             (int)bp.phases.size());
    TextUtils::draw(buf, 10, 135, 16, Color{255, 180, 80, 200});
  }

  TextUtils::draw("Blue=Vel Green=Target Red=Impulse", 10, screenHeight - 70, 12,
           Color{100, 100, 100, 150});
#endif
}


// =============================================================================
// Abilities View
// =============================================================================
void Game::drawAbilitiesView() {
  if (playerEntity == NULL_ENTITY || !registry.hasComponent<AbilityHolder>(playerEntity))
    return;

  auto &holder = registry.getComponent<AbilityHolder>(playerEntity);
  int y = 100;
  TextUtils::draw("HABILIDADES ACTIVAS", 20, y, 14, Color{220, 180, 100, 255});
  y += 22;

  for (auto &ab : holder.abilities) {
    Color rarityColor = WHITE;
    if (ab.rarity == AbilityRarity::RARE) rarityColor = Color{100, 180, 255, 255};
    else if (ab.rarity == AbilityRarity::EPIC) rarityColor = Color{180, 100, 255, 255};
    std::string truncAb = TextUtils::truncate(ab.name, 10, 300);
    TextUtils::draw(truncAb.c_str(), 30, y, 10, rarityColor);
    y += 16;
  }
}


// =============================================================================
// Map Select — DAG graph with room nodes
// =============================================================================
void Game::drawMapSelect() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 5, 15, 255});

  int mainW = 500, mainH = 520;
  int mainX = (screenWidth - mainW) / 2, mainY = 20;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{12, 12, 22, 230},
              Color{160, 130, 60, 255});

  TextUtils::drawCenteredOutlined("MAPA DE LA MAZMORRA", mainY + 35, 20,
           Color{220, 180, 100, 255}, screenWidth, 2);

  // Draw layers top-to-bottom
  int layerCount = runMap.totalLayers();
  int graphY = mainY + 70;
  int graphH = mainH - 130;
  int layerSpacing = (layerCount > 1) ? graphH / (layerCount - 1) : 0;

  // First pass: compute positions for connection lines
  struct NodePos { float x, y; };
  std::vector<std::vector<NodePos>> positions(layerCount);
  for (int l = 0; l < layerCount; l++) {
    int count = (int)runMap.layers[l].size();
    int totalW = count * 60 + (count - 1) * 40;
    int startX = (screenWidth - totalW) / 2;
    for (int n = 0; n < count; n++) {
      float nx = (float)(startX + n * 100 + 30);
      float ny = (float)(graphY + l * layerSpacing);
      positions[l].push_back({nx, ny});
    }
  }

  // Draw connection lines
  for (int l = 0; l < layerCount - 1; l++) {
    for (int n = 0; n < (int)runMap.layers[l].size(); n++) {
      auto &node = runMap.layers[l][n];
      for (int conn : node.connections) {
        if (conn < (int)positions[l + 1].size()) {
          Color lineCol = (node.visited || node.current)
                              ? Color{120, 120, 120, 200}
                              : Color{50, 50, 50, 120};
          DrawLineEx({positions[l][n].x, positions[l][n].y + 15},
                     {positions[l + 1][conn].x, positions[l + 1][conn].y - 15},
                     2.0f, lineCol);
        }
      }
    }
  }

  // Draw nodes
  auto choices = runMap.nextChoices();
  for (int l = 0; l < layerCount; l++) {
    for (int n = 0; n < (int)runMap.layers[l].size(); n++) {
      auto &node = runMap.layers[l][n];
      float nx = positions[l][n].x;
      float ny = positions[l][n].y;
      int radius = 14;

      Color col = node.color();
      if (node.visited) col = Color{(unsigned char)(col.r / 3), (unsigned char)(col.g / 3), (unsigned char)(col.b / 3), 200};

      bool isCursor = false;
      if (l == runMap.currentLayer + 1) {
        // This is the next layer — check if this node is under cursor
        for (int ci = 0; ci < (int)choices.size(); ci++) {
          if (choices[ci] == n && ci == runMap.cursorIndex)
            isCursor = true;
        }
      }

      if (node.current) {
        // Current node: glowing ring
        DrawCircle((int)nx, (int)ny, radius + 4, Color{255, 255, 255, 60});
        DrawCircle((int)nx, (int)ny, radius, col);
        DrawCircleLines((int)nx, (int)ny, radius, WHITE);
      } else if (isCursor) {
        // Selectable node under cursor: bright + pulsing
        float pulse = 0.7f + 0.3f * sinf((float)GetTime() * 5.0f);
        Color bright = {(unsigned char)(col.r * pulse), (unsigned char)(col.g * pulse),
                        (unsigned char)(col.b * pulse), 255};
        DrawCircle((int)nx, (int)ny, radius + 2, Color{220, 180, 100, 100});
        DrawCircle((int)nx, (int)ny, radius, bright);
        DrawCircleLines((int)nx, (int)ny, radius, Color{220, 180, 100, 255});
        // Label below
        const char *lbl = node.label();
        int tw = TextUtils::measure(lbl, 10);
        TextUtils::draw(lbl, (int)nx - tw / 2, (int)ny + radius + 6, 10,
                 Color{220, 180, 100, 255});
      } else {
        DrawCircle((int)nx, (int)ny, radius, col);
        if (!node.visited) {
          const char *lbl = node.label();
          int tw = TextUtils::measure(lbl, 10);
          TextUtils::draw(lbl, (int)nx - tw / 2, (int)ny + radius + 6, 10,
                   Color{150, 150, 150, 180});
        }
      }
    }
  }

  // Legend
  int legendY = mainY + mainH - 45;
  const char *legend[] = {"Combate", "Elite", "Tienda", "Descanso", "Jefe"};
  Color legendCol[] = {
      Color{200, 60, 60, 255}, Color{220, 180, 40, 255}, Color{60, 180, 60, 255},
      Color{60, 120, 220, 255}, Color{200, 40, 200, 255}};
  int lx = mainX + 40;
  for (int i = 0; i < 5; i++) {
    DrawCircle(lx, legendY + 5, 5, legendCol[i]);
    TextUtils::draw(legend[i], lx + 10, legendY - 2, 9, Color{150, 150, 150, 200});
    lx += TextUtils::measure(legend[i], 9) + 25;
  }

  // Controls
  TextUtils::drawCentered("[A/D] Elegir  [ENTER] Entrar",
           screenHeight - 30, 12, Color{120, 120, 120, 255}, screenWidth);

  // Layer indicator
  TextUtils::drawCentered(TextFormat("Capa %d / %d", runMap.currentLayer + 1, layerCount),
           mainY + 55, 10, Color{100, 100, 100, 200}, screenWidth);
}


// =============================================================================
// Shop — spend HP for abilities
// =============================================================================
void Game::drawShop() {
  // Semi-transparent overlay
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 5, 15, 180});

  int mainW = 420, mainH = 440;
  int mainX = (screenWidth - mainW) / 2, mainY = 40;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{22, 12, 12, 230},
              Color{180, 80, 40, 255});

  TextUtils::drawCenteredOutlined("TIENDA INFERNAL", mainY + 35, 20,
           Color{220, 120, 40, 255}, screenWidth, 2);

  // HP cost info
  int cost = 0;
  int currentHP = 0;
  if (registry.hasComponent<Health>(playerEntity)) {
    auto &hp = registry.getComponent<Health>(playerEntity);
    cost = hp.maxHP / 10;
    currentHP = hp.currentHP;
  }
  TextUtils::drawCentered(TextFormat("Coste: %d HP  |  Tu HP: %d", cost, currentHP),
           mainY + 62, 11, Color{200, 100, 100, 255}, screenWidth);

  int cardW = 360, cardH = 80;
  int startY = mainY + 85;
  int itemCount = (int)shopItems.size();

  for (int i = 0; i < itemCount; i++) {
    auto &ab = shopItems[i];
    int cardX = (screenWidth - cardW) / 2;
    int cardY = startY + i * (cardH + 10);

    Color rarityCol = Color{180, 180, 180, 255};
    if (ab.rarity == AbilityRarity::RARE)
      rarityCol = Color{80, 140, 255, 255};
    else if (ab.rarity == AbilityRarity::EPIC)
      rarityCol = Color{200, 80, 255, 255};

    bool sel = (i == selectedAbility);
    DrawRectangle(cardX, cardY, cardW, cardH,
                  sel ? Color{50, 30, 20, 255} : Color{25, 15, 12, 255});
    DrawRectangleLines(cardX, cardY, cardW, cardH,
                       sel ? Color{220, 120, 40, 255} : rarityCol);

    if (sel)
      TextUtils::draw(">", cardX - 12, cardY + cardH / 2 - 8, 16,
               Color{220, 120, 40, 255});

    std::string nameStr = TextUtils::truncate(ab.name, 14, cardW - 80);
    TextUtils::draw(nameStr.c_str(), cardX + 15, cardY + 10, 13, rarityCol);
    TextUtils::drawWrapped(ab.description.c_str(), cardX + 15, cardY + 30, 10, WHITE,
                    cardW - 40);
  }

  TextUtils::drawCentered("[W/S] Elegir  [ENTER] Comprar  [ESC] Salir",
           screenHeight - 40, 12, Color{120, 120, 120, 255}, screenWidth);
}


// =============================================================================
// Rest — heal 30% max HP
// =============================================================================
void Game::drawRest() {
  // Semi-transparent overlay
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 5, 15, 180});

  int mainW = 360, mainH = 240;
  int mainX = (screenWidth - mainW) / 2, mainY = (screenHeight - mainH) / 2 - 30;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{20, 12, 12, 230},
              Color{180, 130, 60, 255});

  TextUtils::drawCenteredOutlined("SALA DE DESCANSO", mainY + 35, 20,
           Color{220, 180, 100, 255}, screenWidth, 2);

  // Current HP display
  if (registry.hasComponent<Health>(playerEntity)) {
    auto &hp = registry.getComponent<Health>(playerEntity);
    int heal = hp.maxHP * 3 / 10;
    int newHP = std::min(hp.currentHP + heal, hp.maxHP);

    TextUtils::drawCentered(TextFormat("HP actual: %d / %d", hp.currentHP, hp.maxHP),
             mainY + 80, 14, Color{200, 200, 200, 255}, screenWidth);

    // Show heal preview
    TextUtils::drawCentered(TextFormat("Curacion: +%d HP", heal),
             mainY + 105, 14, Color{230, 180, 90, 255}, screenWidth);
    TextUtils::drawCentered(TextFormat("HP despues: %d / %d", newHP, hp.maxHP),
             mainY + 130, 12, Color{200, 170, 110, 200}, screenWidth);
  }

  // Pulsing prompt
  float alpha = 150 + 80 * sinf((float)GetTime() * 3.0f);
  TextUtils::drawCentered("[ENTER] Descansar",
           mainY + mainH - 65, 16,
           Color{220, 180, 100, (unsigned char)alpha}, screenWidth);
  TextUtils::drawCentered("[ESC] Continuar sin descansar",
           mainY + mainH - 40, 11, Color{120, 110, 100, 200}, screenWidth);
}


// =============================================================================
// Minimap — small overlay during PLAYING
// =============================================================================
void Game::drawMinimap() {
  // Small minimap in top-right corner showing run progress
  int mapW = 120, mapH = 30;
  int mapX = screenWidth - mapW - 10, mapY = 10;

  DrawRectangle(mapX - 2, mapY - 2, mapW + 4, mapH + 4, Color{0, 0, 0, 150});

  int layerCount = runMap.totalLayers();
  if (layerCount <= 0) return;

  int nodeSpacing = mapW / layerCount;
  for (int l = 0; l < layerCount; l++) {
    float cx = (float)(mapX + l * nodeSpacing + nodeSpacing / 2);
    float cy = (float)(mapY + mapH / 2);
    int r = 4;

    if (l < runMap.currentLayer) {
      // Completed layer
      DrawCircle((int)cx, (int)cy, r, Color{80, 80, 80, 200});
    } else if (l == runMap.currentLayer) {
      // Current layer — bright
      Color col = runMap.current().color();
      DrawCircle((int)cx, (int)cy, r + 1, col);
      DrawCircleLines((int)cx, (int)cy, r + 1, WHITE);
    } else {
      // Future layer
      DrawCircle((int)cx, (int)cy, r, Color{40, 40, 40, 150});
    }

    // Connection line to next
    if (l < layerCount - 1) {
      float nx = (float)(mapX + (l + 1) * nodeSpacing + nodeSpacing / 2);
      DrawLineEx({cx + r + 1, cy}, {nx - r - 1, cy}, 1.0f, Color{50, 50, 50, 150});
    }
  }
}
