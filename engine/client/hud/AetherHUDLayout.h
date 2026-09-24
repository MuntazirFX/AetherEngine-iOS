/* AetherHUDLayout.h — Consistent HUD panel rects (health/armor/air/scoreboard/chat).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_HUD_LAYOUT_H
#define AETHER_HUD_LAYOUT_H

#include "../../core/AetherCore.h"
#include "AetherHUD.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_hud_rect {
    f32 x, y;   /* normalized 0..1 top-left of panel */
    f32 w, h;   /* normalized size */
} aether_hud_rect_t;

typedef struct aether_hud_layout {
    aether_hud_rect_t health;
    aether_hud_rect_t armor;
    aether_hud_rect_t air;
    aether_hud_rect_t ammo;
    aether_hud_rect_t scoreboard;
    aether_hud_rect_t chat;
    f32 margin;       /* edge inset 0..1 */
    f32 panel_gap;    /* gap between stacked panels */
    u32 version;      /* layout schema version */
} aether_hud_layout_t;

/* Classic GoldSrc-ish bottom-left status + bottom-right ammo + top scoreboard/chat. */
void aether_hud_layout_classic(aether_hud_layout_t *out);

/* Apply layout positions onto registered HUD element slots by name.
 * Names: "health","armor","air","ammo","scoreboard","chat". Returns matches. */
u32 aether_hud_layout_apply(aether_hud_t *hud, const aether_hud_layout_t *layout);

/* Copy layout as 6*4 floats [x,y,w,h] × health,armor,air,ammo,scoreboard,chat. */
u32 aether_hud_layout_pack(const aether_hud_layout_t *layout, f32 *out24, u32 max_floats);

#ifdef __cplusplus
}
#endif
#endif
