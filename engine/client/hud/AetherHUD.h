/* AetherHUD.h — HUD framework (base for all HUD elements).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_HUD_H
#define AETHER_HUD_H

#include "../../core/AetherCore.h"
#include "../../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_HUD_MAX_ELEMENTS 32

/* HUD anchor for positioning */
typedef enum aether_hud_anchor {
    AETHER_HUD_ANCHOR_TOP_LEFT = 0,
    AETHER_HUD_ANCHOR_TOP_CENTER,
    AETHER_HUD_ANCHOR_TOP_RIGHT,
    AETHER_HUD_ANCHOR_MID_LEFT,
    AETHER_HUD_ANCHOR_CENTER,
    AETHER_HUD_ANCHOR_MID_RIGHT,
    AETHER_HUD_ANCHOR_BOT_LEFT,
    AETHER_HUD_ANCHOR_BOT_CENTER,
    AETHER_HUD_ANCHOR_BOT_RIGHT,
} aether_hud_anchor_t;

/* HUD color (RGBA, 0..1) */
typedef struct aether_hud_color {
    f32 r, g, b, a;
} aether_hud_color_t;

/* Single HUD element */
typedef struct aether_hud_element {
    const char          *name;
    bool                 visible;
    f32                  x, y;          /* position (0..1 normalized) */
    f32                  scale;
    aether_hud_anchor_t  anchor;
    aether_hud_color_t   color;
    void                *user;

    /* Draw callback — implementation-specific */
    void (*draw)(struct aether_hud_element *self, void *render_ctx);
} aether_hud_element_t;

/* HUD manager */
typedef struct aether_hud aether_hud_t;

aether_hud_t *aether_hud_create(void);
void          aether_hud_destroy(aether_hud_t *hud);

/* Element registration */
i32  aether_hud_register(aether_hud_t *hud, const aether_hud_element_t *elem);
bool aether_hud_set_visible(aether_hud_t *hud, i32 idx, bool visible);
bool aether_hud_set_position(aether_hud_t *hud, i32 idx, f32 x, f32 y);

/* Per-frame rendering */
void aether_hud_draw(aether_hud_t *hud, void *render_ctx);

/* Introspection */
u32 aether_hud_count(const aether_hud_t *hud);
const aether_hud_element_t *aether_hud_at(const aether_hud_t *hud, u32 idx);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_HUD_H */
