// C-callable bridge for libpartikel. Include from C++ safely.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle — actual Emitter* under the hood
typedef void* PartikelHandle;

typedef struct {
    float dirX, dirY;
    float velMin, velMax;
    float angleMin, angleMax;
    float offsetMin, offsetMax;
    int burstMin, burstMax;
    int capacity;
    float originX, originY;
    float gravX, gravY;
    unsigned char startR, startG, startB, startA;
    unsigned char endR, endG, endB, endA;
    float ageMin, ageMax;
    int additive; // 1 = BLEND_ADDITIVE, 0 = BLEND_ALPHA
} PartikelCfg;

PartikelHandle partikel_create(PartikelCfg cfg);
void partikel_reinit(PartikelHandle h, PartikelCfg cfg);
void partikel_burst(PartikelHandle h);
void partikel_start(PartikelHandle h);
void partikel_stop(PartikelHandle h);
void partikel_update(PartikelHandle h, float dt);
void partikel_draw(PartikelHandle h);
void partikel_free(PartikelHandle h);
void partikel_set_texture(unsigned int texId, int w, int h);

#ifdef __cplusplus
}
#endif
