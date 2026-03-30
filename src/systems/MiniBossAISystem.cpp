#include "MiniBossAISystem.h"
#include "../components/MiniBoss.h"
#include "../components/Transform.h"
#include "../components/Health.h"
#include "../components/Combat.h"
#include "../components/Velocity.h"
#include "../components/Collider.h"
#include "../components/Lifetime.h"
#include "CameraSystem.h"
#include <raylib.h>
#include <cmath>

void MiniBossAISystem::update(Registry &registry, CameraSystem &cam, float deltaTime, Entity playerEntity) {
    auto view = registry.view<MiniBoss, Transform2D, Health, Combat>();
    for (auto entity : view) {
        auto &mb = registry.getComponent<MiniBoss>(entity);
        auto &health = registry.getComponent<Health>(entity);

        if (health.isDead()) continue;

        if (!mb.specialActive) {
            mb.specialTimer -= deltaTime;
            if (mb.specialTimer <= 0.0f) {
                mb.specialActive = true;
                mb.specialStep = 0;
            }
        }

        if (mb.specialActive) {
            switch (mb.specialType) {
                case 0:
                    executeCharge(registry, entity, cam, deltaTime, playerEntity);
                    break;
                case 1:
                    executeMultishot(registry, entity, deltaTime, playerEntity);
                    break;
                case 2:
                    executeSlam(registry, entity, cam, deltaTime);
                    break;
            }
        }
    }
}

void MiniBossAISystem::executeCharge(Registry &registry, Entity mb, CameraSystem &cam, float deltaTime, Entity player) {
    auto &boss = registry.getComponent<MiniBoss>(mb);
    auto &transform = registry.getComponent<Transform2D>(mb);
    auto &vel = registry.getComponent<Velocity>(mb);
    auto &combat = registry.getComponent<Combat>(mb);

    switch (boss.specialStep) {
        case 0: { // Windup
            if (boss.chargeTimer == 0.0f) {
                boss.chargeTimer = 0.5f;
            }
            vel.vx = 0.0f;
            vel.vy = 0.0f;
            boss.chargeTimer -= deltaTime;
            if (boss.chargeTimer <= 0.0f) {
                auto &playerTransform = registry.getComponent<Transform2D>(player);
                float dx = playerTransform.x - transform.x;
                float dy = playerTransform.y - transform.y;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.0f) {
                    boss.chargeDirX = dx / len;
                    boss.chargeDirY = dy / len;
                } else {
                    boss.chargeDirX = 1.0f;
                    boss.chargeDirY = 0.0f;
                }
                boss.specialStep = 1;
                boss.chargeTimer = 0.5f;
            }
            break;
        }
        case 1: { // Charge
            vel.vx = boss.chargeDirX * 700.0f;
            vel.vy = boss.chargeDirY * 700.0f;
            boss.chargeTimer -= deltaTime;
            if (boss.chargeTimer <= 0.0f) {
                vel.vx = 0.0f;
                vel.vy = 0.0f;
                boss.specialStep = 2;
                boss.chargeTimer = 0.5f;

                // Spawn shockwave hitbox
                Entity shockwave = registry.createEntity();
                registry.addComponent<Transform2D>(shockwave, transform.x - 40.0f, transform.y - 40.0f);
                registry.addComponent<Collider>(shockwave, 80.0f, 80.0f);
                registry.addComponent<Combat>(shockwave, combat.baseDamage * 2, 300.0f, mb);
                registry.addComponent<Lifetime>(shockwave, 0.15f);

                cam.addShake(12.0f, 0.4f);
            }
            break;
        }
        case 2: { // Recovery
            boss.chargeTimer -= deltaTime;
            if (boss.chargeTimer <= 0.0f) {
                boss.specialActive = false;
                boss.specialTimer = boss.specialCooldown;
                boss.specialStep = 0;
                boss.chargeTimer = 0.0f;
            }
            break;
        }
    }
}

void MiniBossAISystem::executeMultishot(Registry &registry, Entity mb, float deltaTime, Entity player) {
    auto &boss = registry.getComponent<MiniBoss>(mb);
    auto &transform = registry.getComponent<Transform2D>(mb);
    auto &combat = registry.getComponent<Combat>(mb);

    switch (boss.specialStep) {
        case 0: { // Windup
            if (boss.chargeTimer == 0.0f) {
                boss.chargeTimer = 0.4f;
            }
            boss.chargeTimer -= deltaTime;
            if (boss.chargeTimer <= 0.0f) {
                boss.specialStep = 1;
            }
            break;
        }
        case 1: { // Fire fan of 5 projectiles
            auto &playerTransform = registry.getComponent<Transform2D>(player);
            float dx = playerTransform.x - transform.x;
            float dy = playerTransform.y - transform.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 0.0f) {
                dx /= len;
                dy /= len;
            } else {
                dx = 1.0f;
                dy = 0.0f;
            }

            for (int i = -2; i <= 2; i++) {
                float angle = i * 0.25f;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);
                float dirX = dx * cosA - dy * sinA;
                float dirY = dx * sinA + dy * cosA;

                Entity proj = registry.createEntity();
                registry.addComponent<Transform2D>(proj, transform.x, transform.y);
                registry.addComponent<Velocity>(proj, dirX * 350.0f, dirY * 350.0f);
                registry.addComponent<Collider>(proj, 12.0f, 12.0f, true);
                registry.addComponent<Combat>(proj, combat.baseDamage, 80.0f, mb);
                registry.addComponent<Lifetime>(proj, 2.0f);
            }

            boss.specialStep = 2;
            boss.chargeTimer = 0.8f;
            break;
        }
        case 2: { // Recovery
            boss.chargeTimer -= deltaTime;
            if (boss.chargeTimer <= 0.0f) {
                boss.specialActive = false;
                boss.specialTimer = boss.specialCooldown;
                boss.specialStep = 0;
                boss.chargeTimer = 0.0f;
            }
            break;
        }
    }
}

void MiniBossAISystem::executeSlam(Registry &registry, Entity mb, CameraSystem &cam, float deltaTime) {
    auto &boss = registry.getComponent<MiniBoss>(mb);
    auto &transform = registry.getComponent<Transform2D>(mb);
    auto &combat = registry.getComponent<Combat>(mb);

    switch (boss.specialStep) {
        case 0: { // Windup
            if (boss.chargeTimer == 0.0f) {
                boss.chargeTimer = 0.7f;
            }
            boss.chargeTimer -= deltaTime;
            if (boss.chargeTimer <= 0.0f) {
                boss.specialStep = 1;
            }
            break;
        }
        case 1: { // Slam - spawn 3 shockwaves
            float offsets[][2] = {{0.0f, 0.0f}, {80.0f, 0.0f}, {-80.0f, 0.0f}};
            for (int i = 0; i < 3; i++) {
                Entity shockwave = registry.createEntity();
                registry.addComponent<Transform2D>(shockwave, transform.x + offsets[i][0], transform.y + offsets[i][1]);
                registry.addComponent<Collider>(shockwave, 100.0f, 100.0f);
                registry.addComponent<Combat>(shockwave, (int)(combat.baseDamage * 1.5f), 200.0f, mb);
                registry.addComponent<Lifetime>(shockwave, 0.2f);
            }

            cam.addShake(15.0f, 0.5f);
            boss.specialStep = 2;
            boss.chargeTimer = 1.0f;
            break;
        }
        case 2: { // Recovery
            boss.chargeTimer -= deltaTime;
            if (boss.chargeTimer <= 0.0f) {
                boss.specialActive = false;
                boss.specialTimer = boss.specialCooldown;
                boss.specialStep = 0;
                boss.chargeTimer = 0.0f;
            }
            break;
        }
    }
}
