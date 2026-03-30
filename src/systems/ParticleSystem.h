#pragma once
#include "../components/Lifetime.h"
#include "../components/Particle.h"
#include "../components/Sprite.h"
#include "../components/Transform.h"
#include "../core/ECS.h"

class ParticleSystem {
public:
  void update(Registry &registry, float /*deltaTime*/) {
    auto view = registry.view<Particle, Lifetime, Transform2D, Sprite>();

    for (Entity entity : view) {
      auto &particle = registry.getComponent<Particle>(entity);
      auto &lifetime = registry.getComponent<Lifetime>(entity);
      auto &transform = registry.getComponent<Transform2D>(entity);
      auto &sprite = registry.getComponent<Sprite>(entity);

      // Use Lifetime::progress() instead of hardcoded 0.5f
      float t = lifetime.progress();

      // Interpolate color
      sprite.tint.r =
          (unsigned char)(particle.startColor.r +
                          t * (particle.endColor.r - particle.startColor.r));
      sprite.tint.g =
          (unsigned char)(particle.startColor.g +
                          t * (particle.endColor.g - particle.startColor.g));
      sprite.tint.b =
          (unsigned char)(particle.startColor.b +
                          t * (particle.endColor.b - particle.startColor.b));
      sprite.tint.a =
          (unsigned char)(particle.startColor.a +
                          t * (particle.endColor.a - particle.startColor.a));

      // Interpolate scale
      transform.scale =
          particle.startScale + t * (particle.endScale - particle.startScale);
    }
  }
};
