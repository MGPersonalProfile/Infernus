#pragma once

namespace Constants {

// -- Window ------------------------------------------------------------------
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TARGET_FPS = 60;

// -- Player ------------------------------------------------------------------
constexpr float PLAYER_SPEED = 250.0f;
constexpr float DASH_SPEED = 1200.0f;
constexpr float DASH_IFRAMES = 0.3f;
constexpr float LIGHT_ATTACK_STAMINA = 20.0f;
constexpr float HEAVY_ATTACK_STAMINA = 40.0f;
constexpr float DASH_STAMINA = 30.0f;
constexpr float LIGHT_ATTACK_WINDUP = 0.1f;
constexpr float HEAVY_ATTACK_WINDUP = 0.4f;
constexpr float PARRY_STAMINA = 15.0f;
constexpr float PARRY_WINDOW = 0.2f;       // active parry frames
constexpr float PARRY_RECOVERY = 0.4f;     // locked after parry window expires
constexpr float PARRY_STAGGER_TIME = 1.5f; // enemy stagger on successful parry
constexpr float COMBO_WINDOW = 0.35f;      // time to chain next hit after recovery

// -- Combat ------------------------------------------------------------------
constexpr float HITBOX_ACTIVE_TIME = 0.15f;
constexpr float ATTACK_RECOVERY_TIME = 0.2f;
constexpr float HIT_IFRAMES = 0.3f;
constexpr float HIT_FLASH_TIME = 0.1f;
constexpr float HITBOX_OFFSET_LEFT = -45.0f;
constexpr float HITBOX_OFFSET_RIGHT = 35.0f;
constexpr float HITBOX_WIDTH = 40.0f;
constexpr float HITBOX_HEIGHT = 50.0f;

// -- Camera ------------------------------------------------------------------
constexpr float CAMERA_LERP_SPEED = 5.0f;
constexpr float CAMERA_SHAKE_DECAY = 10.0f;

// -- Particles ---------------------------------------------------------------
constexpr int HIT_PARTICLES_MIN = 3;
constexpr int HIT_PARTICLES_MAX = 6;
constexpr float HIT_PARTICLE_LIFETIME = 0.3f;
constexpr int DEATH_PARTICLES_MIN = 15;
constexpr int DEATH_PARTICLES_MAX = 25;
constexpr float DEATH_PARTICLE_LIFETIME = 0.5f;

// -- AI ----------------------------------------------------------------------
constexpr float STAGGER_DURATION = 0.3f;
constexpr float IDLE_DURATION = 1.0f;
constexpr float AGGRO_LEASH_MULTIPLIER = 1.5f;

// -- Loot --------------------------------------------------------------------
constexpr float LOOT_FLOAT_SPEED = -30.0f;
constexpr float LOOT_LIFETIME = 8.0f;
constexpr float LOOT_ORB_SIZE = 12.0f;

// -- Grid / Tiles ------------------------------------------------------------
constexpr int TILE_SIZE = 64;

// -- Health Bar (HUD) --------------------------------------------------------
constexpr float HP_BAR_WIDTH = 40.0f;
constexpr float HP_BAR_HEIGHT = 5.0f;
constexpr float HP_BAR_OFFSET_Y = -12.0f;

} // namespace Constants
