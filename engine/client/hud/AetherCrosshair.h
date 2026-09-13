/* AetherCrosshair.h — Crosshair HUD element (4 styles).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_HUD_CROSSHAIR_H
#define AETHER_HUD_CROSSHAIR_H

#include "AetherHUD.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aether_crosshair_style {
    AETHER_CROSS_DEFAULT = 0,  /* HL classic cross */
    AETHER_CROSS_DOT,          /* simple dot */
    AETHER_CROSS_CIRCLE,       /* circle */
    AETHER_CROSS_CROSSHAIR,    /* CS-style 4-line */
    AETHER_CROSS_STYLE_COUNT
} aether_crosshair_style_t;

typedef struct aether_hud_crosshair {
    aether_hud_element_t      base;
    aether_crosshair_style_t  style;
    f32                       size;         /* 0..1 normalized */
    f32                       gap;          /* space from center */
    f32                       thickness;
    bool                      dynamic;      /* expand when moving */
    f32                       spread;       /* extra gap from movement */
} aether_hud_crosshair_t;

aether_hud_crosshair_t *aether_hud_crosshair_create(aether_hud_t *hud);

void aether_hud_crosshair_set_style(aether_hud_crosshair_t *c,
                                     aether_crosshair_style_t style);
void aether_hud_crosshair_set_spread (aether_hud_crosshair_t *c, f32 spread);
void aether_hud_crosshair_tick      (aether_hud_crosshair_t *c, f32 dt);

void aether_hud_crosshair_draw(aether_hud_element_t *self, void *render_ctx);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_HUD_CROSSHAIR_H */
