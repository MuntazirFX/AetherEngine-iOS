#include "AetherDynLight.h"
#include <string.h>

aether_result_t aether_dyn_lights_init(aether_dyn_lights_t *dl) {
    if (!dl) return AETHER_ERR_INVALID_ARG;
    memset(dl, 0, sizeof(*dl));
    return AETHER_OK;
}
void aether_dyn_lights_clear(aether_dyn_lights_t *dl) {
    if (dl) memset(dl, 0, sizeof(*dl));
}
aether_result_t aether_dyn_lights_add(aether_dyn_lights_t *dl,
                                      const f32 pos[3], const f32 color[3],
                                      f32 radius, f32 intensity) {
    if (!dl || !pos || !color || radius <= 0.f) return AETHER_ERR_INVALID_ARG;
    u32 slot = dl->count < AETHER_MAX_DYN_LIGHTS ? dl->count
                                                 : (dl->count % AETHER_MAX_DYN_LIGHTS);
    aether_dyn_light_t *L = &dl->items[slot];
    memcpy(L->position, pos, sizeof L->position);
    memcpy(L->color, color, sizeof L->color);
    L->radius = radius;
    L->intensity = intensity;
    L->active = true;
    if (dl->count < AETHER_MAX_DYN_LIGHTS) dl->count++;
    return AETHER_OK;
}
u32 aether_dyn_lights_active_count(const aether_dyn_lights_t *dl) {
    if (!dl) return 0;
    u32 n = 0;
    for (u32 i = 0; i < dl->count; ++i) if (dl->items[i].active) n++;
    return n;
}
u32 aether_dyn_lights_copy_render(const aether_dyn_lights_t *dl,
                                  aether_dyn_light_vertex_t *out, u32 max_out) {
    if (!dl || !out || max_out == 0) return 0;
    u32 w = 0;
    for (u32 i = 0; i < dl->count && w < max_out; ++i) {
        const aether_dyn_light_t *L = &dl->items[i];
        if (!L->active) continue;
        out[w].x = L->position[0]; out[w].y = L->position[1]; out[w].z = L->position[2];
        out[w].radius = L->radius;
        out[w].r = L->color[0]; out[w].g = L->color[1]; out[w].b = L->color[2];
        out[w].intensity = L->intensity;
        w++;
    }
    return w;
}
