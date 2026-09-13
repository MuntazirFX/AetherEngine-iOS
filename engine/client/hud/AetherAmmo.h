/* AetherAmmo.h — Ammo display HUD element.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_HUD_AMMO_H
#define AETHER_HUD_AMMO_H

#include "AetherHUD.h"
#include "../../player/AetherPlayerInventory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_hud_ammo {
    aether_hud_element_t          base;
    aether_player_inventory_t    *player;   /* borrowed pointer */
    i32                           clip_ammo;   /* current magazine */
    i32                           clip_max;    /* magazine size */
    f32                           low_ammo_flash_time;
} aether_hud_ammo_t;

aether_hud_ammo_t *aether_hud_ammo_create(aether_hud_t *hud,
                                            aether_player_inventory_t *player);

/* Set clip info (called by weapon system) */
void aether_hud_ammo_set_clip(aether_hud_ammo_t *h, i32 clip, i32 clip_max);

/* Per-frame update */
void aether_hud_ammo_tick(aether_hud_ammo_t *h, f32 dt);

/* Draw callback */
void aether_hud_ammo_draw(aether_hud_element_t *self, void *render_ctx);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_HUD_AMMO_H */
