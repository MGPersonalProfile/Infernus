#include "RenderSystem.h"
#include "../components/Combat.h"
#include "../components/Health.h"
#include "../components/Sprite.h"
#include "../components/Stamina.h"
#include "../components/Transform.h"
#include "../utils/Constants.h"
#include "../utils/TextUtils.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <vector>

void RenderSystem::update(Registry &registry) {
  auto entities = registry.view<Transform2D, Sprite>();

  // Sort by layer before rendering
  std::sort(entities.begin(), entities.end(), [&](Entity a, Entity b) {
    return registry.getComponent<Sprite>(a).layer <
           registry.getComponent<Sprite>(b).layer;
  });

  for (Entity entity : entities) {
    auto &transform = registry.getComponent<Transform2D>(entity);
    auto &sprite = registry.getComponent<Sprite>(entity);

    // Handle flip X by negating width in source rect
    Rectangle srcRect = sprite.sourceRect;
    srcRect.width = sprite.flipX ? -std::abs(srcRect.width)
                                 : std::abs(srcRect.width);

    // --- Visual proxies ---
    Color renderTint = sprite.tint;
    float renderScaleX = transform.scale;
    float renderScaleY = transform.scale;

    // I-Frames (dash / hit)
    if (registry.hasComponent<Health>(entity)) {
      auto &health = registry.getComponent<Health>(entity);
      if (health.isInvulnerable())
        renderTint = ColorAlpha(renderTint, 0.5f);
      if (health.hitFlashTimer > 0.0f)
        renderTint = RED;
    }

    // Combat states (squash & stretch)
    if (registry.hasComponent<Combat>(entity)) {
      auto &combat = registry.getComponent<Combat>(entity);
      if (combat.currentState == AttackState::WINDUP) {
        renderScaleX *= 1.2f;
        renderScaleY *= 0.8f;
      } else if (combat.currentState == AttackState::ACTIVE) {
        renderScaleX *= 0.8f;
        renderScaleY *= 1.2f;
        renderTint = RED;
      }
    }

    // Build destination rect with dynamic scales
    Rectangle destRect = {
        transform.x, transform.y, std::abs(srcRect.width) * renderScaleX,
        std::abs(srcRect.height) * renderScaleY};

    // Anchor adjustment to keep sprite grounded during squash
    destRect.y +=
        (std::abs(srcRect.height) * transform.scale) - destRect.height;
    destRect.x +=
        ((std::abs(srcRect.width) * transform.scale) - destRect.width) / 2.0f;

    DrawTexturePro(sprite.texture, srcRect, destRect, {0.0f, 0.0f},
                   transform.rotation, renderTint);
  }

  // --- Floating health bars ---
  auto healthEntities = registry.view<Transform2D, Health>();
  for (Entity entity : healthEntities) {
    auto &transform = registry.getComponent<Transform2D>(entity);
    auto &health = registry.getComponent<Health>(entity);

    if (health.currentHP >= health.maxHP)
      continue;

    float barX = transform.x;
    float barY = transform.y + Constants::HP_BAR_OFFSET_Y;
    float fillRatio =
        std::max(0.0f, (float)health.currentHP / (float)health.maxHP);

    DrawRectangle((int)barX, (int)barY, (int)Constants::HP_BAR_WIDTH,
                  (int)Constants::HP_BAR_HEIGHT, MAROON);
    DrawRectangle((int)barX, (int)barY,
                  (int)(Constants::HP_BAR_WIDTH * fillRatio),
                  (int)Constants::HP_BAR_HEIGHT, GREEN);
    TextUtils::draw(TextFormat("%d", health.currentHP), (int)barX, (int)(barY - 12),
             10, WHITE);
  }
}
