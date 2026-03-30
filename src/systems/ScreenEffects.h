#pragma once
#include "raylib.h"

// Manages screen-wide visual effects: hitstop, flash, vignette, transitions
class ScreenEffects {
public:
  // Freeze time for a short duration (hitstop)
  void addHitstop(float duration) {
    hitstopTimer = duration;
  }

  // Flash the screen a color (e.g., white on crit, red on hit)
  void addFlash(Color color, float duration) {
    flashColor = color;
    flashTimer = duration;
    flashMaxTimer = duration;
  }

  // Fade in/out transition
  void startFadeIn(float duration) {
    fadeTimer = duration;
    fadeMaxTimer = duration;
    fadeDirection = 1; // fading in (black to clear)
  }

  void startFadeOut(float duration) {
    fadeTimer = duration;
    fadeMaxTimer = duration;
    fadeDirection = -1; // fading out (clear to black)
  }

  // Update all effects. Returns true if in hitstop (caller should skip
  // gameplay)
  bool update(float deltaTime) {
    if (hitstopTimer > 0.0f) {
      hitstopTimer -= deltaTime;
      return true; // In hitstop — skip gameplay
    }

    if (flashTimer > 0.0f)
      flashTimer -= deltaTime;

    if (fadeTimer > 0.0f)
      fadeTimer -= deltaTime;

    return false;
  }

  // Render effects on top of everything (call after EndMode2D, before
  // EndDrawing)
  void render(int screenWidth, int screenHeight) {
    // Screen flash
    if (flashTimer > 0.0f) {
      float alpha = flashTimer / flashMaxTimer;
      DrawRectangle(0, 0, screenWidth, screenHeight,
                    Color{flashColor.r, flashColor.g, flashColor.b,
                          (unsigned char)(flashColor.a * alpha)});
    }

    // Vignette effect (always-on, subtle)
    drawVignette(screenWidth, screenHeight);

    // Fade transition
    if (fadeTimer > 0.0f) {
      float t = fadeTimer / fadeMaxTimer;
      unsigned char a;
      if (fadeDirection == 1)
        a = (unsigned char)(255.0f * t); // fade in: black -> clear
      else
        a = (unsigned char)(255.0f * (1.0f - t)); // fade out: clear -> black

      DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, a});
    }
  }

  bool isInHitstop() const { return hitstopTimer > 0.0f; }

private:
  float hitstopTimer = 0.0f;

  Color flashColor = WHITE;
  float flashTimer = 0.0f;
  float flashMaxTimer = 0.1f;

  float fadeTimer = 0.0f;
  float fadeMaxTimer = 1.0f;
  int fadeDirection = 0; // 1 = fade in, -1 = fade out

  void drawVignette(int w, int h) {
    // Simple vignette: dark borders
    int border = 80;
    // Top
    DrawRectangleGradientV(0, 0, w, border, Color{0, 0, 0, 120},
                           Color{0, 0, 0, 0});
    // Bottom
    DrawRectangleGradientV(0, h - border, w, border, Color{0, 0, 0, 0},
                           Color{0, 0, 0, 120});
    // Left
    DrawRectangleGradientH(0, 0, border, h, Color{0, 0, 0, 100},
                           Color{0, 0, 0, 0});
    // Right
    DrawRectangleGradientH(w - border, 0, border, h, Color{0, 0, 0, 0},
                           Color{0, 0, 0, 100});
  }
};
