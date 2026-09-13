/* AetherAmmo.c — Ammo HUD element implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherAmmo.h"
#include <stdlib.h>

/* Determine primary ammo type for current weapon */
static aether_ammo_type_t current_ammo_type(aether_weapon_id_t w) {
    switch (w) {
        case AETHER_WPN_GLOCK:
        case AETHER_WPN_MP5:      return AETHER_AMMO_9MM;
        case AETHER_WPN_PYTHON:   return AETHER_AMMO_357;
        case AETHER_WPN_SHOTGUN:  return AETHER_AMMO_BUCKSHOT;
        case AETHER_WPN_CROSSBOW: return AETHER_AMMO_BOLT;
        case AETHER_WPN_RPG:      return AETHER_AMMO_RPG;
        case AETHER_WPN_GAUSS:
        case AETHER_WPN_EGON:     return AETHER_AMMO_URANIUM;
        case AETHER_WPN_GRENADE:  return AETHER_AMMO_GRENADE;
        default:                  return AETHER_AMMO_NONE;
    }
}

aether_hud_ammo_t *aether_hud_ammo_create(aether_hud_t *hud,
                                            aether_player_inventory_t *player) {
    if (!hud) return NULL;

    aether_hud_ammo_t *h = (aether_hud_ammo_t*)calloc(1, sizeof *h);
    if (!h) return NULL;

    h->player  = player;
    h->clip_ammo = 0;
    h->clip_max  = 0;
    h->low_ammo_flash_time = 0.0f;

    h->base.name    = "ammo";
    h->base.visible = true;
    h->base.x       = 0.95f;
    h->base.y       = 0.90f;
    h->base.scale   = 1.0f;
    h->base.anchor  = AETHER_HUD_ANCHOR_BOT_RIGHT;
    h->base.color   = (aether_hud_color_t){ 1.0f, 1.0f, 1.0f, 1.0f };
    h->base.user    = h;
    h->base.draw    = aether_hud_ammo_draw;

    i32 idx = aether_hud_register(hud, &h->base);
    if (idx < 0) { free(h); return NULL; }

    aether_log(AETHER_LOG_INFO, "hud", "ammo element registered");
    return h;
}

void aether_hud_ammo_set_clip(aether_hud_ammo_t *h, i32 clip, i32 clip_max) {
    if (!h) return;
    h->clip_ammo = clip;
    h->clip_max  = clip_max;

    /* Trigger flash if low (below 20% or 5 bullets) */
    if (clip > 0 && (clip <= 5 || (clip_max > 0 && clip * 5 <= clip_max))) {
        h->low_ammo_flash_time = 0.4f;
    }
}

void aether_hud_ammo_tick(aether_hud_ammo_t *h, f32 dt) {
    if (!h) return;
    if (h->low_ammo_flash_time > 0.0f) {
        h->low_ammo_flash_time -= dt;
        if (h->low_ammo_flash_time < 0.0f) h->low_ammo_flash_time = 0.0f;
    }
}

void aether_hud_ammo_draw(aether_hud_element_t *self, void *render_ctx) {
    if (!self || !self->user) return;
    aether_hud_ammo_t *h = (aether_hud_ammo_t*)self->user;
    if (!h->player) return;

    aether_weapon_id_t w = aether_player_inv_current(h->player);
    aether_ammo_type_t t = current_ammo_type(w);
    i32 reserve = (t != AETHER_AMMO_NONE)
                  ? aether_player_inv_get_ammo(h->player, t)
                  : 0;

    /* Color: white normally, red when low */
    aether_hud_color_t c = (h->low_ammo_flash_time > 0.0f)
                            ? (aether_hud_color_t){ 1.0f, 0.3f, 0.3f, 1.0f }
                            : (aether_hud_color_t){ 1.0f, 1.0f, 1.0f, 1.0f };

    aether_log(AETHER_LOG_TRACE, "hud",
               "HUD ammo: %d / %d  (reserve=%d)  color=(%.2f,%.2f,%.2f)",
               h->clip_ammo, h->clip_max, reserve, c.r, c.g, c.b);

    (void)render_ctx;
}
