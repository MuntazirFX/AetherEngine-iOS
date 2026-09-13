/* AetherCrosshair.c — Crosshair HUD element implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherCrosshair.h"
#include <stdlib.h>

aether_hud_crosshair_t *aether_hud_crosshair_create(aether_hud_t *hud) {
    if (!hud) return NULL;

    aether_hud_crosshair_t *c = (aether_hud_crosshair_t*)calloc(1, sizeof *c);
    if (!c) return NULL;

    c->style     = AETHER_CROSS_DEFAULT;
    c->size      = 0.01f;
    c->gap       = 0.005f;
    c->thickness = 0.002f;
    c->dynamic   = true;
    c->spread    = 0.0f;

    c->base.name    = "crosshair";
    c->base.visible = true;
    c->base.x       = 0.5f;
    c->base.y       = 0.5f;
    c->base.scale   = 1.0f;
    c->base.anchor  = AETHER_HUD_ANCHOR_CENTER;
    c->base.color   = (aether_hud_color_t){ 0.0f, 1.0f, 0.0f, 1.0f };
    c->base.user    = c;
    c->base.draw    = aether_hud_crosshair_draw;

    i32 idx = aether_hud_register(hud, &c->base);
    if (idx < 0) { free(c); return NULL; }

    aether_log(AETHER_LOG_INFO, "hud", "crosshair element registered (style=%d)",
               (int)c->style);
    return c;
}

void aether_hud_crosshair_set_style(aether_hud_crosshair_t *c,
                                     aether_crosshair_style_t style) {
    if (!c) return;
    if (style < 0 || style >= AETHER_CROSS_STYLE_COUNT) return;
    c->style = style;
    aether_log(AETHER_LOG_INFO, "hud", "crosshair style -> %d", (int)style);
}

void aether_hud_crosshair_set_spread(aether_hud_crosshair_t *c, f32 spread) {
    if (!c) return;
    if (spread < 0.0f) spread = 0.0f;
    if (spread > 1.0f) spread = 1.0f;
    c->spread = spread;
}

void aether_hud_crosshair_tick(aether_hud_crosshair_t *c, f32 dt) {
    if (!c) return;
    /* Spread decays over time when player stops moving */
    if (c->spread > 0.0f) {
        f32 decay = 1.5f * dt;
        c->spread -= decay;
        if (c->spread < 0.0f) c->spread = 0.0f;
    }
}

void aether_hud_crosshair_draw(aether_hud_element_t *self, void *render_ctx) {
    if (!self || !self->user) return;
    aether_hud_crosshair_t *c = (aether_hud_crosshair_t*)self->user;

    /* Compute final gap based on style + spread */
    f32 gap = c->gap;
    if (c->dynamic) gap += c->spread * 0.02f;

    aether_log(AETHER_LOG_TRACE, "hud",
               "HUD crosshair: style=%d  size=%.3f  gap=%.3f  thick=%.3f  spread=%.2f",
               (int)c->style, c->size, gap, c->thickness, c->spread);

    (void)render_ctx;
}
