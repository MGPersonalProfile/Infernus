#pragma once
#include "../components/Ability.h"
#include "../components/DamageNumber.h"
#include "../components/Transform.h"
#include "../core/ECS.h"
#include "../systems/SynergySystem.h"
#include "raylib.h"

// Centralized UI drawing utilities
class UIRenderer {
public:
  // Draw a styled bar (HP, stamina, etc.)
  static void drawBar(int x, int y, int w, int h, float ratio, Color bgColor,
                      Color fillColor, Color borderColor) {
    DrawRectangle(x, y, w, h, bgColor);
    DrawRectangle(x, y, (int)(w * ratio), h, fillColor);
    DrawRectangleLines(x, y, w, h, borderColor);
  }

  // Draw centered text
  static void drawCenteredText(const char *text, int y, int fontSize,
                               Color color, int screenWidth) {
    int tw = MeasureText(text, fontSize);
    DrawText(text, (screenWidth - tw) / 2, y, fontSize, color);
  }

  // Draw a "card" panel
  static void drawPanel(int x, int y, int w, int h, Color bg, Color border) {
    DrawRectangle(x, y, w, h, bg);
    DrawRectangleLines(x, y, w, h, border);
  }

  // Update and draw floating damage numbers
  static void updateDamageNumbers(Registry &registry, float deltaTime) {
    auto view = registry.view<DamageNumber, Transform2D>();
    for (Entity e : view) {
      auto &dn = registry.getComponent<DamageNumber>(e);
      auto &t = registry.getComponent<Transform2D>(e);

      dn.lifetime -= deltaTime;
      t.y += dn.riseSpeed * deltaTime;

      if (dn.lifetime <= 0.0f)
        registry.markForDestruction(e);
    }
  }

  // Render damage numbers (call inside BeginMode2D)
  static void renderDamageNumbers(Registry &registry) {
    auto view = registry.view<DamageNumber, Transform2D>();
    for (Entity e : view) {
      auto &dn = registry.getComponent<DamageNumber>(e);
      auto &t = registry.getComponent<Transform2D>(e);

      unsigned char a = (unsigned char)(dn.alpha() * 255.0f);
      Color col = {dn.color.r, dn.color.g, dn.color.b, a};
      int fontSize = 20;
      DrawText(dn.text.c_str(), (int)t.x, (int)t.y, fontSize, col);
    }
  }

  // Spawn a damage number entity
  static void spawnDamageNumber(Registry &registry, float x, float y,
                                int amount, bool isCrit) {
    Entity e = registry.createEntity();
    registry.addComponent<Transform2D>(e, x, y - 20.0f);

    Color col = isCrit ? Color{255, 220, 50, 255} : Color{255, 80, 80, 255};
    std::string text =
        isCrit ? TextFormat("%d!", amount) : TextFormat("%d", amount);

    registry.addComponent<DamageNumber>(e, text, col);
  }

  // Draw ability icons in the HUD
  static void drawAbilityIcons(const AbilityHolder &holder, int startX,
                               int startY) {
    int iconSize = 24;
    int padding = 4;

    for (int i = 0; i < (int)holder.abilities.size(); i++) {
      auto &ab = holder.abilities[i];
      int x = startX + i * (iconSize + padding);
      int y = startY;

      Color col = {180, 180, 180, 255};
      if (ab.rarity == AbilityRarity::RARE)
        col = {80, 140, 255, 255};
      else if (ab.rarity == AbilityRarity::EPIC)
        col = {200, 80, 255, 255};

      DrawRectangle(x, y, iconSize, iconSize, Color{30, 30, 40, 255});
      DrawRectangleLines(x, y, iconSize, iconSize, col);

      // First letter of ability name
      char letter[2] = {ab.name[0], '\0'};
      DrawText(letter, x + 6, y + 4, 16, col);
    }
  }

  // Draw synergy indicators
  static void drawSynergyBar(const SynergySystem &synergySystem, int x,
                              int y) {
    auto &states = synergySystem.getStates();
    auto &defs = synergySystem.getDefs();

    for (int i = 0; i < (int)states.size(); i++) {
      Color col = states[i].active ? Color{100, 255, 100, 200}
                                   : Color{60, 60, 60, 100};
      DrawRectangle(x, y + i * 16, 8, 12, col);
      DrawText(defs[i].name.c_str(), x + 12, y + i * 16 - 1, 12,
               states[i].active ? Color{200, 255, 200, 255}
                                : Color{80, 80, 80, 150});
    }
  }
};
