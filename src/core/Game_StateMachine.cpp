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


// === SM: input handling + state machine update switch ===

// =============================================================================
// Player Input
// =============================================================================
void Game::handlePlayerInput(float deltaTime) {
  if (!registry.isAlive(playerEntity))
    return;
  if (!registry.hasComponent<Velocity>(playerEntity))
    return;

  auto &velocity = registry.getComponent<Velocity>(playerEntity);
  velocity.vx = 0.0f;
  velocity.vy = 0.0f;

  float speed = LuaEngine::getFeel("player_speed", Constants::PLAYER_SPEED);
  if (registry.hasComponent<PlayerStats>(playerEntity))
    speed = registry.getComponent<PlayerStats>(playerEntity).finalSpeed;

  // Analog gamepad stick or digital WASD/arrows
  auto move = inputManager.getMoveAxis();
  velocity.vx = move.x * speed;
  velocity.vy = move.y * speed;

  // Update facing direction (for attacks) and sprite flip
  auto &playerTransform = registry.getComponent<Transform2D>(playerEntity);
  if (velocity.vx != 0.0f || velocity.vy != 0.0f) {
    float len = sqrtf(velocity.vx * velocity.vx + velocity.vy * velocity.vy);
    playerTransform.facingX = velocity.vx / len;
    playerTransform.facingY = velocity.vy / len;
  }
  if (registry.hasComponent<Sprite>(playerEntity)) {
    auto &sprite = registry.getComponent<Sprite>(playerEntity);
    if (velocity.vx < 0.0f)
      sprite.flipX = true;
    else if (velocity.vx > 0.0f)
      sprite.flipX = false;
  }

  // Animation state transitions (idle ↔ run)
  if (registry.hasComponent<AnimState>(playerEntity)) {
    auto &as = registry.getComponent<AnimState>(playerEntity);
    bool moving = (velocity.vx != 0.0f || velocity.vy != 0.0f);
    if (as.current != AnimStateType::ATTACK) {
      as.setState(moving ? AnimStateType::RUN : AnimStateType::IDLE);
    }
  }

  if (!registry.hasComponent<Combat>(playerEntity) ||
      !registry.hasComponent<Stamina>(playerEntity))
    return;

  auto &combat = registry.getComponent<Combat>(playerEntity);
  auto &stamina = registry.getComponent<Stamina>(playerEntity);

  // Get windup multiplier from player stats (attack speed abilities/items)
  float windupMult = 1.0f;
  if (registry.hasComponent<PlayerStats>(playerEntity))
    windupMult = registry.getComponent<PlayerStats>(playerEntity).finalWindupMultiplier;
  if (windupMult < 0.3f) windupMult = 0.3f; // Cap at 70% speed increase

  // Combo timer decay
  if (combat.comboTimer > 0.0f) {
    combat.comboTimer -= deltaTime;
    if (combat.comboTimer <= 0.0f)
      combat.comboCount = 0; // combo dropped
  }

  // Lua-tunable feel parameters (cached once per call)
  const float fParryStamina  = LuaEngine::getFeel("parry_stamina",  Constants::PARRY_STAMINA);
  const float fParryWindow   = LuaEngine::getFeel("parry_window",   Constants::PARRY_WINDOW);
  const float fHeavyStamina  = LuaEngine::getFeel("heavy_attack_stamina", Constants::HEAVY_ATTACK_STAMINA);
  const float fHeavyWindup   = LuaEngine::getFeel("heavy_attack_windup",  Constants::HEAVY_ATTACK_WINDUP);
  const float fLightStamina  = LuaEngine::getFeel("light_attack_stamina", Constants::LIGHT_ATTACK_STAMINA);
  const float fLightWindup   = LuaEngine::getFeel("light_attack_windup",  Constants::LIGHT_ATTACK_WINDUP);
  const float fComboWindow   = LuaEngine::getFeel("combo_window",   Constants::COMBO_WINDOW);
  const float fDashStamina   = LuaEngine::getFeel("dash_stamina",   Constants::DASH_STAMINA);
  const float fDashIframes   = LuaEngine::getFeel("dash_iframes",   Constants::DASH_IFRAMES);
  const float fDashSpeed     = LuaEngine::getFeel("dash_speed",     Constants::DASH_SPEED);

  if (combat.currentState == AttackState::NONE) {
    // --- Parry (F / RB) ---
    if (inputManager.isActionPressed(InputAction::PARRY) &&
        stamina.hasEnough(fParryStamina)) {
      combat.currentState = AttackState::PARRY_ACTIVE;
      combat.stateTimer = fParryWindow;
      stamina.currentStamina -= fParryStamina;
      stamina.cooldownTimer = stamina.regenDelay;
      AudioManager::getInstance().playSFX("dash"); // reuse for now
    }
    // --- Combo finisher: 3 lights → heavy input triggers finisher ---
    else if (combat.comboCount >= 3 &&
             inputManager.isActionPressed(InputAction::ATTACK_HEAVY) &&
             stamina.hasEnough(fHeavyStamina)) {
      combat.currentState = AttackState::WINDUP;
      combat.stateTimer = fHeavyWindup * windupMult * 0.7f;
      combat.lastAttackType = AttackType::HEAVY;
      stamina.currentStamina -= fHeavyStamina;
      stamina.cooldownTimer = stamina.regenDelay;
      combat.comboCount = 0;
      combat.comboTimer = 0.0f;
      combat.isFinisher = true;
      AudioManager::getInstance().playSFX("attack_heavy");
      if (registry.hasComponent<AnimState>(playerEntity))
        registry.getComponent<AnimState>(playerEntity).setState(AnimStateType::ATTACK);
    }
    // --- Light attack (combo chain) ---
    else if (inputManager.isActionPressed(InputAction::ATTACK_LIGHT) &&
             stamina.hasEnough(fLightStamina)) {
      combat.currentState = AttackState::WINDUP;
      float speedup = 1.0f - combat.comboCount * 0.08f; // each hit slightly faster
      combat.stateTimer = fLightWindup * windupMult * speedup;
      combat.lastAttackType = AttackType::LIGHT;
      stamina.currentStamina -= fLightStamina;
      stamina.cooldownTimer = stamina.regenDelay;
      combat.comboCount++;
      combat.comboTimer = fComboWindow;
      AudioManager::getInstance().playSFX("attack_light");
      if (registry.hasComponent<AnimState>(playerEntity))
        registry.getComponent<AnimState>(playerEntity).setState(AnimStateType::ATTACK);
    }
    // --- Heavy attack (standalone) ---
    else if (inputManager.isActionPressed(InputAction::ATTACK_HEAVY) &&
             stamina.hasEnough(fHeavyStamina)) {
      combat.currentState = AttackState::WINDUP;
      combat.stateTimer = fHeavyWindup * windupMult;
      combat.lastAttackType = AttackType::HEAVY;
      stamina.currentStamina -= fHeavyStamina;
      stamina.cooldownTimer = stamina.regenDelay;
      combat.comboCount = 0;
      combat.comboTimer = 0.0f;
      AudioManager::getInstance().playSFX("attack_heavy");
      if (registry.hasComponent<AnimState>(playerEntity))
        registry.getComponent<AnimState>(playerEntity).setState(AnimStateType::ATTACK);
    }
  }

  if (inputManager.isActionPressed(InputAction::DASH) &&
      stamina.hasEnough(fDashStamina)) {
    stamina.currentStamina -= fDashStamina;
    stamina.cooldownTimer = stamina.regenDelay;
    AudioManager::getInstance().playSFX("dash");

    if (registry.hasComponent<Health>(playerEntity))
      registry.getComponent<Health>(playerEntity).invulnerabilityTimer = fDashIframes;

    // libpartikel: dust burst at player feet in dash direction
    {
      auto &pt = registry.getComponent<Transform2D>(playerEntity);
      float dirX = (velocity.vx != 0.0f || velocity.vy != 0.0f)
                   ? velocity.vx
                   : (registry.hasComponent<Sprite>(playerEntity) &&
                      registry.getComponent<Sprite>(playerEntity).flipX ? -1.0f : 1.0f);
      PartikelEmitters::spawnDashDust(pt.x + 16.0f, pt.y + 40.0f, dirX);
    }

    if (velocity.vx != 0.0f || velocity.vy != 0.0f) {
      float len = sqrtf(velocity.vx * velocity.vx + velocity.vy * velocity.vy);
      velocity.vx = (velocity.vx / len) * fDashSpeed;
      velocity.vy = (velocity.vy / len) * fDashSpeed;
    } else {
      bool left = registry.hasComponent<Sprite>(playerEntity) &&
                  registry.getComponent<Sprite>(playerEntity).flipX;
      velocity.vx = left ? -fDashSpeed : fDashSpeed;
    }

    // Spawn dash ghost trail
    if (registry.hasComponent<Transform2D>(playerEntity)) {
      auto &pt = registry.getComponent<Transform2D>(playerEntity);
      auto &res = ResourceManager::getInstance();
      Texture2D ghostTex = res.getTexture("assets/sprites/fx/dash_ghost.png");
      for (int i = 0; i < 3; i++) {
        Entity ghost = registry.createEntity();
        float gx = pt.x + (float)i * (-velocity.vx * 0.02f);
        float gy = pt.y + (float)i * (-velocity.vy * 0.02f);
        registry.addComponent<Transform2D>(ghost, gx, gy);
        registry.addComponent<Sprite>(ghost, ghostTex,
                                      Rectangle{0, 0, 8, 16}, 9);
        registry.addComponent<Lifetime>(ghost, 0.15f + i * 0.05f);
        Color ghostCol = {140, 140, 170, (unsigned char)(150 - i * 40)};
        registry.addComponent<Particle>(ghost, ghostCol,
                                        Color{100, 120, 180, 0}, 1.0f, 0.5f);
      }

      // Dash damage: cover the whole dash path with a massive expanding hitbox
      if (registry.hasComponent<PlayerStats>(playerEntity) &&
          registry.getComponent<PlayerStats>(playerEntity).dashDamage) {
        int dashDmg = combat.baseDamage;
        float fx = velocity.vx;
        float fy = velocity.vy;
        float len = sqrtf(fx * fx + fy * fy);
        if (len > 0.1f) { fx /= len; fy /= len; } else { fx = 0; fy = 0; }
        
        float dashDistance = fDashSpeed * fDashIframes;
        Entity dashHit = registry.createEntity();
        registry.addComponent<Transform2D>(dashHit, pt.x + (fx * dashDistance)/2.0f - dashDistance/2.0f, pt.y + (fy * dashDistance)/2.0f - dashDistance/2.0f);
        
        float hw = std::abs(fx) > 0.5f ? dashDistance + 40.0f : 40.0f;
        float hh = std::abs(fy) > 0.5f ? dashDistance + 40.0f : 40.0f;
        registry.addComponent<Collider>(dashHit, hw, hh);
        registry.addComponent<Combat>(dashHit, dashDmg, 150.0f, playerEntity);
        registry.addComponent<Lifetime>(dashHit, fDashIframes);
      }
    }
  }

  // Special attack (class ability)
  if (inputManager.isActionPressed(InputAction::SPECIAL_ATTACK) &&
      specialCooldownTimer <= 0.0f &&
      stamina.hasEnough(30.0f)) {
    stamina.currentStamina -= 30.0f;
    stamina.cooldownTimer = stamina.regenDelay;
    executeSpecialAttack();
  }
  // --- Procedural Animation Synchronization ---
  if (registry.hasComponent<Animation>(playerEntity) && registry.hasComponent<Combat>(playerEntity)) {
      auto& anim = registry.getComponent<Animation>(playerEntity);
      
      float dashInvul = 0.0f;
      if (registry.hasComponent<Health>(playerEntity)) {
          dashInvul = registry.getComponent<Health>(playerEntity).invulnerabilityTimer;
      }
      
      if (dashInvul > 0.0f) {
           if (anim.startY != 168.0f) { // Row 3: 56 * 3 (Dash/Ulti)
              anim.startY = 168.0f;
              anim.frames = 2;
              anim.currentFrame = 0;
              anim.frameSpeed = 0.05f;
              anim.loop = false;
              anim.finished = false;
          }
      } else if (combat.currentState == AttackState::WINDUP || combat.currentState == AttackState::ACTIVE) {
          if (anim.startY != 112.0f) { // Row 2: 56 * 2 (Attack)
              anim.startY = 112.0f;
              anim.frames = 3;
              anim.currentFrame = 0;
              anim.frameSpeed = 0.08f;
              anim.loop = false;
              anim.finished = false;
          }
      } else if (velocity.vx != 0.0f || velocity.vy != 0.0f) {
          if (anim.startY != 56.0f) { // Row 1: 56 * 1 (Walk)
              // Only interrupt if not attacking, or attack finished
              anim.startY = 56.0f;
              anim.frames = 6;
              anim.currentFrame = 0;
              anim.frameSpeed = 0.08f;
              anim.loop = true;
              anim.finished = false;
          }
      } else {
          // Idle
          if (anim.startY != 0.0f && (anim.loop || anim.finished)) { // Row 0: 0 (Idle)
              anim.startY = 0.0f;
              anim.frames = 2;
              anim.currentFrame = 0;
              anim.frameSpeed = 0.4f;
              anim.loop = true;
              anim.finished = false;
          }
      }
  }

  // === Active abilities (Q / E) ===
  if (inputManager.isActionPressed(InputAction::ABILITY_Q)) {
    abilitySystem.tryUseActive(registry, playerEntity, 0);
  }
  if (inputManager.isActionPressed(InputAction::ABILITY_E)) {
    abilitySystem.tryUseActive(registry, playerEntity, 1);
  }
}


// =============================================================================
// Special Attacks (class-specific abilities)
// =============================================================================
void Game::executeSpecialAttack() {
  if (!registry.hasComponent<Transform2D>(playerEntity) ||
      !registry.hasComponent<Combat>(playerEntity))
    return;

  auto &pt = registry.getComponent<Transform2D>(playerEntity);
  auto &combat = registry.getComponent<Combat>(playerEntity);
  auto &res = ResourceManager::getInstance();

  if (currentCharacterId == "warrior") {
    // WARRIOR — Golpe Sismico: AoE ground slam around player
    AudioManager::getInstance().playSFX("attack_heavy");
    cameraSystem.addShake(12.0f, 0.4f);
    specialCooldownTimer = 3.0f;

    // Spawn 4 hitboxes in cardinal directions
    float range = 60.0f;
    float offsets[][2] = {{range, 0}, {-range, 0}, {0, range}, {0, -range}};
    for (int i = 0; i < 4; i++) {
      Entity hb = registry.createEntity();
      registry.addComponent<Transform2D>(hb, pt.x + offsets[i][0] - 30,
                                         pt.y + offsets[i][1] - 30);
      registry.addComponent<Collider>(hb, 60.0f, 60.0f);
      registry.addComponent<Combat>(hb, combat.baseDamage * 2,
                                    250.0f, playerEntity);
      registry.addComponent<Lifetime>(hb, 0.2f);
    }

    // VFX: ring of particles
    Texture2D hitTex = res.getTexture("assets/sprites/fx/hit_particle.png");
    for (int i = 0; i < 16; i++) {
      float angle = (float)i * (3.14159f * 2.0f / 16.0f);
      Entity p = registry.createEntity();
      registry.addComponent<Transform2D>(p, pt.x + 20, pt.y + 20);
      registry.addComponent<Velocity>(p, cosf(angle) * 300.0f,
                                      sinf(angle) * 300.0f);
      registry.addComponent<Lifetime>(p, 0.4f);
      registry.addComponent<Sprite>(p, hitTex, Rectangle{0, 0, 4, 4}, 11);
      registry.addComponent<Particle>(p, Color{255, 100, 50, 255},
                                      Color{255, 50, 0, 0}, 1.5f, 0.3f);
    }

  } else if (currentCharacterId == "rogue") {
    // ROGUE — Golpe Sombra: teleport forward + damage line
    AudioManager::getInstance().playSFX("dash");
    specialCooldownTimer = 2.0f;

    float fx = pt.facingX;
    float fy = pt.facingY;
    float len = sqrtf(fx * fx + fy * fy);
    if (len < 0.1f) { fx = 1.0f; fy = 0.0f; len = 1.0f; }
    fx /= len;
    fy /= len;

    float teleportDist = 120.0f;

    // Spawn a large damage line covering the teleport path
    Entity hb = registry.createEntity();
    registry.addComponent<Transform2D>(hb, pt.x + (fx * teleportDist)/2.0f - teleportDist/2.0f,
                                       pt.y + (fy * teleportDist)/2.0f - teleportDist/2.0f);
    float hw = std::abs(fx) > 0.5f ? teleportDist + 60.0f : 60.0f;
    float hh = std::abs(fy) > 0.5f ? teleportDist + 60.0f : 60.0f;
    registry.addComponent<Collider>(hb, hw, hh);
    registry.addComponent<Combat>(hb, combat.baseDamage * 3, 100.0f, playerEntity);
    registry.addComponent<Lifetime>(hb, 0.2f);

    // Teleport player
    pt.x += fx * teleportDist;
    pt.y += fy * teleportDist;

    // Spawn ghost trail
    Texture2D ghostTex = res.getTexture("assets/sprites/fx/dash_ghost.png");
    for (int i = 0; i < 5; i++) {
      Entity ghost = registry.createEntity();
      float gx = pt.x - fx * teleportDist * ((float)i / 5.0f);
      float gy = pt.y - fy * teleportDist * ((float)i / 5.0f);
      registry.addComponent<Transform2D>(ghost, gx, gy);
      registry.addComponent<Sprite>(ghost, ghostTex,
                                    Rectangle{0, 0, 8, 16}, 9);
      registry.addComponent<Lifetime>(ghost, 0.2f + i * 0.04f);
      Color gc = {50, 200, 50, (unsigned char)(200 - i * 35)};
      registry.addComponent<Particle>(ghost, gc,
                                      Color{30, 100, 30, 0}, 1.0f, 0.4f);
    }

    // Brief iframes
    if (registry.hasComponent<Health>(playerEntity))
      registry.getComponent<Health>(playerEntity).invulnerabilityTimer = 0.3f;

  } else if (currentCharacterId == "knight") {
    // KNIGHT — Escudo Oseo: temporary invulnerability shield + thorns
    AudioManager::getInstance().playSFX("menu_confirm");
    specialCooldownTimer = 5.0f;

    // Grant long invulnerability
    if (registry.hasComponent<Health>(playerEntity))
      registry.getComponent<Health>(playerEntity).invulnerabilityTimer = 2.0f;

    cameraSystem.addShake(4.0f, 0.2f);

    // VFX: shield particles in a circle
    Texture2D hitTex = res.getTexture("assets/sprites/fx/hit_particle.png");
    for (int i = 0; i < 12; i++) {
      float angle = (float)i * (3.14159f * 2.0f / 12.0f);
      Entity p = registry.createEntity();
      registry.addComponent<Transform2D>(p, pt.x + 20 + cosf(angle) * 25.0f,
                                         pt.y + 20 + sinf(angle) * 25.0f);
      registry.addComponent<Velocity>(p, cosf(angle) * 50.0f,
                                      sinf(angle) * 50.0f);
      registry.addComponent<Lifetime>(p, 1.5f);
      registry.addComponent<Sprite>(p, hitTex, Rectangle{0, 0, 4, 4}, 11);
      registry.addComponent<Particle>(p, Color{80, 120, 255, 200},
                                      Color{40, 60, 180, 0}, 1.0f, 0.2f);
    }

    // Spawn retaliatory hitbox around player (thorns effect)
    Entity thornBox = registry.createEntity();
    registry.addComponent<Transform2D>(thornBox, pt.x - 30, pt.y - 30);
    registry.addComponent<Collider>(thornBox, 100.0f, 100.0f);
    registry.addComponent<Combat>(thornBox, combat.baseDamage,
                                  200.0f, playerEntity);
    registry.addComponent<Lifetime>(thornBox, 2.0f);
  }
}


void Game::update(float deltaTime) {
  INFERNUS_ZONE_N("Game::update");
  AudioManager::getInstance().update();
  DebugPanel::handleInput();
  LuaEngine::handleInput();

  // Apply debug time scale (also affects fades, which feels right for slo-mo tests)
  deltaTime *= DebugPanel::tunables().timeScale;

  // Update fade and flash timers globally (so they tick in ALL states,
  // not just PLAYING — fixes red flash persisting on death screen)
  screenEffects.updateFade(deltaTime);
  screenEffects.updateFlash(deltaTime);

  // Handle screen transitions
  if (transitionPhase == 1 && !screenEffects.isFadingOut()) {
    // Fade-out complete — run callback, switch state, start fade-in
    if (transitionCallback) { transitionCallback(); transitionCallback = nullptr; }
    state = pendingTransition;
    transitionPhase = 2;
    screenEffects.startFadeIn(transitionDuration);
  } else if (transitionPhase == 2 && !screenEffects.isFading()) {
    transitionPhase = 0;
  }
  if (transitionPhase == 1) return; // Block input during fade-out

  // Global debug toggle (F3)
  if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

  switch (state) {
  case GameState::MAIN_MENU:
    if (IsKeyPressed(KEY_ENTER)) {
      AudioManager::getInstance().playSFX("menu_confirm");
      selectedCharacter = 0;
      transitionTo(GameState::CHARACTER_SELECT);
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
      stateBeforeOptions = GameState::MAIN_MENU;
      optionSelected = 0;
      state = GameState::OPTIONS;
    }
    return;

  case GameState::OPTIONS: {
    auto &audio = AudioManager::getInstance();
    int optionCount = 6; // SFX, Music, Fullscreen, ScreenShake, DamageNumbers, Back
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      optionSelected = (optionSelected - 1 + optionCount) % optionCount;
      audio.playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      optionSelected = (optionSelected + 1) % optionCount;
      audio.playSFX("menu_select");
    }
    float step = 0.1f;
    if (optionSelected == 0) { // SFX Volume
      if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        float vol = std::max(0.0f, audio.getSFXVolume() - step);
        audio.setSFXVolume(vol);
      }
      if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        float vol = std::min(1.0f, audio.getSFXVolume() + step);
        audio.setSFXVolume(vol);
        audio.playSFX("menu_select");
      }
    } else if (optionSelected == 1) { // Music Volume
      if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        float vol = std::max(0.0f, audio.getMusicVolume() - step);
        audio.setMusicVolume(vol);
      }
      if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        float vol = std::min(1.0f, audio.getMusicVolume() + step);
        audio.setMusicVolume(vol);
      }
    } else if (optionSelected == 2) { // Fullscreen
      if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_LEFT) ||
          IsKeyPressed(KEY_RIGHT)) {
#ifdef __EMSCRIPTEN__
        EmscriptenFullscreenChangeEvent fsce;
        emscripten_get_fullscreen_status(&fsce);
        if (fsce.isFullscreen) {
          emscripten_exit_fullscreen();
        } else {
          emscripten_request_fullscreen("#canvas", true);
        }
#else
        ToggleFullscreen();
#endif
        audio.playSFX("menu_confirm");
      }
    } else if (optionSelected == 3) { // Screen Shake
      if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_LEFT) ||
          IsKeyPressed(KEY_RIGHT)) {
        screenShakeEnabled = !screenShakeEnabled;
        cameraSystem.shakeEnabled = screenShakeEnabled;
        audio.playSFX("menu_confirm");
      }
    } else if (optionSelected == 4) { // Damage Numbers
      if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_LEFT) ||
          IsKeyPressed(KEY_RIGHT)) {
        damageNumbersEnabled = !damageNumbersEnabled;
        audio.playSFX("menu_confirm");
      }
    } else if (optionSelected == 5) { // Back
      if (IsKeyPressed(KEY_ENTER)) {
        audio.playSFX("menu_confirm");
        state = stateBeforeOptions;
      }
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
      state = stateBeforeOptions;
    }
    return;
  }

  case GameState::CHARACTER_SELECT:
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
      selectedCharacter = (selectedCharacter - 1 + 3) % 3;
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
      selectedCharacter = (selectedCharacter + 1) % 3;
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_ENTER)) {
      AudioManager::getInstance().playSFX("menu_confirm");
      int charIdx = selectedCharacter;
      transitionCallback = [this, charIdx]() { startGame(charIdx); };
      transitionTo(GameState::PLAYING, 0.4f);
    }
    if (IsKeyPressed(KEY_ESCAPE))
      transitionTo(GameState::MAIN_MENU);
    return;

  case GameState::PAUSED: {
    int pauseCount = 3; // Continuar, Opciones, Abandonar
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      pauseSelected = (pauseSelected - 1 + pauseCount) % pauseCount;
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      pauseSelected = (pauseSelected + 1) % pauseCount;
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
      state = GameState::PLAYING;
    } else if (IsKeyPressed(KEY_ENTER)) {
      AudioManager::getInstance().playSFX("menu_confirm");
      if (pauseSelected == 0) {
        state = GameState::PLAYING;
      } else if (pauseSelected == 1) {
        stateBeforeOptions = GameState::PAUSED;
        optionSelected = 0;
        state = GameState::OPTIONS;
      } else if (pauseSelected == 2) {
        transitionCallback = [this]() {
          auto all = registry.view<Transform2D>();
          for (Entity e : all) registry.destroyEntity(e);
          registry.flushDestroyed();
          AudioManager::getInstance().playMusic("menu");
        };
        transitionTo(GameState::MAIN_MENU, 0.4f);
      }
    }
    return;
  }

  case GameState::GAME_OVER:
    if (IsKeyPressed(KEY_ENTER)) {
      AudioManager::getInstance().playSFX("menu_confirm");
      transitionCallback = [this]() {
        auto all = registry.view<Transform2D>();
        for (Entity e : all) registry.destroyEntity(e);
        registry.flushDestroyed();
        selectedCharacter = 0;
        AudioManager::getInstance().playMusic("menu");
      };
      transitionTo(GameState::CHARACTER_SELECT, 0.5f);
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
      transitionCallback = [this]() {
        auto all = registry.view<Transform2D>();
        for (Entity e : all) registry.destroyEntity(e);
        registry.flushDestroyed();
        AudioManager::getInstance().playMusic("menu");
      };
      transitionTo(GameState::MAIN_MENU, 0.5f);
    }
    return;

  case GameState::VICTORY:
    if (IsKeyPressed(KEY_ENTER)) {
      AudioManager::getInstance().playSFX("menu_confirm");
      transitionCallback = [this]() {
        auto all = registry.view<Transform2D>();
        for (Entity e : all) registry.destroyEntity(e);
        registry.flushDestroyed();
        AudioManager::getInstance().playMusic("menu");
      };
      transitionTo(GameState::MAIN_MENU, 0.5f);
    }
    return;

  case GameState::ABILITY_SELECT:
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      selectedAbility =
          (selectedAbility - 1 + (int)abilityChoices.size()) %
          (int)abilityChoices.size();
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      selectedAbility = (selectedAbility + 1) % (int)abilityChoices.size();
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_ENTER) && !abilityChoices.empty()) {
      AudioManager::getInstance().playSFX("menu_confirm");
      transitionCallback = [this]() {
        abilitySystem.grantAbility(registry, playerEntity,
                                   abilityChoices[selectedAbility]);
        synergySystem.evaluate(registry, playerEntity);
        recalculatePlayerStats();

        // If next layer is boss (single node), go directly
        if (runMap.isBossNext()) {
          runMap.advanceTo(0);
          spawnRoomFromNode(runMap.current());
          if (state == GameState::BOSS_INTRO)
            pendingTransition = GameState::BOSS_INTRO;
        } else if (!runMap.isLastLayer()) {
          // Show map for player to choose next room
          runMap.cursorIndex = 0;
          pendingTransition = GameState::MAP_SELECT;
        }
      };
      transitionTo(GameState::PLAYING, 0.4f);
    }
    return;

  case GameState::BOSS_INTRO:
    bossIntroTimer -= deltaTime;
    if (bossIntroTimer <= 0.0f) {
      state = GameState::PLAYING;
    }
    return;

  case GameState::MAP_SELECT: {
    auto choices = runMap.nextChoices();
    if (choices.empty()) return;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
      runMap.cursorIndex = (runMap.cursorIndex - 1 + (int)choices.size()) % (int)choices.size();
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
      runMap.cursorIndex = (runMap.cursorIndex + 1) % (int)choices.size();
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_ENTER)) {
      AudioManager::getInstance().playSFX("menu_confirm");
      int chosen = choices[runMap.cursorIndex];
      transitionCallback = [this, chosen]() {
        runMap.advanceTo(chosen);
        spawnRoomFromNode(runMap.current());
        // If spawnRoomFromNode set a special state, mirror it
        if (state == GameState::BOSS_INTRO || state == GameState::SHOP ||
            state == GameState::REST)
          pendingTransition = state;
      };
      transitionTo(GameState::PLAYING, 0.4f);
    }
    return;
  }

  case GameState::SHOP: {
    // Shop: offer 3 abilities for HP cost
    if (!shopInitialized) {
      shopItems = abilitySystem.getRandomChoices(3);
      selectedAbility = 0;
      shopInitialized = true;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
      selectedAbility = (selectedAbility - 1 + (int)shopItems.size()) % (int)shopItems.size();
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      selectedAbility = (selectedAbility + 1) % (int)shopItems.size();
      AudioManager::getInstance().playSFX("menu_select");
    }
    if (IsKeyPressed(KEY_ENTER) && !shopItems.empty() &&
        registry.hasComponent<Health>(playerEntity)) {
      auto &hp = registry.getComponent<Health>(playerEntity);
      int cost = hp.maxHP / 10; // 10% max HP per purchase
      if (hp.currentHP > cost) {
        hp.currentHP -= cost;
        abilitySystem.grantAbility(registry, playerEntity, shopItems[selectedAbility]);
        synergySystem.evaluate(registry, playerEntity);
        recalculatePlayerStats();
        shopItems.erase(shopItems.begin() + selectedAbility);
        if (selectedAbility >= (int)shopItems.size())
          selectedAbility = std::max(0, (int)shopItems.size() - 1);
        AudioManager::getInstance().playSFX("menu_confirm");
      } else {
        AudioManager::getInstance().playSFX("hit_player"); // not enough HP
      }
    }
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E)) {
      shopInitialized = false;
      // Go to map select or boss if last
      if (runMap.isBossNext()) {
        transitionCallback = [this]() {
          runMap.advanceTo(0);
          spawnRoomFromNode(runMap.current());
          if (state == GameState::BOSS_INTRO)
            pendingTransition = GameState::BOSS_INTRO;
        };
        transitionTo(GameState::PLAYING, 0.4f);
      } else if (!runMap.isLastLayer()) {
        runMap.cursorIndex = 0;
        state = GameState::MAP_SELECT;
      }
    }
    return;
  }

  case GameState::REST:
    if (IsKeyPressed(KEY_ENTER)) {
      // Heal 30% max HP
      if (registry.hasComponent<Health>(playerEntity)) {
        auto &hp = registry.getComponent<Health>(playerEntity);
        int heal = hp.maxHP * 3 / 10;
        hp.currentHP = std::min(hp.currentHP + heal, hp.maxHP);
        AudioManager::getInstance().playSFX("menu_confirm");
        screenEffects.addFlash(Color{220, 180, 100, 80}, 0.3f); // amber heal flash (paleta Circulo VII)
      }
      // Advance
      if (runMap.isBossNext()) {
        transitionCallback = [this]() {
          runMap.advanceTo(0);
          spawnRoomFromNode(runMap.current());
          if (state == GameState::BOSS_INTRO)
            pendingTransition = GameState::BOSS_INTRO;
        };
        transitionTo(GameState::PLAYING, 0.4f);
      } else if (!runMap.isLastLayer()) {
        runMap.cursorIndex = 0;
        state = GameState::MAP_SELECT;
      }
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
      // Skip rest, go to map
      if (runMap.isBossNext()) {
        transitionCallback = [this]() {
          runMap.advanceTo(0);
          spawnRoomFromNode(runMap.current());
          if (state == GameState::BOSS_INTRO)
            pendingTransition = GameState::BOSS_INTRO;
        };
        transitionTo(GameState::PLAYING, 0.4f);
      } else if (!runMap.isLastLayer()) {
        runMap.cursorIndex = 0;
        state = GameState::MAP_SELECT;
      }
    }
    return;

  case GameState::INVENTORY:
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_I))
      state = GameState::PLAYING;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
      inventorySelectedSlot = std::max(0, inventorySelectedSlot - 1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      if (registry.hasComponent<ItemHolder>(playerEntity)) {
        auto &ih = registry.getComponent<ItemHolder>(playerEntity);
        int maxSlot = ih.equippedItems.empty() ? 0 : (int)ih.equippedItems.size() - 1;
        inventorySelectedSlot = std::min(maxSlot, inventorySelectedSlot + 1);
      }
    }
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X)) {
      if (registry.hasComponent<ItemHolder>(playerEntity)) {
        auto &ih = registry.getComponent<ItemHolder>(playerEntity);
        if (inventorySelectedSlot >= 0 &&
            inventorySelectedSlot < (int)ih.equippedItems.size()) {
          ih.removeItem(inventorySelectedSlot);
          if (inventorySelectedSlot >= (int)ih.equippedItems.size())
            inventorySelectedSlot = std::max(0, (int)ih.equippedItems.size() - 1);
          recalculatePlayerStats();
          synergySystem.evaluate(registry, playerEntity);
        }
      }
    }
    return;

  case GameState::INFO:
    if (IsKeyPressed(KEY_ESCAPE) || 
        inputManager.isActionPressed(InputAction::OPEN_INFO) || 
        inputManager.isActionPressed(InputAction::OPEN_INVENTORY) || 
        inputManager.isActionPressed(InputAction::OPEN_ABILITIES))
      state = GameState::PLAYING;
    
    // Allow swapping tabs inside info menu
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) infoMenuTab = (infoMenuTab + 1) % 3;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) infoMenuTab = (infoMenuTab + 2) % 3;
    return;

  case GameState::ITEM_SWAP: {
    if (IsKeyPressed(KEY_ESCAPE)) {
      state = GameState::PLAYING;
      return;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
      inventorySelectedSlot = std::max(0, inventorySelectedSlot - 1);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
      if (registry.hasComponent<ItemHolder>(playerEntity)) {
        auto &ih = registry.getComponent<ItemHolder>(playerEntity);
        int maxSlot = ih.equippedItems.empty() ? 0 : (int)ih.equippedItems.size() - 1;
        inventorySelectedSlot = std::min(maxSlot, inventorySelectedSlot + 1);
      }
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (registry.hasComponent<ItemHolder>(playerEntity)) {
        auto &ih = registry.getComponent<ItemHolder>(playerEntity);
        if (inventorySelectedSlot >= 0 &&
            inventorySelectedSlot < (int)ih.equippedItems.size()) {
          ih.equippedItems[inventorySelectedSlot] = pendingItem;
          recalculatePlayerStats();
          synergySystem.evaluate(registry, playerEntity);
        }
      }
      state = GameState::PLAYING;
    }
    return;
  }

  case GameState::PLAYING:
    break;

  default:
    break;
  }

  // --- PLAYING state logic ---

  // Special attack cooldown
  if (specialCooldownTimer > 0.0f)
    specialCooldownTimer -= deltaTime;

  // Hitstop check — freeze gameplay briefly on big hits
  if (screenEffects.update(deltaTime))
    return; // In hitstop — skip gameplay update

  if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
    pauseSelected = 0;
    state = GameState::PAUSED;
  }

  // Inventory / Stats toggle
  if (inputManager.isActionPressed(InputAction::OPEN_INVENTORY)) {
    inventorySelectedSlot = 0;
    state = GameState::INVENTORY;
    return;
  }
  if (inputManager.isActionPressed(InputAction::OPEN_INFO)) {
    state = GameState::INFO;
    infoMenuTab = 0;
    return;
  }
  if (inputManager.isActionPressed(InputAction::OPEN_ABILITIES)) {
    state = GameState::INFO;
    infoMenuTab = 1;
    return;
  }

  // Track run time
  saveManager.getCurrentRun().timePlayed += deltaTime;

  // Check player death
  if (registry.hasComponent<Health>(playerEntity) &&
      registry.getComponent<Health>(playerEntity).isDead()) {
    endRun(false);
    screenEffects.addFlash(Color{180, 0, 0, 100}, 0.5f);
    AudioManager::getInstance().playSFX("player_death");
    AudioManager::getInstance().stopMusic();
    transitionTo(GameState::GAME_OVER, 0.6f);
    return;
  }

  handlePlayerInput(deltaTime);
  abilitySystem.update(registry, deltaTime);
  aiSystem.update(registry, deltaTime);
  bossAISystem.update(registry, cameraSystem, deltaTime, playerEntity);
  miniBossAISystem.update(registry, cameraSystem, deltaTime, playerEntity);
  movementSystem.update(registry, deltaTime);

  collisionSystem.update(registry);
  collisionSystem.resolveWallCollisions(registry);
  collisionSystem.enforceBoundaries(registry,
                                    (float)roomGenerator.getPixelWidth(),
                                    (float)roomGenerator.getPixelHeight());

  physicsVFXSystem.update(registry, deltaTime);
  trapSystem.update(registry, deltaTime);
  staminaSystem.update(registry, deltaTime);
  healthSystem.update(registry, deltaTime);
  combatSystem.update(registry, cameraSystem, screenEffects, deltaTime);

  processLootPickups();
  checkRoomClear();

  UIRenderer::updateDamageNumbers(registry, deltaTime);
  particleSystem.update(registry, deltaTime);
  PartikelEmitters::update(deltaTime);
  animationSystem.update(registry, deltaTime);
  cameraSystem.update(registry, playerEntity, deltaTime);
  updateAtmosphericParticles(deltaTime);

  registry.flushDestroyed();
}
