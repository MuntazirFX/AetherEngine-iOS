/* AetherHealth.h — Health display HUD element.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_HUD_HEALTH_H
#define AETHER_HUD_HEALTH_H

#include "AetherHUD.h"
#include "../../player/AetherPlayerHealth.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_hud_health {
    aether_hud_element_t  base;
    aether_player_health_t *player;   /* borrowed pointer */
    bool                   show_armor;
    bool                   show_battery;
    f32                    damage_flash_time;  /* remaining red flash */
} aether_hud_health_t;

/* Create a health HUD element.
 * Registers with the given HUD. Returns pointer to internal state. */
aether_hud_health_t *aether_hud_health_create(aether_hud_t *hud,
                                                aether_player_health_t *player);

/* Trigger red flash when player takes damage */
void aether_hud_health_trigger_damage_flash(aether_hud_health_t *h, f32 amount);

/* Per-frame update (for flash countdown) */
void aether_hud_health_tick(aether_hud_health_t *h, f32 dt);

/* Internal draw callback (called by HUD framework) */
void aether_hud_health_draw(aether_hud_element_t *self, void *render_ctx);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_HUD_HEALTH_H */
