#include "PartikelEmitters.h"
#include "partikel_bridge.h"
#include "raylib.h"
#include <unordered_map>
#include <string>

namespace PartikelEmitters {

static const int MAX_EMITTERS = 32;

struct EmitterSlot {
    PartikelHandle handle = nullptr;
    bool active = false;
    float timer = 0.0f;
};

static EmitterSlot g_slots[MAX_EMITTERS];
static Texture2D g_pixelTex;                          // 2x2 white fallback
static std::unordered_map<std::string, Texture2D> g_artTextures;
static bool g_initialized = false;

static int findFreeSlot() {
    for (int i = 0; i < MAX_EMITTERS; ++i)
        if (!g_slots[i].active) return i;
    return -1;
}

// Switch the global libpartikel texture before bursting. Particles spawned
// during this burst will sample from the new texture. Cached so we don't
// re-load on every spawn.
//
// Caveat: libpartikel uses a single global texture binding. If two effects
// with different textures are active simultaneously, both will draw using
// whichever was bound last. For combat where bursts are short (~0.6-0.9s),
// the visual imperfection is tolerable.
static void useArtTexture(const char *path) {
    auto it = g_artTextures.find(path);
    Texture2D tex;
    if (it == g_artTextures.end()) {
        tex = LoadTexture(path);
        if (tex.id == 0) {
            // Couldn't load — fall back to default and don't cache failure
            partikel_set_texture(g_pixelTex.id, g_pixelTex.width, g_pixelTex.height);
            return;
        }
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        g_artTextures[path] = tex;
    } else {
        tex = it->second;
    }
    partikel_set_texture(tex.id, tex.width, tex.height);
}

static void fireEmitter(PartikelCfg cfg, float duration) {
    int slot = findFreeSlot();
    if (slot < 0) return;

    if (g_slots[slot].handle) {
        partikel_reinit(g_slots[slot].handle, cfg);
    } else {
        g_slots[slot].handle = partikel_create(cfg);
    }
    g_slots[slot].active = true;
    g_slots[slot].timer = duration;
    partikel_start(g_slots[slot].handle);
    partikel_burst(g_slots[slot].handle);
}

void init() {
    if (g_initialized) return;
    Image img = GenImageColor(2, 2, WHITE);
    g_pixelTex = LoadTextureFromImage(img);
    UnloadImage(img);
    partikel_set_texture(g_pixelTex.id, g_pixelTex.width, g_pixelTex.height);
    for (int i = 0; i < MAX_EMITTERS; ++i) g_slots[i] = {};
    g_initialized = true;
}

void shutdown() {
    if (!g_initialized) return;
    for (int i = 0; i < MAX_EMITTERS; ++i) {
        if (g_slots[i].handle) {
            partikel_free(g_slots[i].handle);
            g_slots[i].handle = nullptr;
        }
    }
    for (auto &kv : g_artTextures) UnloadTexture(kv.second);
    g_artTextures.clear();
    UnloadTexture(g_pixelTex);
    g_initialized = false;
}

void update(float deltaTime) {
    if (!g_initialized) return;
    for (int i = 0; i < MAX_EMITTERS; ++i) {
        if (!g_slots[i].active) continue;
        partikel_update(g_slots[i].handle, deltaTime);
        g_slots[i].timer -= deltaTime;
        if (g_slots[i].timer <= 0.0f) {
            partikel_stop(g_slots[i].handle);
            g_slots[i].active = false;
        }
    }
}

void draw() {
    if (!g_initialized) return;
    for (int i = 0; i < MAX_EMITTERS; ++i) {
        if (!g_slots[i].active || !g_slots[i].handle) continue;
        partikel_draw(g_slots[i].handle);
    }
}

void spawnBlood(float x, float y, int count) {
    if (!g_initialized) return;
    useArtTexture("assets/sprites/particles/blood_drop.png");
    PartikelCfg cfg = {};
    cfg.dirX = 0; cfg.dirY = -1;
    cfg.velMin = 80; cfg.velMax = 200;
    cfg.angleMin = -180; cfg.angleMax = 180;
    cfg.offsetMin = 0; cfg.offsetMax = 5;
    cfg.burstMin = count; cfg.burstMax = count;
    cfg.capacity = count * 2;
    cfg.originX = x; cfg.originY = y;
    cfg.gravX = 0; cfg.gravY = 300;
    cfg.startR = 180; cfg.startG = 20; cfg.startB = 20; cfg.startA = 255;
    cfg.endR = 80; cfg.endG = 0; cfg.endB = 0; cfg.endA = 0;
    cfg.ageMin = 0.3f; cfg.ageMax = 0.7f;
    cfg.additive = 0;
    fireEmitter(cfg, 0.8f);
}

void spawnDashDust(float x, float y, float dirX) {
    if (!g_initialized) return;
    useArtTexture("assets/sprites/particles/dust_cloud.png");
    PartikelCfg cfg = {};
    cfg.dirX = -dirX; cfg.dirY = -0.3f;
    cfg.velMin = 40; cfg.velMax = 100;
    cfg.angleMin = -30; cfg.angleMax = 30;
    cfg.offsetMin = 0; cfg.offsetMax = 3;
    cfg.burstMin = 6; cfg.burstMax = 10;
    cfg.capacity = 20;
    cfg.originX = x; cfg.originY = y;
    cfg.gravX = 0; cfg.gravY = 50;
    cfg.startR = 160; cfg.startG = 140; cfg.startB = 120; cfg.startA = 180;
    cfg.endR = 100; cfg.endG = 90; cfg.endB = 80; cfg.endA = 0;
    cfg.ageMin = 0.2f; cfg.ageMax = 0.5f;
    cfg.additive = 0;
    fireEmitter(cfg, 0.6f);
}

void spawnFireTrail(float x, float y) {
    if (!g_initialized) return;
    useArtTexture("assets/sprites/particles/fire_burst.png");
    PartikelCfg cfg = {};
    cfg.dirX = 0; cfg.dirY = -1;
    cfg.velMin = 20; cfg.velMax = 60;
    cfg.angleMin = -20; cfg.angleMax = 20;
    cfg.offsetMin = 0; cfg.offsetMax = 4;
    cfg.burstMin = 3; cfg.burstMax = 6;
    cfg.capacity = 16;
    cfg.originX = x; cfg.originY = y;
    cfg.gravX = 0; cfg.gravY = -40;
    cfg.startR = 255; cfg.startG = 160; cfg.startB = 30; cfg.startA = 240;
    cfg.endR = 200; cfg.endG = 40; cfg.endB = 10; cfg.endA = 0;
    cfg.ageMin = 0.3f; cfg.ageMax = 0.8f;
    cfg.additive = 1;
    fireEmitter(cfg, 0.9f);
}

void spawnSlamShockwave(float x, float y) {
    if (!g_initialized) return;
    useArtTexture("assets/sprites/particles/shockwave.png");
    PartikelCfg cfg = {};
    cfg.dirX = 1; cfg.dirY = 0;
    cfg.velMin = 150; cfg.velMax = 350;
    cfg.angleMin = -180; cfg.angleMax = 180;
    cfg.offsetMin = 0; cfg.offsetMax = 8;
    cfg.burstMin = 20; cfg.burstMax = 30;
    cfg.capacity = 60;
    cfg.originX = x; cfg.originY = y;
    cfg.gravX = 0; cfg.gravY = 200;
    cfg.startR = 255; cfg.startG = 200; cfg.startB = 100; cfg.startA = 255;
    cfg.endR = 150; cfg.endG = 80; cfg.endB = 30; cfg.endA = 0;
    cfg.ageMin = 0.2f; cfg.ageMax = 0.6f;
    cfg.additive = 1;
    fireEmitter(cfg, 0.7f);
}

} // namespace PartikelEmitters
