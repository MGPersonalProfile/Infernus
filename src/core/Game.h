#pragma once
#include <functional>
#include "../core/ECS.h"
#include "../core/GameState.h"
#include "../core/ResourceManager.h"
#include "../input/InputManager.h"
#include "../audio/AudioManager.h"
#include "../systems/AISystem.h"
#include "../systems/AnimationSystem.h"
#include "../systems/BossAISystem.h"
#include "../systems/CameraSystem.h"
#include "../systems/CollisionSystem.h"
#include "../systems/CombatSystem.h"
#include "../systems/HealthSystem.h"
#include "../systems/MiniBossAISystem.h"
#include "../systems/MovementSystem.h"
#include "../systems/ParticleSystem.h"
#include "../systems/RenderSystem.h"
#include "../systems/ScreenEffects.h"
#include "../systems/AbilitySystem.h"
#include "../systems/StaminaSystem.h"
#include "../systems/SynergySystem.h"
#include "../systems/TrapSystem.h"
#include "../systems/PhysicsVFXSystem.h"
#include "../systems/ItemSystem.h"

#include "../components/Ability.h"
#include "../components/Item.h"
#include "../components/ItemPickup.h"
#include "../components/PlayerStats.h"
#include "../meta/SaveManager.h"
#include "../components/DamageNumber.h"
#include "../entities/BossFactory.h"
#include "../entities/MiniBossFactory.h"
#include "../ui/UIRenderer.h"
#include "../world/RoomGenerator.h"
#include "../entities/EnemyFactory.h"
#include "../entities/PlayerFactory.h"

class Game {
public:
  Game();

  void init();
  void update(float deltaTime);
  void render();
  void shutdown();
  void run();

private:
  int screenWidth;
  int screenHeight;

  GameState state = GameState::MAIN_MENU;
  Registry registry;
  InputManager inputManager;

  // Systems
  MovementSystem movementSystem;
  RenderSystem renderSystem;
  AnimationSystem animationSystem;
  CollisionSystem collisionSystem;
  CombatSystem combatSystem;
  StaminaSystem staminaSystem;
  HealthSystem healthSystem;
  CameraSystem cameraSystem;
  ParticleSystem particleSystem;
  AISystem aiSystem;
  BossAISystem bossAISystem;
  MiniBossAISystem miniBossAISystem;
  ScreenEffects screenEffects;
  AbilitySystem abilitySystem;
  SynergySystem synergySystem;
  TrapSystem trapSystem;
  PhysicsVFXSystem physicsVFXSystem;
  ItemSystem itemSystem;

  RoomGenerator roomGenerator;
  SaveManager saveManager;

  Entity playerEntity = NULL_ENTITY;
  Entity bossEntity = NULL_ENTITY;

  // Room/wave management
  int currentRoom = 0;
  int totalRooms = 5;       // rooms before boss
  int enemiesAlive = 0;
  bool roomCleared = false;

  // Ability selection
  std::vector<AbilityData> abilityChoices;
  int selectedAbility = 0;

  // Character selection
  int selectedCharacter = 0;
  std::string currentCharacterId = "warrior";
  float specialCooldownTimer = 0.0f;

  // Options menu
  int optionSelected = 0;
  GameState stateBeforeOptions = GameState::MAIN_MENU;

  // Pause menu navigation
  int pauseSelected = 0;

  // Screen transitions (fade out → switch state → fade in)
  GameState pendingTransition = GameState::MAIN_MENU;
  int transitionPhase = 0; // 0=none, 1=fading out, 2=fading in
  float transitionDuration = 0.25f;
  void transitionTo(GameState target, float duration = 0.25f);
  // Callback actions to run when the fade-out completes (before fade-in)
  std::function<void()> transitionCallback = nullptr;

  // Settings toggles
  bool screenShakeEnabled = true;
  bool damageNumbersEnabled = true;

  // Inventory
  int inventorySelectedSlot = 0;

  // Item swap (when picking up item with full inventory)
  ItemData pendingItem;

  void handlePlayerInput();
  void processLootPickups();
  void spawnRoom();
  void checkRoomClear();
  void executeSpecialAttack();
  void recalculatePlayerStats();

  // Atmospheric particles (ash/embers floating in dungeon)
  struct AshParticle {
    float x, y, vx, vy, life, maxLife, size;
    Color color;
  };
  std::vector<AshParticle> ashParticles;
  float ashSpawnTimer = 0.0f;
  void updateAtmosphericParticles(float deltaTime);
  void renderAtmosphericParticles();

  // UI helper: draw a panel with the art texture (fallback to solid color)
  void drawUIPanel(int x, int y, int w, int h, Color fallbackBg, Color borderCol);

  // UI drawing
  void drawHUD();
  void drawMainMenu();
  void drawPauseMenu();
  void drawGameOver();
  void drawVictory();
  void drawBossIntro();
  void drawBossHealthBar();
  void drawMiniBossHealthBar();
  void drawAbilitySelect();
  void drawCharacterSelect();
  void drawOptions();
  void drawRunStats();
  void drawInventory();
  void drawStatsWindow();
  void drawItemSwap();
  void drawDebugOverlay();
  void drawAbilitiesView();
  void startGame(int characterIndex);
  void endRun(bool victory);
};
