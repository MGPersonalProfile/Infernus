#include "AbilitySystem.h"
#include "../debug/Profiler.h"
#include "../audio/AudioManager.h"
#include "../components/AIBehavior.h"
#include "../components/Collider.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Lifetime.h"
#include "../components/Particle.h"
#include "../components/PlayerStats.h"
#include "../components/Sprite.h"
#include "../components/Stamina.h"
#include "../components/Transform.h"
#include "../components/Velocity.h"
#include "../core/ResourceManager.h"
#include "../systems/PartikelEmitters.h"
#include "../utils/JsonLoader.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <json.hpp>
#include "raylib.h"

using json = nlohmann::json;

// Map string -> AbilityEffect enum
static AbilityEffect parseEffect(const std::string &s) {
  if (s == "damage_boost")
    return AbilityEffect::DAMAGE_BOOST;
  if (s == "speed_boost")
    return AbilityEffect::SPEED_BOOST;
  if (s == "heal")
    return AbilityEffect::HEAL;
  if (s == "shield")
    return AbilityEffect::SHIELD;
  if (s == "thorns")
    return AbilityEffect::THORNS;
  if (s == "lifesteal")
    return AbilityEffect::LIFESTEAL;
  if (s == "stamina_regen")
    return AbilityEffect::STAMINA_REGEN;
  if (s == "fire_trail")
    return AbilityEffect::FIRE_TRAIL;
  if (s == "crit_chance")
    return AbilityEffect::CRIT_CHANCE;
  if (s == "max_hp_up")
    return AbilityEffect::MAX_HP_UP;
  if (s == "dash_damage")
    return AbilityEffect::DASH_DAMAGE;
  if (s == "attack_speed_up")
    return AbilityEffect::ATTACK_SPEED_UP;
  return AbilityEffect::DAMAGE_BOOST;
}

static AbilityRarity parseRarity(const std::string &s) {
  if (s == "rare")
    return AbilityRarity::RARE;
  if (s == "epic")
    return AbilityRarity::EPIC;
  return AbilityRarity::COMMON;
}

void AbilitySystem::loadAbilities(const std::string &path) {
  json data;
  if (!JsonLoader::load(path, data)) return; // already logged
  if (!data.contains("abilities") || !data["abilities"].is_array()) {
    TraceLog(LOG_WARNING, "JSON: %s — missing or invalid 'abilities' array",
             path.c_str());
    return;
  }
  for (auto &entry : data["abilities"]) {
    AbilityData ab;
    ab.id = entry.value("id", "unknown");
    ab.name = entry.value("name", "???");
    ab.description = entry.value("description", "");
    ab.effect = parseEffect(entry.value("effect", "damage_boost"));
    ab.rarity = parseRarity(entry.value("rarity", "common"));
    ab.value = entry.value("value", 0.0f);
    ab.duration = entry.value("duration", 0.0f);
    ab.cooldown = entry.value("cooldown", 0.0f);

    if (entry.contains("tags")) {
      for (auto &tag : entry["tags"])
        ab.tags.push_back(tag.get<std::string>());
    }

    abilityPool.push_back(ab);
  }
}

void AbilitySystem::update(Registry &registry, float deltaTime) {
  INFERNUS_ZONE_N("AbilitySystem");
  auto holders = registry.view<AbilityHolder>();
  for (Entity e : holders) {
    auto &holder = registry.getComponent<AbilityHolder>(e);

    bool changed = false;
    for (auto it = holder.activeBuffs.begin();
         it != holder.activeBuffs.end();) {
      it->remainingTime -= deltaTime;
      if (it->remainingTime <= 0.0f) {
        it = holder.activeBuffs.erase(it);
        changed = true;
      } else {
        ++it;
      }
    }

    if (changed)
      holder.recalculate();
  }

  // Tick active ability cooldowns
  auto actives = registry.view<ActiveAbilities>();
  for (Entity e : actives) {
    registry.getComponent<ActiveAbilities>(e).tick(deltaTime);
  }
}

std::vector<AbilityData> AbilitySystem::getRandomChoices(int count) const {
  std::vector<AbilityData> choices;
  if (abilityPool.empty())
    return choices;

  // Copy indices, shuffle, pick first N
  std::vector<int> indices;
  indices.reserve(abilityPool.size());
  for (int i = 0; i < (int)abilityPool.size(); i++)
    indices.push_back(i);

  // Fisher-Yates shuffle using raylib's random
  for (int i = (int)indices.size() - 1; i > 0; i--) {
    int j = GetRandomValue(0, i);
    std::swap(indices[i], indices[j]);
  }

  int n = std::min(count, (int)indices.size());
  for (int i = 0; i < n; i++)
    choices.push_back(abilityPool[indices[i]]);

  return choices;
}

void AbilitySystem::grantAbility(Registry &registry, Entity player,
                                 const AbilityData &ability) {
  if (!registry.hasComponent<AbilityHolder>(player))
    registry.addComponent<AbilityHolder>(player);

  auto &holder = registry.getComponent<AbilityHolder>(player);

  // Immediate one-shot effects
  if (ability.effect == AbilityEffect::HEAL) {
    if (registry.hasComponent<Health>(player)) {
      auto &h = registry.getComponent<Health>(player);
      h.currentHP = std::min(h.currentHP + (int)ability.value, h.maxHP);
    }
    return; // Don't store heal as a permanent ability
  }

  if (ability.effect == AbilityEffect::SHIELD) {
    if (registry.hasComponent<Health>(player)) {
      registry.getComponent<Health>(player).invulnerabilityTimer = ability.value;
    }
    return;
  }

  // Timed buff
  if (ability.duration > 0.0f) {
    holder.activeBuffs.push_back(
        {ability.effect, ability.value, ability.duration});
    holder.recalculate();
    return;
  }

  // Permanent passive — apply max HP immediately
  if (ability.effect == AbilityEffect::MAX_HP_UP) {
    if (registry.hasComponent<Health>(player)) {
      auto &h = registry.getComponent<Health>(player);
      h.maxHP += (int)ability.value;
      h.currentHP += (int)ability.value;
    }
  }

  holder.abilities.push_back(ability);
  holder.recalculate();
}

// =============================================================================
// Active Abilities (Q/E slots)
// =============================================================================

static ActiveAbilityType parseActiveType(const std::string &s) {
  if (s == "shield") return ActiveAbilityType::SHIELD;
  if (s == "teleport") return ActiveAbilityType::TELEPORT;
  if (s == "aoe_knockback") return ActiveAbilityType::AOE_KNOCKBACK;
  if (s == "lifesteal_burst") return ActiveAbilityType::LIFESTEAL_BURST;
  return ActiveAbilityType::PROJECTILE;
}

static DamageType parseDamageType(const std::string &s) {
  if (s == "fire") return DamageType::FIRE;
  if (s == "ice") return DamageType::ICE;
  if (s == "lightning") return DamageType::LIGHTNING;
  if (s == "toxic") return DamageType::TOXIC;
  return DamageType::PHYSICAL;
}

void AbilitySystem::loadActiveAbilities(const std::string &path) {
  json data;
  if (!JsonLoader::load(path, data)) return; // already logged
  if (!data.contains("abilities") || !data["abilities"].is_array()) {
    TraceLog(LOG_WARNING, "JSON: %s — missing or invalid 'abilities' array",
             path.c_str());
    return;
  }
  for (auto &entry : data["abilities"]) {
    ActiveAbilityData a;
    a.id          = entry.value("id", "unknown");
    a.name        = entry.value("name", "???");
    a.description = entry.value("description", "");
    a.iconPath    = entry.value("icon", "");
    a.type        = parseActiveType(entry.value("type", "projectile"));
    a.cooldown    = entry.value("cooldown", 4.0f);
    a.staminaCost = entry.value("stamina", 30.0f);
    a.duration    = entry.value("duration", 0.0f);
    a.param1      = entry.value("param1", 0.0f);
    a.param2      = entry.value("param2", 0.0f);
    a.damageType  = entry.value("damageType", "physical");
    activeAbilityPool[a.id] = a;
  }

  if (data.contains("defaultByClass")) {
    for (auto it = data["defaultByClass"].begin();
         it != data["defaultByClass"].end(); ++it) {
      std::vector<std::string> ids;
      for (auto &id : it.value()) ids.push_back(id.get<std::string>());
      defaultActivesByClass[it.key()] = ids;
    }
  }

  TraceLog(LOG_INFO, "ABILITY: Loaded %d active abilities, %d class defaults",
           (int)activeAbilityPool.size(), (int)defaultActivesByClass.size());
}

const ActiveAbilityData *AbilitySystem::findActive(const std::string &id) const {
  auto it = activeAbilityPool.find(id);
  return (it == activeAbilityPool.end()) ? nullptr : &it->second;
}

void AbilitySystem::equipDefaultActives(Registry &registry, Entity player,
                                       const std::string &classId) {
  auto it = defaultActivesByClass.find(classId);
  if (it == defaultActivesByClass.end()) return;
  if (it->second.size() >= 1) equipActive(registry, player, it->second[0], 0);
  if (it->second.size() >= 2) equipActive(registry, player, it->second[1], 1);
}

void AbilitySystem::equipActive(Registry &registry, Entity player,
                                const std::string &id, int slot) {
  if (!registry.hasComponent<ActiveAbilities>(player))
    registry.addComponent<ActiveAbilities>(player);
  auto &actives = registry.getComponent<ActiveAbilities>(player);

  const ActiveAbilityData *ab = findActive(id);
  if (!ab) {
    TraceLog(LOG_WARNING, "ABILITY: equipActive — id '%s' not in pool", id.c_str());
    return;
  }
  if (slot == 0) {
    actives.slotQ = *ab;
    actives.hasQ = true;
    actives.cooldownQ = 0.0f;
  } else if (slot == 1) {
    actives.slotE = *ab;
    actives.hasE = true;
    actives.cooldownE = 0.0f;
  }
}

bool AbilitySystem::tryUseActive(Registry &registry, Entity player, int slot) {
  if (!registry.hasComponent<ActiveAbilities>(player)) return false;
  if (!registry.hasComponent<Stamina>(player)) return false;

  auto &actives = registry.getComponent<ActiveAbilities>(player);
  auto &stam = registry.getComponent<Stamina>(player);

  bool ready = (slot == 0) ? actives.readyQ() : actives.readyE();
  if (!ready) return false;

  const ActiveAbilityData &a = (slot == 0) ? actives.slotQ : actives.slotE;
  if (!stam.hasEnough(a.staminaCost)) return false;

  // Pay cost + start cooldown
  stam.currentStamina -= a.staminaCost;
  stam.cooldownTimer = stam.regenDelay;
  if (slot == 0) actives.cooldownQ = a.cooldown;
  else           actives.cooldownE = a.cooldown;

  // Dispatch effect
  switch (a.type) {
  case ActiveAbilityType::PROJECTILE:        dispatchProjectile(registry, player, a); break;
  case ActiveAbilityType::SHIELD:            dispatchShield(registry, player, a); break;
  case ActiveAbilityType::TELEPORT:          dispatchTeleport(registry, player, a); break;
  case ActiveAbilityType::AOE_KNOCKBACK:     dispatchAOEKnockback(registry, player, a); break;
  case ActiveAbilityType::LIFESTEAL_BURST:   dispatchLifestealBurst(registry, player, a, slot); break;
  }
  return true;
}

// --- Effect dispatchers ---

void AbilitySystem::dispatchProjectile(Registry &registry, Entity player,
                                      const ActiveAbilityData &a) {
  if (!registry.hasComponent<Transform2D>(player)) return;
  auto &t = registry.getComponent<Transform2D>(player);
  float dx = t.facingX, dy = t.facingY;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 0.1f) { dx = 1.0f; dy = 0.0f; len = 1.0f; }
  dx /= len; dy /= len;

  Entity proj = registry.createEntity();
  registry.addComponent<Transform2D>(proj, t.x + 16.0f, t.y + 16.0f);
  registry.addComponent<Velocity>(proj, dx * a.param2, dy * a.param2);
  registry.addComponent<Collider>(proj, 16.0f, 16.0f, true);
  auto &c = registry.addComponent<Combat>(proj, (int)a.param1, 200.0f, player);
  c.damageType = parseDamageType(a.damageType);
  registry.addComponent<Lifetime>(proj, 1.5f);
  Texture2D tex = ResourceManager::getInstance().getTexture("assets/sprites/fx/fire_spear.png");
  registry.addComponent<Sprite>(proj, tex, Rectangle{0, 0, 16, 16}, 9);

  AudioManager::getInstance().playSFX("attack_heavy");
  PartikelEmitters::spawnFireTrail(t.x + 16.0f, t.y + 16.0f);
}

void AbilitySystem::dispatchShield(Registry &registry, Entity player,
                                  const ActiveAbilityData &a) {
  if (!registry.hasComponent<Health>(player)) return;
  registry.getComponent<Health>(player).invulnerabilityTimer = a.duration;
  AudioManager::getInstance().playSFX("dash");
}

void AbilitySystem::dispatchTeleport(Registry &registry, Entity player,
                                    const ActiveAbilityData &a) {
  if (!registry.hasComponent<Transform2D>(player)) return;
  auto &t = registry.getComponent<Transform2D>(player);
  float dx = t.facingX, dy = t.facingY;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 0.1f) { dx = 1.0f; dy = 0.0f; len = 1.0f; }
  dx /= len; dy /= len;
  // Spawn ghost at origin
  PartikelEmitters::spawnDashDust(t.x + 16.0f, t.y + 32.0f, dx);
  // Move
  t.x += dx * a.param1;
  t.y += dy * a.param1;
  PartikelEmitters::spawnDashDust(t.x + 16.0f, t.y + 32.0f, dx);
  // Brief i-frames after teleport
  if (registry.hasComponent<Health>(player))
    registry.getComponent<Health>(player).invulnerabilityTimer = 0.25f;
  AudioManager::getInstance().playSFX("dash");
}

void AbilitySystem::dispatchAOEKnockback(Registry &registry, Entity player,
                                        const ActiveAbilityData &a) {
  if (!registry.hasComponent<Transform2D>(player)) return;
  auto &pt = registry.getComponent<Transform2D>(player);
  float radius = a.param1;
  float radiusSq = radius * radius;

  auto enemies = registry.view<AIBehavior, Transform2D, Velocity>();
  for (Entity e : enemies) {
    auto &et = registry.getComponent<Transform2D>(e);
    float ex = et.x - pt.x, ey = et.y - pt.y;
    float distSq = ex * ex + ey * ey;
    if (distSq > radiusSq || distSq < 1.0f) continue;
    float dist = std::sqrt(distSq);
    auto &ev = registry.getComponent<Velocity>(e);
    ev.vx += (ex / dist) * a.param2;
    ev.vy += (ey / dist) * a.param2;
    // Stagger
    auto &ai = registry.getComponent<AIBehavior>(e);
    ai.currentState = AIState::STAGGER;
    ai.stateTimer = 0.0f;
    ai.staggerDuration = 1.0f;
  }

  // Visual: shockwave + shake
  PartikelEmitters::spawnSlamShockwave(pt.x + 16.0f, pt.y + 24.0f);
  AudioManager::getInstance().playSFX("boss_slam");
}

void AbilitySystem::dispatchLifestealBurst(Registry &registry, Entity player,
                                          const ActiveAbilityData &a, int slot) {
  if (!registry.hasComponent<ActiveAbilities>(player)) return;
  auto &actives = registry.getComponent<ActiveAbilities>(player);
  int charges = (int)a.duration; // semantic: number of empowered hits
  if (slot == 0) actives.chargesQ = charges;
  else           actives.chargesE = charges;
  // Visual hint: brief red tint via flash
  AudioManager::getInstance().playSFX("attack_light");
}
