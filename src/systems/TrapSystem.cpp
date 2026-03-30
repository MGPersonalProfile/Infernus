#include "TrapSystem.h"
#include "../components/Animation.h"
#include "../components/Collider.h"
#include "../components/Health.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../components/Trap.h"
#include "../components/Velocity.h"
#include "../ui/UIRenderer.h"
#include "raylib.h"

void TrapSystem::update(Registry &registry, float deltaTime) {
  auto traps = registry.view<Trap, Transform2D, Collider>();

  for (Entity trapEntity : traps) {
    auto &trap = registry.getComponent<Trap>(trapEntity);

    // Fire traps cycle on/off
    if (trap.type == TrapType::FIRE_TRAP) {
      trap.timer += deltaTime;
      while (trap.timer >= trap.cycleTime) {
        trap.timer -= trap.cycleTime;
        trap.active = !trap.active;
      }
      // Update animation frame based on active state
      if (registry.hasComponent<Animation>(trapEntity)) {
        auto &anim = registry.getComponent<Animation>(trapEntity);
        anim.currentFrame = trap.active ? 1 : 0;
      }
    }

    // Pits don't deal damage — they block movement (handled by collision)
    if (trap.type == TrapType::PIT || !trap.active)
      continue;

    // Check overlap with entities that have Health + Velocity (moving entities)
    auto &trapT = registry.getComponent<Transform2D>(trapEntity);
    auto &trapC = registry.getComponent<Collider>(trapEntity);

    auto victims = registry.view<Health, Transform2D, Collider, Velocity>();
    for (Entity victim : victims) {
      if (victim == trapEntity)
        continue;

      auto &vh = registry.getComponent<Health>(victim);
      if (vh.isDead() || vh.invulnerabilityTimer > 0.0f)
        continue;

      auto &vt = registry.getComponent<Transform2D>(victim);
      auto &vc = registry.getComponent<Collider>(victim);

      // AABB overlap
      bool overlaps =
          (trapC.rect.x < vc.rect.x + vc.rect.width &&
           trapC.rect.x + trapC.rect.width > vc.rect.x &&
           trapC.rect.y < vc.rect.y + vc.rect.height &&
           trapC.rect.y + trapC.rect.height > vc.rect.y);

      if (overlaps) {
        vh.currentHP -= trap.damage;
        vh.invulnerabilityTimer = 0.5f; // brief iframes from trap
        UIRenderer::spawnDamageNumber(registry, vt.x + 10.0f, vt.y,
                                      trap.damage, false);
      }
    }
  }
}
