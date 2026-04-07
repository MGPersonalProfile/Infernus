#include "Game.h"
#include "../components/AIBehavior.h"
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

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

// Pixel font is used via TextUtils::draw / TextUtils::measure throughout.

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
  inputManager.init();
  cameraSystem.init(screenWidth, screenHeight);
  AudioManager::getInstance().init();
  AudioManager::getInstance().playMusic("menu");
  abilitySystem.loadAbilities("assets/data/abilities.json");
  synergySystem.loadSynergies("assets/data/synergies.json");
  itemSystem.loadItems("assets/data/items.json");
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
}

void Game::spawnRoom() {
  // Clear existing enemies
  auto enemies = registry.view<AIBehavior>();
  for (Entity e : enemies)
    registry.destroyEntity(e);

  // Clear room geometry
  roomGenerator.clear(registry);
  registry.flushDestroyed();
  roomCleared = false;

  bool isBossRoom = (currentRoom >= totalRooms);

  // Generate and instantiate room layout
  RoomTemplate room = roomGenerator.generate(currentRoom, isBossRoom);
  roomGenerator.instantiate(registry, room);

  // Move player to spawn point
  if (registry.hasComponent<Transform2D>(playerEntity)) {
    auto &pt = registry.getComponent<Transform2D>(playerEntity);
    pt.x = RoomGenerator::tileToWorld(room.playerSpawn.first, room.tileSize);
    pt.y = RoomGenerator::tileToWorld(room.playerSpawn.second, room.tileSize);
  }

  if (!isBossRoom) {
    int numEnemies = (int)room.enemySpawns.size();
    enemiesAlive = numEnemies;

    for (auto &spawn : room.enemySpawns) {
      float ex = RoomGenerator::tileToWorld(spawn.first, room.tileSize);
      float ey = RoomGenerator::tileToWorld(spawn.second, room.tileSize);

      int type = GetRandomValue(0, 4);
      if (type == 0)
        EnemyFactory::create(registry, "melee", ex, ey, playerEntity);
      else if (type == 1)
        EnemyFactory::create(registry, "ranged", ex, ey, playerEntity);
      else if (type == 2)
        EnemyFactory::create(registry, "tank", ex, ey, playerEntity);
      else if (type == 3)
        EnemyFactory::create(registry, "assassin", ex, ey, playerEntity);
      else
        EnemyFactory::create(registry, "bomber", ex, ey, playerEntity);
    }

    // Spawn a mini-boss every 2 rooms (rooms 1, 3, 5...)
    if (currentRoom > 0 && currentRoom % 2 == 1) {
      const char *miniBossIds[] = {"infernal_knight", "soul_archer", "pit_fiend"};
      int mbIdx = (currentRoom / 2) % 3;
      float mbx = RoomGenerator::tileToWorld(room.width / 2, room.tileSize);
      float mby = RoomGenerator::tileToWorld(room.height / 2, room.tileSize);
      MiniBossFactory::create(registry, miniBossIds[mbIdx], mbx, mby, playerEntity);
      enemiesAlive++;
    }
  } else {
    // Boss room
    enemiesAlive = 1;
    float bx = RoomGenerator::tileToWorld(room.width / 2 + 4, room.tileSize);
    float by = RoomGenerator::tileToWorld(room.height / 2, room.tileSize);
    bossEntity = BossFactory::create(registry, "minotaur", bx, by);
    AudioManager::getInstance().playSFX("boss_roar");
    AudioManager::getInstance().playMusic("boss");
    state = GameState::BOSS_INTRO;
  }
}

void Game::checkRoomClear() {
  if (roomCleared)
    return;

  // Count alive enemies (AIBehavior), mini-bosses, or boss (BossPhase)
  auto enemies = registry.view<AIBehavior>();
  auto bosses = registry.view<BossPhase, Health>();

  int alive = 0;
  for (Entity e : enemies) {
    if (registry.hasComponent<Health>(e) &&
        !registry.getComponent<Health>(e).isDead())
      alive++;
  }
  for (Entity e : bosses) {
    if (!registry.getComponent<Health>(e).isDead())
      alive++;
  }

  if (alive == 0) {
    roomCleared = true;

    saveManager.getCurrentRun().roomsCleared = currentRoom + 1;

    // Check if boss was killed
    if (currentRoom >= totalRooms) {
      endRun(true);
      transitionTo(GameState::VICTORY, 0.6f);
      return;
    }

    // Offer ability selection between rooms with a smooth fade
    transitionCallback = [this]() {
      abilityChoices = abilitySystem.getRandomChoices(3);
      selectedAbility = 0;
    };
    transitionTo(GameState::ABILITY_SELECT, 0.35f);
  }
}

// =============================================================================
// Player Input
// =============================================================================
void Game::handlePlayerInput() {
  if (!registry.isAlive(playerEntity))
    return;
  if (!registry.hasComponent<Velocity>(playerEntity))
    return;

  auto &velocity = registry.getComponent<Velocity>(playerEntity);
  velocity.vx = 0.0f;
  velocity.vy = 0.0f;

  float speed = Constants::PLAYER_SPEED;
  if (registry.hasComponent<PlayerStats>(playerEntity))
    speed = registry.getComponent<PlayerStats>(playerEntity).finalSpeed;

  if (inputManager.isActionDown(InputAction::MOVE_UP))
    velocity.vy -= speed;
  if (inputManager.isActionDown(InputAction::MOVE_DOWN))
    velocity.vy += speed;
  if (inputManager.isActionDown(InputAction::MOVE_LEFT))
    velocity.vx -= speed;
  if (inputManager.isActionDown(InputAction::MOVE_RIGHT))
    velocity.vx += speed;

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

  if (combat.currentState == AttackState::NONE) {
    if (inputManager.isActionPressed(InputAction::ATTACK_LIGHT) &&
        stamina.hasEnough(Constants::LIGHT_ATTACK_STAMINA)) {
      combat.currentState = AttackState::WINDUP;
      combat.stateTimer = Constants::LIGHT_ATTACK_WINDUP * windupMult;
      combat.lastAttackType = AttackType::LIGHT;
      stamina.currentStamina -= Constants::LIGHT_ATTACK_STAMINA;
      stamina.cooldownTimer = stamina.regenDelay;
      AudioManager::getInstance().playSFX("attack_light");
    } else if (inputManager.isActionPressed(InputAction::ATTACK_HEAVY) &&
               stamina.hasEnough(Constants::HEAVY_ATTACK_STAMINA)) {
      combat.currentState = AttackState::WINDUP;
      combat.stateTimer = Constants::HEAVY_ATTACK_WINDUP * windupMult;
      combat.lastAttackType = AttackType::HEAVY;
      stamina.currentStamina -= Constants::HEAVY_ATTACK_STAMINA;
      stamina.cooldownTimer = stamina.regenDelay;
      AudioManager::getInstance().playSFX("attack_heavy");
    }
  }

  if (inputManager.isActionPressed(InputAction::DASH) &&
      stamina.hasEnough(Constants::DASH_STAMINA)) {
    stamina.currentStamina -= Constants::DASH_STAMINA;
    stamina.cooldownTimer = stamina.regenDelay;
    AudioManager::getInstance().playSFX("dash");

    if (registry.hasComponent<Health>(playerEntity))
      registry.getComponent<Health>(playerEntity).invulnerabilityTimer =
          Constants::DASH_IFRAMES;

    if (velocity.vx != 0.0f || velocity.vy != 0.0f) {
      float len = sqrtf(velocity.vx * velocity.vx + velocity.vy * velocity.vy);
      velocity.vx = (velocity.vx / len) * Constants::DASH_SPEED;
      velocity.vy = (velocity.vy / len) * Constants::DASH_SPEED;
    } else {
      bool left = registry.hasComponent<Sprite>(playerEntity) &&
                  registry.getComponent<Sprite>(playerEntity).flipX;
      velocity.vx = left ? -Constants::DASH_SPEED : Constants::DASH_SPEED;
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

      // Dash damage: spawn hitbox at player position
      if (registry.hasComponent<PlayerStats>(playerEntity) &&
          registry.getComponent<PlayerStats>(playerEntity).dashDamage) {
        int dashDmg = combat.baseDamage;
        Entity dashHit = registry.createEntity();
        registry.addComponent<Transform2D>(dashHit, pt.x - 20.0f, pt.y - 20.0f);
        registry.addComponent<Collider>(dashHit, 40.0f, 40.0f);
        registry.addComponent<Combat>(dashHit, dashDmg, 150.0f, playerEntity);
        registry.addComponent<Lifetime>(dashHit, 0.2f);
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

    // Spawn damage hitboxes along the path
    for (int i = 1; i <= 3; i++) {
      float d = teleportDist * ((float)i / 3.0f);
      Entity hb = registry.createEntity();
      registry.addComponent<Transform2D>(hb, pt.x + fx * d - 15,
                                         pt.y + fy * d - 15);
      registry.addComponent<Collider>(hb, 30.0f, 30.0f);
      registry.addComponent<Combat>(hb, combat.baseDamage * 3,
                                    100.0f, playerEntity);
      registry.addComponent<Lifetime>(hb, 0.15f);
    }

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

// =============================================================================
// Loot Pickups
// =============================================================================
void Game::processLootPickups() {
  if (!registry.hasComponent<Collider>(playerEntity))
    return;

  auto &playerC = registry.getComponent<Collider>(playerEntity);

  auto lootView = registry.view<Loot, Transform2D, Collider>();
  for (Entity lootEntity : lootView) {
    if (registry.hasComponent<AIBehavior>(lootEntity) ||
        registry.hasComponent<BossPhase>(lootEntity))
      continue;

    auto &lootC = registry.getComponent<Collider>(lootEntity);

    bool overlaps = CheckCollisionRecs(playerC.rect, lootC.rect);
    if (!overlaps)
      continue;

    auto &loot = registry.getComponent<Loot>(lootEntity);
    if (loot.type == LootType::HEALTH_ORB &&
        registry.hasComponent<Health>(playerEntity)) {
      auto &h = registry.getComponent<Health>(playerEntity);
      h.currentHP = std::min(h.currentHP + loot.value, h.maxHP);
      AudioManager::getInstance().playSFX("pickup_health");
    } else if (loot.type == LootType::STAMINA_ORB &&
               registry.hasComponent<Stamina>(playerEntity)) {
      auto &s = registry.getComponent<Stamina>(playerEntity);
      s.currentStamina =
          std::min(s.currentStamina + (float)loot.value, s.maxStamina);
      AudioManager::getInstance().playSFX("pickup_stamina");
    }
    registry.markForDestruction(lootEntity);
  }

  // Item pickups
  auto itemView = registry.view<ItemPickup, Transform2D, Collider>();
  for (Entity itemEntity : itemView) {
    auto &itemC = registry.getComponent<Collider>(itemEntity);

    bool overlaps = CheckCollisionRecs(playerC.rect, itemC.rect);
    if (!overlaps)
      continue;

    auto &pickup = registry.getComponent<ItemPickup>(itemEntity);
    // Assign a random item if the pickup doesn't have one
    if (pickup.item.id.empty())
      pickup.item = itemSystem.getRandomItem(currentRoom);

    if (registry.hasComponent<ItemHolder>(playerEntity)) {
      auto &holder = registry.getComponent<ItemHolder>(playerEntity);
      if (!holder.isFull()) {
        holder.addItem(pickup.item);
        recalculatePlayerStats();
        synergySystem.evaluate(registry, playerEntity);
        AudioManager::getInstance().playSFX("menu_confirm");
      } else {
        // Inventory full — go to swap screen
        pendingItem = pickup.item;
        inventorySelectedSlot = 0;
        state = GameState::ITEM_SWAP;
      }
    }
    registry.markForDestruction(itemEntity);
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

void Game::update(float deltaTime) {
  AudioManager::getInstance().update();

  // Update fade timer globally (so transitions work in ALL states)
  screenEffects.updateFade(deltaTime);

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
        currentRoom++;
        spawnRoom();
        // spawnRoom may set state to BOSS_INTRO; mirror it into pendingTransition
        if (state == GameState::BOSS_INTRO)
          pendingTransition = GameState::BOSS_INTRO;
      };
      transitionTo(GameState::PLAYING, 0.4f);
    }
    return;

  case GameState::BOSS_INTRO: {
    static float introTimer = 2.0f;
    introTimer -= deltaTime;
    if (introTimer <= 0.0f) {
      introTimer = 2.0f;
      state = GameState::PLAYING;
    }
    return;
  }

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

  case GameState::STATS_VIEW:
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_TAB))
      state = GameState::PLAYING;
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
  if (inputManager.isActionPressed(InputAction::OPEN_STATS)) {
    state = GameState::STATS_VIEW;
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

  handlePlayerInput();
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
  animationSystem.update(registry, deltaTime);
  cameraSystem.update(registry, playerEntity, deltaTime);
  updateAtmosphericParticles(deltaTime);

  registry.flushDestroyed();
}

// =============================================================================
// Render
// =============================================================================
void Game::render() {
  BeginDrawing();
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
  case GameState::STATS_VIEW:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawStatsWindow();
    break;
  case GameState::ITEM_SWAP:
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    EndMode2D();
    drawItemSwap();
    break;
  case GameState::PLAYING:
  default: {
    // Parallax dungeon background (screen-space, drawn BEFORE world geometry)
    {
      Texture2D parallaxTex = ResourceManager::getInstance().getTexture(
          "assets/art/parallax_dungeon.png");
      if (parallaxTex.id > 0) {
        float scaleX = (float)screenWidth / parallaxTex.width;
        float scaleY = (float)screenHeight / parallaxTex.height;
        float scale = std::max(scaleX, scaleY);
        float drawW = parallaxTex.width * scale;
        float drawH = parallaxTex.height * scale;
        // Parallax offset based on camera position
        float offsetX = -fmodf(cameraSystem.camera.target.x * 0.15f, drawW);
        float offsetY = -fmodf(cameraSystem.camera.target.y * 0.1f, drawH);
        DrawTexturePro(
            parallaxTex,
            {0, 0, (float)parallaxTex.width, (float)parallaxTex.height},
            {offsetX, offsetY, drawW, drawH}, {0, 0}, 0.0f,
            Color{180, 160, 180, 255});
        // Tile horizontally if needed
        if (offsetX + drawW < screenWidth) {
          DrawTexturePro(
              parallaxTex,
              {0, 0, (float)parallaxTex.width, (float)parallaxTex.height},
              {offsetX + drawW, offsetY, drawW, drawH}, {0, 0}, 0.0f,
              Color{180, 160, 180, 255});
        }
        // Dark overlay so tiles are still readable on top
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 100});
      }
    }
    BeginMode2D(cameraSystem.camera);
    renderSystem.update(registry);
    renderAtmosphericParticles();
    if (damageNumbersEnabled) UIRenderer::renderDamageNumbers(registry);
    EndMode2D();
    drawHUD();
    if (bossEntity != NULL_ENTITY && registry.isAlive(bossEntity))
      drawBossHealthBar();
    drawMiniBossHealthBar();
    break;
  }
  }

  // Screen effects overlay (vignette, flash, fade)
  screenEffects.render(screenWidth, screenHeight);
  if (showDebug && state == GameState::PLAYING) drawDebugOverlay();

  EndDrawing();
}

// =============================================================================
// HUD
// =============================================================================
void Game::drawHUD() {
  if (!registry.isAlive(playerEntity))
    return;

  if (registry.hasComponent<Health>(playerEntity) &&
      registry.hasComponent<Stamina>(playerEntity)) {
    auto &health = registry.getComponent<Health>(playerEntity);
    auto &stam = registry.getComponent<Stamina>(playerEntity);

    // HP Bar (dark bg, red fill with bright top edge, gold border)
    float hpRatio = std::max(0.0f, (float)health.currentHP / health.maxHP);
    DrawRectangle(20, 20, 300, 22, Color{40, 5, 5, 220});
    int hpFill = (int)(300 * hpRatio);
    DrawRectangleGradientV(20, 20, hpFill, 11, Color{220, 60, 40, 255},
                           Color{160, 25, 20, 255});
    DrawRectangleGradientV(20, 31, hpFill, 11, Color{140, 20, 15, 255},
                           Color{100, 10, 10, 255});
    DrawRectangleLinesEx({20, 20, 300, 22}, 1.0f, Color{180, 150, 80, 200});
    TextUtils::draw(TextFormat("HP %d/%d", health.currentHP, health.maxHP), 25, 24, 12,
             WHITE);

    // Stamina Bar (dark bg, green fill, gold border)
    float stRatio = stam.currentStamina / stam.maxStamina;
    DrawRectangle(20, 48, 200, 14, Color{5, 25, 5, 220});
    int stFill = (int)(200 * stRatio);
    DrawRectangleGradientV(20, 48, stFill, 7, Color{50, 200, 50, 255},
                           Color{30, 140, 30, 255});
    DrawRectangleGradientV(20, 55, stFill, 7, Color{25, 120, 25, 255},
                           Color{15, 80, 15, 255});
    DrawRectangleLinesEx({20, 48, 200, 14}, 1.0f, Color{150, 130, 70, 180});
  }

  TextUtils::draw(TextFormat("Sala %d/%d", currentRoom + 1, totalRooms + 1), 20, 68,
           12, Color{200, 200, 200, 255});

  // Ability icons
  if (registry.hasComponent<AbilityHolder>(playerEntity)) {
    auto &holder = registry.getComponent<AbilityHolder>(playerEntity);
    UIRenderer::drawAbilityIcons(holder, 20, 90);
  }

  // Right-side HUD: synergies on top, then counters below
  int rhsX = screenWidth - 200;
  int rhsY = 20;

  // Synergy indicators
  UIRenderer::drawSynergyBar(synergySystem, rhsX, rhsY);

  // Calculate where synergies end (each is 16px tall)
  int synergyCount = (int)synergySystem.getStates().size();
  int afterSynergies = rhsY + synergyCount * 16 + 6;

  // Ability count indicator
  if (registry.hasComponent<AbilityHolder>(playerEntity)) {
    auto &ah = registry.getComponent<AbilityHolder>(playerEntity);
    if (!ah.abilities.empty()) {
      TextUtils::draw(TextFormat("Hab:%d [H]", (int)ah.abilities.size()),
               rhsX, afterSynergies, 10, Color{180, 200, 100, 255});
      afterSynergies += 14;
    }
  }

  // Item count indicator
  if (registry.hasComponent<ItemHolder>(playerEntity)) {
    auto &ih = registry.getComponent<ItemHolder>(playerEntity);
    if (!ih.equippedItems.empty()) {
      TextUtils::draw(TextFormat("Items:%d/%d [I]", (int)ih.equippedItems.size(),
                          ih.maxItems),
               rhsX, afterSynergies, 10, Color{200, 180, 100, 255});
      afterSynergies += 14;
    }
  }

  // Active synergy count
  {
    auto &states = synergySystem.getStates();
    int activeCount = 0;
    for (auto &s : states) if (s.active) activeCount++;
    if (activeCount > 0)
      TextUtils::draw(TextFormat("Sinergias:%d", activeCount),
               rhsX, afterSynergies, 10, Color{100, 255, 100, 200});
  }

  // Special attack cooldown indicator
  {
    const char *specialName = "Especial [L]";
    if (currentCharacterId == "warrior")
      specialName = "Sismico [L]";
    else if (currentCharacterId == "rogue")
      specialName = "Sombra [L]";
    else if (currentCharacterId == "knight")
      specialName = "Escudo [L]";

    Color specCol = (specialCooldownTimer <= 0.0f)
                        ? Color{220, 180, 100, 255}
                        : Color{80, 80, 80, 255};
    TextUtils::draw(specialName, 20, screenHeight - 44, 12, specCol);
    if (specialCooldownTimer > 0.0f)
      TextUtils::draw(TextFormat("%.1fs", specialCooldownTimer), 150, screenHeight - 44,
               12, Color{180, 80, 80, 255});
  }

  // Controls hint — shorter to fit pixel font
  TextUtils::draw("WASD:Move J/K:Atk L:Spec SPACE:Dash I:Items", 20,
           screenHeight - 24, 10, Color{120, 120, 120, 255});
  DrawFPS(screenWidth - 90, 4);
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
  int nameW = TextUtils::measure(name, 14);
  TextUtils::draw(name, (screenWidth - nameW) / 2 + 1, barY - 22, 14,
           Color{0, 0, 0, 180});
  TextUtils::draw(name, (screenWidth - nameW) / 2, barY - 23, 14,
           Color{230, 190, 100, 255});

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

  // Title with shadow
  int ty = screenHeight / 4 - 20;
  TextUtils::drawCenteredShadow("INFERNUS", ty, 48,
           Color{200, 40, 20, 255}, screenWidth);

  // Subtitle
  TextUtils::drawCentered("\"Abandonad toda esperanza\"",
           ty + 60, 10, Color{180, 150, 100, 255}, screenWidth);

  // Menu items with glow effect
  int menuY = screenHeight / 2 + 40;
  float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
  unsigned char alpha = (unsigned char)(180 + 75 * pulse);
  TextUtils::drawCenteredShadow("[ENTER] Comenzar", menuY, 18,
           Color{255, 220, 150, alpha}, screenWidth);
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

  TextUtils::drawCenteredShadow("PAUSA", py + 25, 28, Color{220, 200, 150, 255}, screenWidth);

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

    TextUtils::drawCentered(items[i], y + 4, fontSize, col, screenWidth);

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

  int mainW = 650, mainH = 520;
  int mainX = (screenWidth - mainW) / 2, mainY = 20;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{12, 12, 22, 230},
              Color{180, 140, 60, 255});

  TextUtils::drawCenteredShadow("OPCIONES", 40, 24, Color{220, 180, 100, 255}, screenWidth);

  auto &audio = AudioManager::getInstance();
  int cx = screenWidth / 2;
  int startY = 85;
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
      DrawRectangle(cx - 260, y - 5, 520, 34, Color{30, 30, 50, 255});
      DrawRectangleLinesEx({(float)(cx - 260), (float)(y - 5), 520, 34}, 1.0f,
                           Color{220, 180, 100, 120});
    }

    if (i < 2) {
      // Volume sliders
      const char *label = (i == 0) ? "Volumen SFX" : "Volumen Musica";
      float vol = (i == 0) ? audio.getSFXVolume() : audio.getMusicVolume();
      TextUtils::draw(label, cx - 240, y + 5, 10, labelCol);

      int barX = cx + 40;
      int barW = 140;
      int barY = y + 7;
      DrawRectangle(barX, barY, barW, 12, Color{40, 40, 50, 255});
      int fillW = (int)(vol * barW);
      DrawRectangle(barX, barY, fillW, 12, sel ? Color{220, 180, 100, 255} : Color{120, 120, 140, 255});
      DrawRectangleLines(barX, barY, barW, 12, Color{80, 80, 100, 255});
      TextUtils::draw(TextFormat("%d%%", (int)(vol * 100)),
               barX + barW + 10, y + 5, 10, valCol);
      if (sel) {
        TextUtils::draw("<", barX - 14, y + 5, 10, Color{220, 180, 100, 255});
        TextUtils::draw(">", barX + barW + 55, y + 5, 10, Color{220, 180, 100, 255});
      }
    } else if (i == 2) {
      TextUtils::draw("Pantalla Completa", cx - 240, y + 5, 10, labelCol);
      const char *val = IsWindowFullscreen() ? "SI" : "NO";
      TextUtils::draw(val, cx + 100, y + 5, 10, valCol);
    } else if (i == 3) {
      TextUtils::draw("Screen Shake", cx - 240, y + 5, 10, labelCol);
      const char *val = screenShakeEnabled ? "SI" : "NO";
      TextUtils::draw(val, cx + 100, y + 5, 10, val[0] == 'S' ? Color{100, 255, 100, 255} : Color{255, 80, 80, 255});
    } else if (i == 4) {
      TextUtils::draw("Numeros de Dano", cx - 240, y + 5, 10, labelCol);
      const char *val = damageNumbersEnabled ? "SI" : "NO";
      TextUtils::draw(val, cx + 100, y + 5, 10, val[0] == 'S' ? Color{100, 255, 100, 255} : Color{255, 80, 80, 255});
    } else {
      TextUtils::drawCentered("< Volver >", y + 5, 12, labelCol, screenWidth);
    }
  }

  // Keybinds reference
  int kbY = startY + optionCount * rowH + 15;
  DrawRectangle(cx - 260, kbY, 520, 1, Color{60, 50, 40, 200});
  TextUtils::drawCentered("CONTROLES", kbY + 8, 10, Color{180, 150, 100, 200}, screenWidth);
  int ky = kbY + 26;
  Color kc = Color{120, 120, 130, 200};
  TextUtils::draw("WASD  Movimiento", cx - 220, ky, 8, kc);
  TextUtils::draw("J     Ataque ligero", cx + 30, ky, 8, kc);
  ky += 14;
  TextUtils::draw("K     Ataque pesado", cx - 220, ky, 8, kc);
  TextUtils::draw("L     Especial", cx + 30, ky, 8, kc);
  ky += 14;
  TextUtils::draw("SPACE Dash", cx - 220, ky, 8, kc);
  TextUtils::draw("I     Inventario", cx + 30, ky, 8, kc);
  ky += 14;
  TextUtils::draw("TAB   Estadisticas", cx - 220, ky, 8, kc);
  TextUtils::draw("H     Habilidades", cx + 30, ky, 8, kc);

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
                   {0, 0}, 0.0f, Color{100, 40, 40, 255});
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 180});
  }

  // Pulsing title
  float pulse = 0.6f + 0.4f * sinf((float)GetTime() * 2.0f);
  unsigned char titleAlpha = (unsigned char)(180 + 75 * pulse);
  TextUtils::drawCenteredShadow("HAS MUERTO", screenHeight / 6, 36,
           Color{200, 30, 20, titleAlpha}, screenWidth);

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
  TextUtils::drawCenteredShadow("[ ENTER ] Reintentar", screenHeight - 85, 12,
           Color{220, 180, 100, enterAlpha}, screenWidth);
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
  TextUtils::drawCenteredShadow("VICTORIA", screenHeight / 8, 36,
           Color{255, 210, 60, titleAlpha}, screenWidth);

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
  TextUtils::drawCenteredShadow("[ ENTER ] Menu Principal", screenHeight - 70, 12,
           Color{255, 220, 140, alpha}, screenWidth);
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
    TextUtils::drawCenteredShadow(name, screenHeight / 2 + 145, 28,
             Color{220, 180, 100, 255}, screenWidth);
  }

  float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 2.5f);
  unsigned char alpha = (unsigned char)(120 + 135 * pulse);
  TextUtils::drawCentered("Preparate...", screenHeight / 2 + 190, 14,
           Color{180, 160, 140, alpha}, screenWidth);
}

void Game::startGame(int characterIndex) {
  saveManager.resetCurrentRun();
  using json = nlohmann::json;

  // Load character data
  std::ifstream file("assets/data/characters.json");
  json data;
  int hp = 100, damage = 15;
  float spd = 250.0f, stam = 100.0f, stamRegen = 20.0f;
  std::string charId = "warrior";

  if (file.is_open()) {
    file >> data;
    auto &chars = data["characters"];
    if (characterIndex < (int)chars.size()) {
      auto &c = chars[characterIndex];
      hp = c.value("hp", 100);
      damage = c.value("damage", 15);
      spd = c.value("speed", 250.0f);
      stam = c.value("stamina", 100.0f);
      stamRegen = c.value("staminaRegen", 20.0f);
      charId = c.value("id", "warrior");
    }
  }

  currentCharacterId = charId;

  // Create player with chosen stats
  playerEntity = registry.createEntity();
  registry.addComponent<Transform2D>(playerEntity, screenWidth / 2.0f,
                                     screenHeight / 2.0f);

  // Load class-specific sprite
  std::string spritePath = "assets/sprites/player/" + charId + "_idle.png";
  Texture2D tex = ResourceManager::getInstance().getTexture(spritePath);
  registry.addComponent<Sprite>(playerEntity, tex,
                                Rectangle{0, 0, 48, 56}, 10);
  registry.addComponent<Animation>(playerEntity, 2, 0.4f, 48.0f, 56.0f);
  registry.addComponent<Velocity>(playerEntity, 0.0f, 0.0f);
  registry.addComponent<Collider>(playerEntity, 36.0f, 50.0f, false);
  registry.addComponent<Health>(playerEntity, hp);
  registry.addComponent<Stamina>(playerEntity, stam, stamRegen, 1.0f);
  registry.addComponent<Combat>(playerEntity, damage, 150.0f);
  registry.addComponent<AbilityHolder>(playerEntity);
  registry.addComponent<ItemHolder>(playerEntity);

  // Set up PlayerStats with base values
  auto &pstats = registry.addComponent<PlayerStats>(playerEntity);
  pstats.baseMaxHP = hp;
  pstats.baseDamage = damage;
  pstats.baseSpeed = spd;
  pstats.baseMaxStamina = stam;
  pstats.baseStaminaRegen = stamRegen;
  pstats.classId = charId;

  // Apply speed via AbilityHolder base multiplier
  registry.getComponent<AbilityHolder>(playerEntity).speedMultiplier =
      spd / Constants::PLAYER_SPEED;

  // Grant starting item from character data
  if (file.is_open()) {
    // Re-read since stream was consumed
    std::ifstream file2("assets/data/characters.json");
    if (file2.is_open()) {
      json data2;
      file2 >> data2;
      auto &chars2 = data2["characters"];
      if (characterIndex < (int)chars2.size()) {
        std::string startItemId = chars2[characterIndex].value("startItemId", "");
        if (!startItemId.empty()) {
          ItemData startItem = itemSystem.getItemById(startItemId);
          if (!startItem.id.empty())
            itemSystem.grantItem(registry, playerEntity, startItem);
        }
      }
    }
  }

  currentRoom = 0;
  bossEntity = NULL_ENTITY;
  specialCooldownTimer = 0.0f;
  state = GameState::PLAYING;
  screenEffects.startFadeIn(0.5f);
  AudioManager::getInstance().playSFX("game_start");
  AudioManager::getInstance().playMusic("circle_7");
  spawnRoom();
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
  TextUtils::draw(title, (screenWidth - tw) / 2 + 2, screenHeight / 10 + 2, titleSize,
           Color{0, 0, 0, 180});
  TextUtils::draw(title, (screenWidth - tw) / 2, screenHeight / 10, titleSize,
           Color{220, 180, 100, 255});

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
  int innerPad = 10; // Padding inside card

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

    int textY = cy + innerPad + 180 + 8; // right below portrait

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
    textY += nameSize + 4;

    // Stats — 2 rows, colored values
    int statSize = 10;
    const char *statLine1 = TextFormat("HP:%d  DMG:%d",
        c.value("hp", 100), c.value("damage", 15));
    const char *statLine2 = TextFormat("SPD:%.0f STA:%.0f",
        c.value("speed", 250.0f), c.value("stamina", 100.0f));
    // Auto-shrink stats if needed
    int sfs = statSize;
    while (TextUtils::measure(statLine1, sfs) > contentW && sfs > 6) sfs--;
    TextUtils::draw(statLine1, cx + innerPad, textY, sfs,
             Color{120, 200, 120, 255});
    textY += sfs + 3;
    TextUtils::draw(statLine2, cx + innerPad, textY, sfs,
             Color{120, 200, 120, 255});
    textY += sfs + 5;

    // Description — single line, truncate cleanly with "..."
    int descSize = 8;
    std::string desc = c.value("description", "");
    while (TextUtils::measure(desc.c_str(), descSize) > contentW && desc.size() > 4) {
      desc.pop_back();
    }
    // If we truncated, trim trailing spaces and add ellipsis
    if (desc.size() < c.value("description", std::string("")).size()) {
      while (!desc.empty() && desc.back() == ' ') desc.pop_back();
      desc += "...";
    }
    TextUtils::draw(desc.c_str(), cx + innerPad, textY, descSize,
             Color{150, 150, 150, 255});
    textY += descSize + 4;

    // Special ability — single line, truncate cleanly
    std::string special = c.value("special", "");
    while (TextUtils::measure(special.c_str(), descSize) > contentW && special.size() > 4) {
      special.pop_back();
    }
    if (special.size() < c.value("special", std::string("")).size()) {
      while (!special.empty() && special.back() == ' ') special.pop_back();
      special += "...";
    }
    if (!special.empty()) {
      TextUtils::draw(special.c_str(), cx + innerPad, textY, descSize,
               Color{220, 180, 100, 200});
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
  int mainX = (screenWidth - mainW) / 2, mainY = 30;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{12, 12, 22, 230},
              Color{160, 130, 60, 255});

  TextUtils::drawCenteredShadow("ELIGE UNA HABILIDAD", 40, 22,
           Color{220, 180, 100, 255}, screenWidth);

  int cardW = 500, cardH = 100;
  int startY = 100;

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
      TextUtils::draw(">", cardX - 20, cardY + cardH / 2 - 8, 16,
               Color{220, 180, 100, 255});

    // Name + rarity tag
    std::string nameStr = TextUtils::truncate(ab.name, 14, contentW - 80);
    TextUtils::draw(nameStr.c_str(), cardX + 15, cardY + 10, 14, rarityCol);
    int rw = TextUtils::measure(rarityText, 10);
    TextUtils::draw(rarityText, cardX + cardW - rw - 15, cardY + 12, 10, rarityCol);

    // Description — truncate to fit
    std::string descStr = TextUtils::truncate(ab.description, 10, contentW);
    TextUtils::draw(descStr.c_str(), cardX + 15, cardY + 35, 10, WHITE);

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
      Color col = synStates[i].active ? Color{100, 255, 100, 255}
                                      : Color{80, 80, 80, 255};
      TextUtils::draw(synDefs[i].name.c_str(), 15, sy, 10, col);
      sy += 14;
    }
  }
}

void Game::endRun(bool victory) {
  auto &run = saveManager.getCurrentRun();
  run.totalRooms = totalRooms + 1;
  run.bossDefeated = victory;

  if (registry.hasComponent<AbilityHolder>(playerEntity))
    run.abilitiesCollected =
        (int)registry.getComponent<AbilityHolder>(playerEntity)
            .abilities.size();

  saveManager.recordRun(run);
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
// Stats Window (TAB key)
// =============================================================================
void Game::drawStatsWindow() {
  DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 200});

  // Panel background
  int mainW = 500, mainH = 520;
  int mainX = (screenWidth - mainW) / 2, mainY = 25;
  drawUIPanel(mainX, mainY, mainW, mainH, Color{15, 15, 25, 230},
              Color{180, 140, 60, 255});

  const char *title = "ESTADISTICAS";
  TextUtils::drawCenteredShadow(title, 38, 24, Color{220, 180, 100, 255}, screenWidth);

  if (!registry.hasComponent<PlayerStats>(playerEntity))
    return;
  auto &ps = registry.getComponent<PlayerStats>(playerEntity);

  int cx = screenWidth / 2 - 180;
  int sy = 90;
  int rowH = 22;

  auto drawStat = [&](const char *label, const char *value, Color col) {
    TextUtils::draw(label, cx, sy, 10, Color{180, 180, 180, 255});
    TextUtils::draw(value, cx + 180, sy, 10, col);
    sy += rowH;
  };

  drawStat("Clase:", ps.classId.c_str(), Color{220, 180, 100, 255});
  sy += 10;
  drawStat("HP Max:", TextFormat("%d (base %d)", ps.finalMaxHP, ps.baseMaxHP),
           Color{180, 60, 60, 255});
  drawStat("Dano:", TextFormat("%d (base %d)", ps.finalDamage, ps.baseDamage),
           Color{200, 160, 60, 255});
  drawStat("Velocidad:", TextFormat("%.0f (base %.0f)", ps.finalSpeed, ps.baseSpeed),
           Color{60, 180, 60, 255});
  drawStat("Stamina Max:", TextFormat("%.0f", ps.finalMaxStamina),
           Color{60, 160, 200, 255});
  drawStat("Regen Stamina:", TextFormat("%.1f", ps.finalStaminaRegen),
           Color{60, 160, 200, 255});
  sy += 10;
  drawStat("Crit:", TextFormat("%.0f%%", ps.finalCritChance * 100.0f),
           Color{255, 200, 60, 255});
  drawStat("Robo de Vida:", TextFormat("%.0f%%", ps.finalLifesteal * 100.0f),
           Color{180, 255, 180, 255});
  drawStat("Espinas:", TextFormat("%.0f%%", ps.finalThorns * 100.0f),
           Color{200, 120, 80, 255});
  sy += 10;

  // Attack modifiers
  if (ps.extraHitboxes > 0)
    drawStat("Multi-golpe:", TextFormat("+%d", ps.extraHitboxes),
             Color{255, 180, 100, 255});
  if (ps.projectileAttack)
    drawStat("Proyectil:", "Activo", Color{100, 200, 255, 255});
  if (ps.areaAttack)
    drawStat("Area:", "Activo", Color{255, 100, 100, 255});
  if (ps.chainAttack)
    drawStat("Cadena:", TextFormat("%d rebotes", ps.chainBounces),
             Color{200, 200, 100, 255});
  if (ps.fireTrailChance > 0.0f)
    drawStat("Rastro de Fuego:", TextFormat("%.0f%%", ps.fireTrailChance * 100.0f),
             Color{255, 120, 30, 255});

  TextUtils::drawCentered("[ESC/TAB] Cerrar", screenHeight - 45,
           8, Color{120, 120, 120, 255}, screenWidth);
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

  TextUtils::drawCenteredShadow("INTERCAMBIAR ITEM", 28, 18, Color{255, 200, 80, 255}, screenWidth);

  // Show new item
  int newBoxW = 450, newBoxH = 60;
  int nbx = (screenWidth - newBoxW) / 2;
  int nby = 75;
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

  int panelW = 450, slotH = 45;
  int px = (screenWidth - panelW) / 2;
  int py = 150;

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
// UI Panel — draws a panel with the art texture or a fallback solid bg
// =============================================================================
void Game::drawUIPanel(int x, int y, int w, int h, Color fallbackBg,
                       Color borderCol) {
  Texture2D panelTex = ResourceManager::getInstance().getTexture(
      "assets/art/ui_panel.png");
  if (panelTex.id > 0) {
    // Draw tiled/stretched panel texture
    Rectangle src = {0, 0, (float)panelTex.width, (float)panelTex.height};
    Rectangle dst = {(float)x, (float)y, (float)w, (float)h};
    DrawTexturePro(panelTex, src, dst, {0, 0}, 0.0f,
                   Color{255, 255, 255, fallbackBg.a});
  } else {
    DrawRectangle(x, y, w, h, fallbackBg);
  }
  // Ornate border: outer gold, inner dark
  DrawRectangleLinesEx({(float)x, (float)y, (float)w, (float)h}, 2.0f,
                       borderCol);
  DrawRectangleLinesEx({(float)(x + 3), (float)(y + 3), (float)(w - 6),
                        (float)(h - 6)},
                       1.0f, Color{borderCol.r, borderCol.g, borderCol.b, 80});
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
  for (auto &p : ashParticles) {
    float alpha = (p.life / p.maxLife);
    Color c = p.color;
    c.a = (unsigned char)(c.a * alpha);
    DrawRectangle((int)p.x, (int)p.y, (int)p.size, (int)p.size, c);
  }
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
// Lifecycle
// =============================================================================
void Game::shutdown() {
  saveManager.save();
  AudioManager::getInstance().shutdown();
  ResourceManager::getInstance().unloadAll();
  CloseWindow();
}

void Game::run() {
  init();
  while (!WindowShouldClose()) {
    update(GetFrameTime());
    render();
  }
  shutdown();
}
