#include "AetherHUDLayout.h"
#include <string.h>

void aether_hud_layout_classic(aether_hud_layout_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->margin = 0.04f;
    out->panel_gap = 0.012f;
    out->version = 1;
    /* Bottom-left status cluster */
    out->health = (aether_hud_rect_t){ 0.04f, 0.88f, 0.22f, 0.08f };
    out->armor  = (aether_hud_rect_t){ 0.04f, 0.80f, 0.18f, 0.06f };
    out->air    = (aether_hud_rect_t){ 0.04f, 0.73f, 0.16f, 0.05f };
    /* Bottom-right ammo */
    out->ammo   = (aether_hud_rect_t){ 0.74f, 0.88f, 0.22f, 0.08f };
    /* Top-center scoreboard, bottom-center chat */
    out->scoreboard = (aether_hud_rect_t){ 0.20f, 0.06f, 0.60f, 0.42f };
    out->chat       = (aether_hud_rect_t){ 0.20f, 0.70f, 0.60f, 0.16f };
}

u32 aether_hud_layout_apply(aether_hud_t *hud, const aether_hud_layout_t *layout) {
    if (!hud || !layout) return 0;
    u32 matched = 0;
    u32 n = aether_hud_count(hud);
    for (u32 i = 0; i < n; ++i) {
        const aether_hud_element_t *e = aether_hud_at(hud, i);
        if (!e || !e->name) continue;
        const aether_hud_rect_t *r = NULL;
        if (strcmp(e->name, "health") == 0) r = &layout->health;
        else if (strcmp(e->name, "armor") == 0) r = &layout->armor;
        else if (strcmp(e->name, "air") == 0) r = &layout->air;
        else if (strcmp(e->name, "ammo") == 0) r = &layout->ammo;
        else if (strcmp(e->name, "scoreboard") == 0) r = &layout->scoreboard;
        else if (strcmp(e->name, "chat") == 0) r = &layout->chat;
        if (!r) continue;
        aether_hud_set_position(hud, (i32)i, r->x, r->y);
        matched++;
    }
    return matched;
}

u32 aether_hud_layout_pack(const aether_hud_layout_t *layout, f32 *out24, u32 max_floats) {
    if (!layout || !out24 || max_floats < 24) return 0;
    const aether_hud_rect_t *rs[6] = {
        &layout->health, &layout->armor, &layout->air,
        &layout->ammo, &layout->scoreboard, &layout->chat
    };
    for (int i = 0; i < 6; ++i) {
        out24[i*4+0] = rs[i]->x;
        out24[i*4+1] = rs[i]->y;
        out24[i*4+2] = rs[i]->w;
        out24[i*4+3] = rs[i]->h;
    }
    return 24;
}
