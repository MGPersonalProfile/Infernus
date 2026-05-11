#include "Game.h"
#include "../debug/DebugPanel.h"
#include "../debug/Profiler.h"
#include "../debug/Telemetry.h"
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


// === ROOM: room spawn / clear / start / end run flows ===

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

  // Try to load a handcrafted LDtk arena first (drops in seamlessly when
  // Antigravity adds files to assets/rooms/). Falls back to procedural.
  RoomTemplate room;
  std::string ldtkPath = isBossRoom
      ? "assets/rooms/boss_arena.ldtk"
      : "assets/rooms/combat_room_" + std::to_string(currentRoom) + ".ldtk";
  if (FileExists(ldtkPath.c_str())) {
    auto handcrafted = LDtkRoomLoader::loadProject(ldtkPath);
    if (!handcrafted.empty() && handcrafted[0].width > 0) {
      room = handcrafted[0];
      TraceLog(LOG_INFO, "ROOM: loaded handcrafted %s (%dx%d)",
               ldtkPath.c_str(), room.width, room.height);
    } else {
      room = roomGenerator.generate(currentRoom, isBossRoom);
    }
  } else {
    room = roomGenerator.generate(currentRoom, isBossRoom);
  }
  roomGenerator.instantiate(registry, room);
  collisionSystem.invalidateWallCache();
  // Tell AI system about the new room so enemies can pathfind
  aiSystem.setRoom(&roomGenerator.getCurrentRoom());

  // Telemetry: discrete event so QA scripts can correlate state changes
  if (Telemetry::isActive()) {
    char buf[160];
    snprintf(buf, sizeof(buf),
             "{\"index\":%d,\"boss\":%s,\"width\":%d,\"height\":%d}",
             currentRoom, isBossRoom ? "true" : "false",
             room.width, room.height);
    Telemetry::event("room_entered", buf);
  }

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
  // Collider centered horizontally + biased toward feet vertically — hitbox
  // matches the body silhouette instead of the full sprite frame (which had
  // empty padding around it). offsetX/Y position the collider relative to the
  // top-left of the entity Transform.
  {
    float colW = fw * 0.6f;   // narrower than visual sprite
    float colH = fh * 0.7f;   // shorter — feet to upper-torso
    auto &col = registry.addComponent<Collider>(playerEntity, colW, colH, false);
    col.offsetX = (fw - colW) * 0.5f;        // center horizontally
    col.offsetY = fh - colH;                 // anchor to feet
  }
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

  // First-run tutorial overlay (G.1): show prompts for 8s on very first run.
  tutorialFadeRemaining =
      (saveManager.getProgress().totalRuns == 0) ? 8.0f : 0.0f;

  // Generate the run map and spawn the first room
  runMap.generate();
  state = GameState::PLAYING;
  screenEffects.startFadeIn(0.5f);
  AudioManager::getInstance().playSFX("game_start");
  AudioManager::getInstance().playMusic("circle_7");
  spawnRoomFromNode(runMap.current());
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
