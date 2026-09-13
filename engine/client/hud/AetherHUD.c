/* AetherHUD.c — HUD framework implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherHUD.h"
#include <stdlib.h>
#include <string.h>

struct aether_hud {
    aether_hud_element_t elements[AETHER_HUD_MAX_ELEMENTS];
    u32                  count;
};

aether_hud_t *aether_hud_create(void) {
    aether_hud_t *hud = (aether_hud_t*)calloc(1, sizeof *hud);
    if (!hud) return NULL;
    aether_log(AETHER_LOG_INFO, "hud", "HUD created (max=%d)", AETHER_HUD_MAX_ELEMENTS);
    return hud;
}

void aether_hud_destroy(aether_hud_t *hud) {
    if (!hud) return;
    free(hud);
    aether_log(AETHER_LOG_INFO, "hud", "HUD destroyed");
}

i32 aether_hud_register(aether_hud_t *hud, const aether_hud_element_t *elem) {
    if (!hud || !elem) return -1;
    if (hud->count >= AETHER_HUD_MAX_ELEMENTS) {
        aether_log(AETHER_LOG_WARN, "hud", "max elements reached");
        return -1;
    }

    i32 idx = (i32)hud->count;
    hud->elements[idx] = *elem;
    /* Defaults */
    if (hud->elements[idx].scale <= 0.0f) hud->elements[idx].scale = 1.0f;
    hud->count++;

    aether_log(AETHER_LOG_INFO, "hud", "registered element '%s' at idx=%d",
               elem->name ? elem->name : "?", idx);
    return idx;
}

bool aether_hud_set_visible(aether_hud_t *hud, i32 idx, bool visible) {
    if (!hud || idx < 0 || idx >= (i32)hud->count) return false;
    hud->elements[idx].visible = visible;
    return true;
}

bool aether_hud_set_position(aether_hud_t *hud, i32 idx, f32 x, f32 y) {
    if (!hud || idx < 0 || idx >= (i32)hud->count) return false;
    hud->elements[idx].x = x;
    hud->elements[idx].y = y;
    return true;
}

void aether_hud_draw(aether_hud_t *hud, void *render_ctx) {
    if (!hud) return;
    for (u32 i = 0; i < hud->count; ++i) {
        aether_hud_element_t *e = &hud->elements[i];
        if (!e->visible) continue;
        if (e->draw) e->draw(e, render_ctx);
    }
}

u32 aether_hud_count(const aether_hud_t *hud) {
    return hud ? hud->count : 0;
}

const aether_hud_element_t *aether_hud_at(const aether_hud_t *hud, u32 idx) {
    if (!hud || idx >= hud->count) return NULL;
    return &hud->elements[idx];
}
