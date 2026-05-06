#include "Game.h"
#include "../debug/DebugPanel.h"
#include "../debug/Profiler.h"
#include "../scripting/LuaEngine.h"
#include "../systems/PartikelEmitters.h"
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

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

// Pixel font is used via TextUtils::draw / TextUtils::measure throughout.
static int infoMenuTab = 0;

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
  ashParticles.clear();

  bool isBossRoom = (currentRoom >= totalRooms);

  // Generate and instantiate room layout
  RoomTemplate room = roomGenerator.generate(currentRoom, isBossRoom);
  roomGenerator.instantiate(registry, room);
  collisionSystem.invalidateWallCache();
  // Tell AI system about the new room so enemies can pathfind
  aiSystem.setRoom(&roomGenerator.getCurrentRoom());

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

      // Enemy composition scales with room difficulty
      const char *enemyId = "melee";
      if (currentRoom <= 0) {
        // Sala 0: only melee + ranged (learn the basics)
        enemyId = GetRandomValue(0, 1) == 0 ? "melee" : "ranged";
      } else if (currentRoom == 1) {
        // Sala 1: melee, ranged, bomber
        int r = GetRandomValue(0, 2);
        enemyId = (r == 0) ? "melee" : (r == 1) ? "ranged" : "bomber";
      } else if (currentRoom == 2) {
        // Sala 2: melee, ranged, bomber, assassin
        int r = GetRandomValue(0, 3);
        const char *pool[] = {"melee", "ranged", "bomber", "assassin"};
        enemyId = pool[r];
      } else {
        // Sala 3+: all types including tank
        int r = GetRandomValue(0, 4);
        const char *pool[] = {"melee", "ranged", "tank", "assassin", "bomber"};
        enemyId = pool[r];
      }
      EnemyFactory::create(registry, enemyId, ex, ey, playerEntity);
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
    bossIntroTimer = 2.0f;
    state = GameState::BOSS_INTRO;
  }
}

void Game::spawnRoomFromNode(MapNode &node) {
  currentRoom = node.difficulty;

  switch (node.type) {
  case RoomType::COMBAT:
  case RoomType::ELITE:
    spawnRoom(); // reuse existing combat room generation
    // Elite rooms: buff all enemies
    if (node.type == RoomType::ELITE) {
      auto enemies = registry.view<AIBehavior, Health, Combat>();
      for (Entity e : enemies) {
        auto &h = registry.getComponent<Health>(e);
        auto &c = registry.getComponent<Combat>(e);
        h.maxHP = (int)(h.maxHP * 1.5f);
        h.currentHP = h.maxHP;
        c.baseDamage = (int)(c.baseDamage * 1.3f);
      }
    }
    break;

  case RoomType::SHOP:
    // Generate an empty room, no enemies — shop UI handled by SHOP state
    {
      roomGenerator.clear(registry);
      registry.flushDestroyed();
      collisionSystem.invalidateWallCache();
      RoomTemplate room = roomGenerator.generate(0, false);
      // Clear enemy spawns — shop has no enemies
      room.enemySpawns.clear();
      roomGenerator.instantiate(registry, room);
      collisionSystem.invalidateWallCache();
      if (registry.hasComponent<Transform2D>(playerEntity)) {
        auto &pt = registry.getComponent<Transform2D>(playerEntity);
        pt.x = RoomGenerator::tileToWorld(room.playerSpawn.first, room.tileSize);
        pt.y = RoomGenerator::tileToWorld(room.playerSpawn.second, room.tileSize);
      }
      enemiesAlive = 0;
      roomCleared = true;
      state = GameState::SHOP;
    }
    break;

  case RoomType::REST:
    // Generate an empty room — rest UI handled by REST state
    {
      roomGenerator.clear(registry);
      registry.flushDestroyed();
      collisionSystem.invalidateWallCache();
      RoomTemplate room = roomGenerator.generate(0, false);
      room.enemySpawns.clear();
      roomGenerator.instantiate(registry, room);
      collisionSystem.invalidateWallCache();
      if (registry.hasComponent<Transform2D>(playerEntity)) {
        auto &pt = registry.getComponent<Transform2D>(playerEntity);
        pt.x = RoomGenerator::tileToWorld(room.playerSpawn.first, room.tileSize);
        pt.y = RoomGenerator::tileToWorld(room.playerSpawn.second, room.tileSize);
      }
      enemiesAlive = 0;
      roomCleared = true;
      state = GameState::REST;
    }
    break;

  case RoomType::BOSS:
    spawnRoom(); // uses currentRoom >= totalRooms logic
    break;
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
        screenEffects.addFlash(Color{50, 200, 50, 80}, 0.3f);
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

    // HP Bar (dark bg, red fill with bright top edge, gold border)
    float hpRatio = std::max(0.0f, (float)health.currentHP / health.maxHP);
    DrawRectangle(20, 20, 300, 22, Color{40, 5, 5, 220});
    int hpFill = (int)(300 * hpRatio);
    DrawRectangleGradientV(20, 20, hpFill, 11, Color{220, 60, 40, 255},
                           Color{160, 25, 20, 255});
    DrawRectangleGradientV(20, 31, hpFill, 11, Color{140, 20, 15, 255},
                           Color{100, 10, 10, 255});
    DrawRectangleLinesEx({20, 20, 300, 22}, 1.0f, Color{180, 150, 80, 200});
    TextUtils::drawOutlined(TextFormat("HP %d/%d", health.currentHP, health.maxHP), 25, 23, 12,
             WHITE, 2);

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

  // Special cooldown — only when on cooldown so the screen stays clean.
  if (specialCooldownTimer > 0.0f) {
    TextUtils::drawOutlined(TextFormat("L: %.1fs", specialCooldownTimer), 20, 102, 10,
             Color{220, 100, 100, 255}, 1);
  }

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
      DrawRectangleLinesEx({(float)x, (float)y, (float)slotSize, (float)slotSize},
                           2.0f, has ? Color{180, 140, 70, 255} : Color{60, 50, 40, 255});

      if (!has) {
        // Empty slot — show key label only
        TextUtils::draw(keyLabel, x + slotSize / 2 - 4, y + slotSize / 2 - 8,
                        16, Color{80, 70, 60, 255});
        return;
      }

      // Icon
      Texture2D icon = ResourceManager::getInstance().getTexture(a.iconPath);
      if (icon.id > 0) {
        Rectangle src = {0, 0, (float)icon.width, (float)icon.height};
        Rectangle dst = {(float)(x + 4), (float)(y + 4),
                         (float)(slotSize - 8), (float)(slotSize - 8)};
        DrawTexturePro(icon, src, dst, {0, 0}, 0.0f, WHITE);
      } else {
        // Fallback: name
        TextUtils::draw(a.name.c_str(), x + 4, y + slotSize / 2 - 4, 8,
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

  // Frame sizes per class
  float fw = 48.0f, fh = 56.0f;
  if (charId == "rogue") { fw = 40.0f; fh = 52.0f; }
  else if (charId == "knight") { fw = 52.0f; fh = 60.0f; }

  // Load class-specific sprite
  std::string spriteBase = "assets/sprites/player/" + charId;
  Texture2D tex = ResourceManager::getInstance().getTexture(spriteBase + "_idle.png");
  registry.addComponent<Sprite>(playerEntity, tex,
                                Rectangle{0, 0, fw, fh}, 10);
  registry.addComponent<Animation>(playerEntity, 6, 0.15f, fw, fh);

  // Multi-state animation clips
  auto &animState = registry.addComponent<AnimState>(playerEntity);
  animState.addClip(AnimStateType::IDLE, spriteBase + "_idle.png", 6, 0.15f, fw, fh);
  animState.addClip(AnimStateType::RUN, spriteBase + "_run.png", 8, 0.09f, fw, fh);
  animState.addClip(AnimStateType::ATTACK, spriteBase + "_attack.png", 6, 0.07f, fw, fh, false);

  registry.addComponent<Velocity>(playerEntity, 0.0f, 0.0f);
  registry.addComponent<Collider>(playerEntity, fw * 0.75f, fh * 0.9f, false);
  registry.addComponent<Health>(playerEntity, hp);
  registry.addComponent<Stamina>(playerEntity, stam, stamRegen, 1.0f);
  registry.addComponent<Combat>(playerEntity, damage, 150.0f);
  registry.addComponent<AbilityHolder>(playerEntity);
  registry.addComponent<ItemHolder>(playerEntity);

  // Equip default active abilities (Q/E) for the chosen class
  abilitySystem.equipDefaultActives(registry, playerEntity, charId);

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
  shopInitialized = false;
  ashParticles.clear();
  abilityChoices.clear();
  enemiesAlive = 0;
  roomCleared = false;

  // Generate the run map and spawn the first room
  runMap.generate();
  state = GameState::PLAYING;
  screenEffects.startFadeIn(0.5f);
  AudioManager::getInstance().playSFX("game_start");
  AudioManager::getInstance().playMusic("circle_7");
  spawnRoomFromNode(runMap.current());
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
    stat("Velocidad", TextFormat("%.0f", ps.finalSpeed), Color{80, 200, 80, 255});
    stat("Stamina", TextFormat("%.0f", ps.finalMaxStamina),
         Color{80, 180, 220, 255});
    stat("Crit", TextFormat("%.0f%%", ps.finalCritChance * 100.0f),
         Color{255, 200, 60, 255});
    if (ps.finalLifesteal > 0.0f)
      stat("Robo Vida", TextFormat("%.0f%%", ps.finalLifesteal * 100.0f),
           Color{180, 255, 180, 255});
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
        Color col = Color{120, 255, 120, 255};
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

static void drawScaledNPatch(Texture2D tex, Rectangle dst, int srcBorder, int dstBorder, Color tint) {
    int w = tex.width;
    int h = tex.height;
    int sb = srcBorder;
    
    Rectangle srcTL = { 0.f, 0.f, (float)sb, (float)sb };
    Rectangle srcT = { (float)sb, 0.f, (float)(w - 2*sb), (float)sb };
    Rectangle srcTR = { (float)(w - sb), 0.f, (float)sb, (float)sb };
    Rectangle srcL = { 0.f, (float)sb, (float)sb, (float)(h - 2*sb) };
    Rectangle srcC = { (float)sb, (float)sb, (float)(w - 2*sb), (float)(h - 2*sb) };
    Rectangle srcR = { (float)(w - sb), (float)sb, (float)sb, (float)(h - 2*sb) };
    Rectangle srcBL = { 0.f, (float)(h - sb), (float)sb, (float)sb };
    Rectangle srcB = { (float)sb, (float)(h - sb), (float)(w - 2*sb), (float)sb };
    Rectangle srcBR = { (float)(w - sb), (float)(h - sb), (float)sb, (float)sb };

    float dw = dst.width;
    float dh = dst.height;
    
    float actualDbX = std::min((float)dstBorder, dw / 2.0f);
    float actualDbY = std::min((float)dstBorder, dh / 2.0f);
    
    Rectangle dstTL = { dst.x, dst.y, actualDbX, actualDbY };
    Rectangle dstT = { dst.x + actualDbX, dst.y, std::max(0.0f, dw - 2*actualDbX), actualDbY };
    Rectangle dstTR = { dst.x + dw - actualDbX, dst.y, actualDbX, actualDbY };
    Rectangle dstL = { dst.x, dst.y + actualDbY, actualDbX, std::max(0.0f, dh - 2*actualDbY) };
    Rectangle dstC = { dst.x + actualDbX, dst.y + actualDbY, std::max(0.0f, dw - 2*actualDbX), std::max(0.0f, dh - 2*actualDbY) };
    Rectangle dstR = { dst.x + dw - actualDbX, dst.y + actualDbY, actualDbX, std::max(0.0f, dh - 2*actualDbY) };
    Rectangle dstBL = { dst.x, dst.y + dh - actualDbY, actualDbX, actualDbY };
    Rectangle dstB = { dst.x + actualDbX, dst.y + dh - actualDbY, std::max(0.0f, dw - 2*actualDbX), actualDbY };
    Rectangle dstBR = { dst.x + dw - actualDbX, dst.y + dh - actualDbY, actualDbX, actualDbY };

    DrawTexturePro(tex, srcTL, dstTL, {0,0}, 0, tint);
    DrawTexturePro(tex, srcT, dstT, {0,0}, 0, tint);
    DrawTexturePro(tex, srcTR, dstTR, {0,0}, 0, tint);
    DrawTexturePro(tex, srcL, dstL, {0,0}, 0, tint);
    DrawTexturePro(tex, srcC, dstC, {0,0}, 0, tint);
    DrawTexturePro(tex, srcR, dstR, {0,0}, 0, tint);
    DrawTexturePro(tex, srcBL, dstBL, {0,0}, 0, tint);
    DrawTexturePro(tex, srcB, dstB, {0,0}, 0, tint);
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
             mainY + 105, 14, Color{80, 220, 80, 255}, screenWidth);
    TextUtils::drawCentered(TextFormat("HP despues: %d / %d", newHP, hp.maxHP),
             mainY + 130, 12, Color{150, 200, 150, 200}, screenWidth);
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

// =============================================================================
// Lifecycle
// =============================================================================
void Game::shutdown() {
  saveManager.save();
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
