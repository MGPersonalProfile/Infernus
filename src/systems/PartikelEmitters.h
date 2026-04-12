#pragma once
// Emitter-based particle effects using libpartikel.
// Complements the ECS-based ParticleSystem with high-volume effects:
// blood bursts, fire trails, dash dust, boss slam shockwaves.
//
// Usage:
//   PartikelEmitters::init();          // after InitWindow()
//   PartikelEmitters::spawnBlood(x,y); // one-shot burst
//   PartikelEmitters::update(dt);      // each frame
//   PartikelEmitters::draw();          // in BeginMode2D / EndMode2D
//   PartikelEmitters::shutdown();      // before CloseWindow()

namespace PartikelEmitters {

void init();
void shutdown();
void update(float deltaTime);
void draw();

// One-shot effects
void spawnBlood(float x, float y, int count = 12);
void spawnDashDust(float x, float y, float dirX);
void spawnFireTrail(float x, float y);
void spawnSlamShockwave(float x, float y);

} // namespace PartikelEmitters
