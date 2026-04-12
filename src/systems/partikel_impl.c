// Pure C file — implements libpartikel + the bridge functions.
#define LIBPARTIKEL_IMPLEMENTATION
#include "partikel.h"
#include "partikel_bridge.h"

static Texture2D g_pTex = {0};

void partikel_set_texture(unsigned int texId, int w, int h) {
    g_pTex.id = texId;
    g_pTex.width = w;
    g_pTex.height = h;
    g_pTex.mipmaps = 1;
    g_pTex.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
}

static EmitterConfig cfgFromBridge(PartikelCfg c) {
    EmitterConfig cfg = {0};
    cfg.direction = (Vector2){c.dirX, c.dirY};
    cfg.velocity = (FloatRange){c.velMin, c.velMax};
    cfg.directionAngle = (FloatRange){c.angleMin, c.angleMax};
    cfg.velocityAngle = (FloatRange){0, 0};
    cfg.offset = (FloatRange){c.offsetMin, c.offsetMax};
    cfg.originAcceleration = (FloatRange){0, 0};
    cfg.burst = (IntRange){c.burstMin, c.burstMax};
    cfg.capacity = (size_t)c.capacity;
    cfg.emissionRate = 0;
    cfg.origin = (Vector2){c.originX, c.originY};
    cfg.externalAcceleration = (Vector2){c.gravX, c.gravY};
    cfg.startColor = (Color){c.startR, c.startG, c.startB, c.startA};
    cfg.endColor = (Color){c.endR, c.endG, c.endB, c.endA};
    cfg.age = (FloatRange){c.ageMin, c.ageMax};
    cfg.blendMode = c.additive ? BLEND_ADDITIVE : BLEND_ALPHA;
    cfg.texture = g_pTex;
    cfg.particle_Deactivator = Particle_DeactivatorAge;
    return cfg;
}

PartikelHandle partikel_create(PartikelCfg c) {
    return (PartikelHandle)Emitter_New(cfgFromBridge(c));
}

void partikel_reinit(PartikelHandle h, PartikelCfg c) {
    Emitter_Reinit((Emitter*)h, cfgFromBridge(c));
}

void partikel_burst(PartikelHandle h)  { Emitter_Burst((Emitter*)h); }
void partikel_start(PartikelHandle h)  { Emitter_Start((Emitter*)h); }
void partikel_stop(PartikelHandle h)   { Emitter_Stop((Emitter*)h); }
void partikel_update(PartikelHandle h, float dt) { Emitter_Update((Emitter*)h, dt); }
void partikel_draw(PartikelHandle h)   { Emitter_Draw((Emitter*)h); }
void partikel_free(PartikelHandle h)   { Emitter_Free((Emitter*)h); }
