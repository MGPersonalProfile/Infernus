#include "Game.h"
#include "../debug/DebugPanel.h"
#include "../debug/Profiler.h"
#include "../debug/Telemetry.h"
#include "../scripting/LuaEngine.h"
#include "../systems/AnimEventDispatcher.h"
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

// Pixel font is used via TextUtils::draw / TextUtils::measure throughout.
int infoMenuTab = 0;


// === CORE: lifecycle, render shell, transitions, utilities ===

Game::Game()
    : screenWidth(Constants::SCREEN_WIDTH),
      screenHeight(Constants::SCREEN_HEIGHT) {}


// =============================================================================
// Init
// =============================================================================
void Game::init() {
  InitWindow(screenWidth, screenHeight, "INFERNUS");
  SetExitKey(0); // Disable ESC auto-close — we handle ESC ourselves
  SetTargetFPS(Constants::TARGET_FPS);
  DebugPanel::setup();
  LuaEngine::setup();
  PartikelEmitters::init();
  inputManager.init();
  cameraSystem.init(screenWidth, screenHeight);
  AudioManager::getInstance().init();
  AudioManager::getInstance().playMusic("menu");
  abilitySystem.loadAbilities("assets/data/abilities.json");
  abilitySystem.loadActiveAbilities("assets/data/active_abilities.json");
  synergySystem.loadSynergies("assets/data/synergies.json");
  itemSystem.loadItems("assets/data/items.json");
  // Wire dispatcher to live subsystems so screen_shake / add_hitstop can fire.
  AnimEventDispatcher::wire(&cameraSystem, &screenEffects);
  AnimEventDispatcher::load("assets/data/animation_events/events.json");
  saveManager.load();

  // Preload art textures so they're ready before the first frame
  auto &res = ResourceManager::getInstance();
  res.getTexture("assets/art/title_bg.png");
  res.getTexture("assets/art/parallax_dungeon.png");
  res.getTexture("assets/art/ui_panel.png");
  res.getTexture("assets/art/portrait_warrior.png");
  res.getTexture("assets/art/portrait_rogue.png");
  res.getTexture("assets/art/portrait_knight.png");
  res.getTexture("assets/art/portrait_minotaur.png");
  res.getTexture("assets/art/portrait_infernal_knight.png");
  res.getTexture("assets/art/portrait_soul_archer.png");
  res.getTexture("assets/art/portrait_pit_fiend.png");

  // Post-Processing Initializations
  renderTarget = LoadRenderTexture(screenWidth, screenHeight);
  // Fixed the blurriness
  SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_POINT);
  crtVignetteShader = LoadShader(0, "src/shaders/CRT_Vignette.fs");
  renderSizeLoc = GetShaderLocation(crtVignetteShader, "renderSize");
  float sz[2] = { (float)screenWidth, (float)screenHeight };
  SetShaderValue(crtVignetteShader, renderSizeLoc, sz, SHADER_UNIFORM_VEC2);
  
  if (testMode) {
      state = GameState::PLAYING;
      startGame(0);
  }
}


// =============================================================================
// Update
// =============================================================================
// Start a smooth screen transition (fade out → callback → fade in)
void Game::transitionTo(GameState target, float duration) {
  if (transitionPhase != 0) return; // Already in a transition
  pendingTransition = target;
  transitionDuration = duration;
  transitionPhase = 1; // Start fading out
  screenEffects.startFadeOut(duration);
}


// =============================================================================
// Render
// =============================================================================
void Game::render() {
  INFERNUS_ZONE_N("Game::render");
  BeginTextureMode(renderTarget);
  ClearBackground(Color{10, 10, 15, 255});

  switch (state) {
  case GameState::MAIN_MENU:
    drawMainMenu();
    break;
  case GameState::CHARACTER_SELECT:
    drawCharacterSelect();
    break;
  case GameState::OPTIONS:
    drawOptions();
    break;
  case GameState::PAUSED:
    // Draw game underneath
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawHUD();
    drawPauseMenu();
    break;
  case GameState::ABILITY_SELECT:
    drawAbilitySelect();
    break;
  case GameState::GAME_OVER:
    drawGameOver();
    break;
  case GameState::VICTORY:
    drawVictory();
    break;
  case GameState::BOSS_INTRO:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawBossIntro();
    break;
  case GameState::INVENTORY:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawInventory();
    break;
  case GameState::INFO:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawInfoMenu();
    break;
  case GameState::ITEM_SWAP:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawItemSwap();
    break;
  case GameState::MAP_SELECT:
    drawMapSelect();
    break;
  case GameState::SHOP:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawShop();
    break;
  case GameState::REST:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawRest();
    break;
  case GameState::PLAYING:
  default: {

    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    renderAtmosphericParticles();
    PartikelEmitters::draw();
    if (damageNumbersEnabled) UIRenderer::renderDamageNumbers(registry);
    EndMode2D();
    drawHUD();
    if (bossEntity != NULL_ENTITY && registry.isAlive(bossEntity))
      drawBossHealthBar();
    drawMiniBossHealthBar();
    drawMinimap();
    break;
  }
  }

  // Screen effects overlay (fade)
  screenEffects.render(screenWidth, screenHeight);
  if (showDebug && state == GameState::PLAYING) drawDebugOverlay();

  EndTextureMode();

  // === DUMP RENDER TARGET TO SCREEN WITH SHADER ===
  BeginDrawing();
  ClearBackground(BLACK);
  
  // Update Shader Uniforms (optional dynamic time injection, etc.)
  // float time = GetTime();
  // SetShaderValue(crtVignetteShader, GetShaderLocation(crtVignetteShader, "time"), &time, SHADER_UNIFORM_FLOAT);
  
  BeginShaderMode(crtVignetteShader);
  // Note: Y is flipped in OpenGL framebuffers!
  DrawTextureRec(renderTarget.texture,
                 Rectangle{ 0, 0, (float)renderTarget.texture.width, -(float)renderTarget.texture.height },
                 Vector2{ 0, 0 }, WHITE);
  EndShaderMode();

  // Debug panel (ImGui) — drawn on top of the shader output, inside BeginDrawing/EndDrawing.
  // Compiles to a no-op on WASM builds (no INFERNUS_IMGUI define).
  DebugPanel::draw(registry);

  EndDrawing();
}


// =============================================================================
// recalculatePlayerStats — aggregates base + abilities + items → final
// =============================================================================
void Game::recalculatePlayerStats() {
  if (!registry.hasComponent<PlayerStats>(playerEntity))
    return;

  auto &ps = registry.getComponent<PlayerStats>(playerEntity);

  // Reset to base
  ps.finalMaxHP = ps.baseMaxHP;
  ps.finalDamage = ps.baseDamage;
  ps.finalSpeed = ps.baseSpeed;
  ps.finalMaxStamina = ps.baseMaxStamina;
  ps.finalStaminaRegen = ps.baseStaminaRegen;
  ps.finalCritChance = 0.0f;
  ps.finalLifesteal = 0.0f;
  ps.finalThorns = 0.0f;
  ps.finalWindupMultiplier = 1.0f;
  ps.extraHitboxes = 0;
  ps.projectileAttack = false;
  ps.areaAttack = false;
  ps.chainAttack = false;
  ps.chainBounces = 0;
  ps.fireTrailChance = 0.0f;

  // Apply ability bonuses
  if (registry.hasComponent<AbilityHolder>(playerEntity)) {
    auto &ah = registry.getComponent<AbilityHolder>(playerEntity);
    ps.finalDamage = (int)(ps.finalDamage * ah.damageMultiplier);
    ps.finalSpeed *= ah.speedMultiplier;
    ps.finalCritChance += ah.critChance;
    ps.finalLifesteal += ah.lifestealFraction;
    ps.finalStaminaRegen *= ah.staminaRegenMultiplier;
    ps.finalWindupMultiplier = ah.windupMultiplier;
    ps.finalThorns += ah.thornsDamage;
    ps.finalMaxHP += ah.bonusMaxHP;
    ps.dashDamage = ah.dashDamage;
  }

  // Apply item effects
  if (registry.hasComponent<ItemHolder>(playerEntity)) {
    auto &ih = registry.getComponent<ItemHolder>(playerEntity);
    for (auto &item : ih.equippedItems) {
      for (auto &eff : item.effects) {
        switch (eff.type) {
        case ItemEffect::STAT_DAMAGE:
          ps.finalDamage = (int)(ps.finalDamage * (1.0f + eff.value));
          break;
        case ItemEffect::STAT_SPEED:
          ps.finalSpeed *= (1.0f + eff.value);
          break;
        case ItemEffect::STAT_MAX_HP:
          ps.finalMaxHP += (int)eff.value;
          break;
        case ItemEffect::STAT_CRIT:
          ps.finalCritChance += eff.value;
          break;
        case ItemEffect::STAT_LIFESTEAL:
          ps.finalLifesteal += eff.value;
          break;
        case ItemEffect::STAT_STAMINA_REGEN:
          ps.finalStaminaRegen *= (1.0f + eff.value);
          break;
        case ItemEffect::STAT_ATTACK_SPEED:
          ps.finalWindupMultiplier -= eff.value;
          break;
        case ItemEffect::STAT_THORNS:
          ps.finalThorns += eff.value;
          break;
        case ItemEffect::MULTI_HIT:
          ps.extraHitboxes += (int)eff.value;
          break;
        case ItemEffect::PROJECTILE_ATTACK:
          ps.projectileAttack = true;
          break;
        case ItemEffect::AREA_ATTACK:
          ps.areaAttack = true;
          break;
        case ItemEffect::CHAIN_ATTACK:
          ps.chainAttack = true;
          ps.chainBounces = (int)eff.value;
          break;
        case ItemEffect::FIRE_TRAIL:
          ps.fireTrailChance += eff.value;
          break;
        }
      }
    }
  }

  // Apply synergy bonuses
  auto &synDefs = synergySystem.getDefs();
  auto &synStates = synergySystem.getStates();
  for (int i = 0; i < (int)synDefs.size(); i++) {
    if (!synStates[i].active)
      continue;
    ps.finalDamage = (int)(ps.finalDamage * (1.0f + synDefs[i].bonusDamage));
    ps.finalSpeed *= (1.0f + synDefs[i].bonusSpeed);
    ps.finalCritChance += synDefs[i].bonusCrit;
    ps.finalLifesteal += synDefs[i].bonusLifesteal;
    ps.finalMaxHP += synDefs[i].bonusMaxHP;
    ps.finalStaminaRegen *= (1.0f + synDefs[i].bonusStaminaRegen);
  }

  // Clamp values to safe ranges
  if (ps.finalWindupMultiplier < 0.3f) ps.finalWindupMultiplier = 0.3f;
  if (ps.finalCritChance > 0.8f) ps.finalCritChance = 0.8f;
  if (ps.finalLifesteal > 1.0f) ps.finalLifesteal = 1.0f;
  if (ps.finalDamage < 1) ps.finalDamage = 1;
  if (ps.finalMaxHP < 10) ps.finalMaxHP = 10;
  if (ps.finalSpeed > ps.baseSpeed * 2.0f) ps.finalSpeed = ps.baseSpeed * 2.0f; // cap at 2x base

  // Apply final stats to actual components
  if (registry.hasComponent<Health>(playerEntity)) {
    auto &h = registry.getComponent<Health>(playerEntity);
    int hpDiff = ps.finalMaxHP - h.maxHP;
    h.maxHP = ps.finalMaxHP;
    if (hpDiff > 0)
      h.currentHP = std::min(h.currentHP + hpDiff, h.maxHP);
    if (h.currentHP > h.maxHP)
      h.currentHP = h.maxHP;
  }
  if (registry.hasComponent<Combat>(playerEntity)) {
    registry.getComponent<Combat>(playerEntity).baseDamage = ps.finalDamage;
  }
  if (registry.hasComponent<Stamina>(playerEntity)) {
    auto &s = registry.getComponent<Stamina>(playerEntity);
    s.maxStamina = ps.finalMaxStamina;
    s.regenRate = ps.finalStaminaRegen;
  }
}


// Local helper used by drawUIPanel — renders a texture as a scaled 9-patch
// (corners stay fixed, edges and center stretch). Kept static to limit scope.
static void drawScaledNPatch(Texture2D tex, Rectangle dst, int srcBorder, int dstBorder, Color tint) {
    int w = tex.width;
    int h = tex.height;
    int sb = srcBorder;

    Rectangle srcTL = { 0.f, 0.f, (float)sb, (float)sb };
    Rectangle srcT  = { (float)sb, 0.f, (float)(w - 2*sb), (float)sb };
    Rectangle srcTR = { (float)(w - sb), 0.f, (float)sb, (float)sb };
    Rectangle srcL  = { 0.f, (float)sb, (float)sb, (float)(h - 2*sb) };
    Rectangle srcC  = { (float)sb, (float)sb, (float)(w - 2*sb), (float)(h - 2*sb) };
    Rectangle srcR  = { (float)(w - sb), (float)sb, (float)sb, (float)(h - 2*sb) };
    Rectangle srcBL = { 0.f, (float)(h - sb), (float)sb, (float)sb };
    Rectangle srcB  = { (float)sb, (float)(h - sb), (float)(w - 2*sb), (float)sb };
    Rectangle srcBR = { (float)(w - sb), (float)(h - sb), (float)sb, (float)sb };

    float dw = dst.width;
    float dh = dst.height;
    float adbX = std::min((float)dstBorder, dw / 2.0f);
    float adbY = std::min((float)dstBorder, dh / 2.0f);

    Rectangle dstTL = { dst.x, dst.y, adbX, adbY };
    Rectangle dstT  = { dst.x + adbX, dst.y, std::max(0.0f, dw - 2*adbX), adbY };
    Rectangle dstTR = { dst.x + dw - adbX, dst.y, adbX, adbY };
    Rectangle dstL  = { dst.x, dst.y + adbY, adbX, std::max(0.0f, dh - 2*adbY) };
    Rectangle dstC  = { dst.x + adbX, dst.y + adbY, std::max(0.0f, dw - 2*adbX), std::max(0.0f, dh - 2*adbY) };
    Rectangle dstR  = { dst.x + dw - adbX, dst.y + adbY, adbX, std::max(0.0f, dh - 2*adbY) };
    Rectangle dstBL = { dst.x, dst.y + dh - adbY, adbX, adbY };
    Rectangle dstB  = { dst.x + adbX, dst.y + dh - adbY, std::max(0.0f, dw - 2*adbX), adbY };
    Rectangle dstBR = { dst.x + dw - adbX, dst.y + dh - adbY, adbX, adbY };

    DrawTexturePro(tex, srcTL, dstTL, {0,0}, 0, tint);
    DrawTexturePro(tex, srcT,  dstT,  {0,0}, 0, tint);
    DrawTexturePro(tex, srcTR, dstTR, {0,0}, 0, tint);
    DrawTexturePro(tex, srcL,  dstL,  {0,0}, 0, tint);
    DrawTexturePro(tex, srcC,  dstC,  {0,0}, 0, tint);
    DrawTexturePro(tex, srcR,  dstR,  {0,0}, 0, tint);
    DrawTexturePro(tex, srcBL, dstBL, {0,0}, 0, tint);
    DrawTexturePro(tex, srcB,  dstB,  {0,0}, 0, tint);
    DrawTexturePro(tex, srcBR, dstBR, {0,0}, 0, tint);
}

// =============================================================================
// UI Panel — draws a panel with the art texture using scaled 9-patch
// =============================================================================
void Game::drawUIPanel(int x, int y, int w, int h, Color fallbackBg,
                       Color /*borderCol*/) {
  Texture2D panelTex = ResourceManager::getInstance().getTexture(
      "assets/art/ui_panel.png");
  if (panelTex.id > 0) {
    Rectangle dst = {(float)x, (float)y, (float)w, (float)h};
    
    // Source border size: Assuming standard 1024x1024 AI gen image, border is ~180px
    int sourceBorder = panelTex.width * 0.18f; 
    
    // Destination border size: Scale down to 30px so it frames nicely without eating up space
    int destBorder = 30; 
    
    drawScaledNPatch(panelTex, dst, sourceBorder, destBorder, Color{255, 255, 255, fallbackBg.a});
    
    // Extra darkening center to make text super clear
    DrawRectangle(x + destBorder, y + destBorder, w - 2*destBorder, h - 2*destBorder, Color{10, 10, 15, (unsigned char)(fallbackBg.a * 0.5f)});
  } else {
    DrawRectangle(x, y, w, h, fallbackBg);
  }
}


// =============================================================================
// Atmospheric Particles — ash, embers, subtle fog floating in dungeon
// =============================================================================
void Game::updateAtmosphericParticles(float deltaTime) {
  float roomW = (float)roomGenerator.getPixelWidth();
  float roomH = (float)roomGenerator.getPixelHeight();
  if (roomW < 1.0f)
    return;

  // Spawn new ash/ember particles
  ashSpawnTimer -= deltaTime;
  if (ashSpawnTimer <= 0.0f) {
    ashSpawnTimer = 0.06f;
    if ((int)ashParticles.size() < 80) {
      AshParticle p;
      p.x = (float)GetRandomValue(0, (int)roomW);
      p.y = (float)GetRandomValue(0, (int)roomH);
      p.vx = (float)GetRandomValue(-15, 15);
      p.vy = (float)GetRandomValue(-30, -5);
      p.maxLife = 3.0f + (float)GetRandomValue(0, 200) / 100.0f;
      p.life = p.maxLife;
      p.size = 1.0f + (float)GetRandomValue(0, 20) / 10.0f;
      // Mix of ash (gray) and embers (orange)
      if (GetRandomValue(0, 3) == 0)
        p.color = Color{255, (unsigned char)GetRandomValue(100, 180), 30,
                        (unsigned char)GetRandomValue(120, 200)};
      else
        p.color = Color{(unsigned char)GetRandomValue(60, 100),
                        (unsigned char)GetRandomValue(55, 90),
                        (unsigned char)GetRandomValue(50, 80),
                        (unsigned char)GetRandomValue(80, 150)};
      p.frameIndex = GetRandomValue(0, 3);
      ashParticles.push_back(p);
    }
  }

  // Update existing particles
  for (int i = (int)ashParticles.size() - 1; i >= 0; i--) {
    auto &p = ashParticles[i];
    p.life -= deltaTime;
    if (p.life <= 0.0f) {
      ashParticles.erase(ashParticles.begin() + i);
      continue;
    }
    p.x += p.vx * deltaTime;
    p.y += p.vy * deltaTime;
    // Gentle sway
    p.vx += sinf(p.y * 0.02f + (float)GetTime()) * 8.0f * deltaTime;
  }
}


void Game::renderAtmosphericParticles() {
  auto &res = ResourceManager::getInstance();
  Texture2D ashTex = res.getTexture("assets/sprites/particles/ash_particle.png");
  bool hasTexture = (ashTex.id > 0);

  for (auto &p : ashParticles) {
    float alpha = (p.life / p.maxLife);
    Color c = p.color;
    c.a = (unsigned char)(c.a * alpha);

    if (hasTexture) {
      Rectangle src = {(float)(p.frameIndex * 8), 0.0f, 8.0f, 8.0f};
      Rectangle dest = {p.x, p.y, p.size * 4.0f, p.size * 4.0f};
      DrawTexturePro(ashTex, src, dest, {0, 0}, 0.0f, c);
    } else {
      DrawRectangle((int)p.x, (int)p.y, (int)p.size, (int)p.size, c);
    }
  }
}


// =============================================================================
// Lifecycle
// =============================================================================
void Game::shutdown() {
  saveManager.save();
  Telemetry::close();
  UnloadRenderTexture(renderTarget);
  UnloadShader(crtVignetteShader);
  AudioManager::getInstance().shutdown();
  ResourceManager::getInstance().unloadAll();
  PartikelEmitters::shutdown();
  LuaEngine::shutdown();
  DebugPanel::shutdown();
  CloseWindow();
}


void Game::run() {
  init();
  while (!WindowShouldClose()) {
    update(GetFrameTime());
    render();
    INFERNUS_FRAME;
  }
  shutdown();
}
