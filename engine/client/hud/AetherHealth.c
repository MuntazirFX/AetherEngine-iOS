/* AetherHealth.c — Health HUD element implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherHealth.h"
#include <stdlib.h>
#include <string.h>

static aether_hud_color_t health_color(f32 hp) {
    if (hp > 75.0f) return (aether_hud_color_t){ 1.0f, 1.0f, 1.0f, 1.0f };
    if (hp > 50.0f) return (aether_hud_color_t){ 1.0f, 1.0f, 0.5f, 1.0f };
    if (hp > 25.0f) return (aether_hud_color_t){ 1.0f, 0.7f, 0.3f, 1.0f };
    return             (aether_hud_color_t){ 1.0f, 0.3f, 0.3f, 1.0f };
}

static aether_hud_color_t armor_color(f32 ar) {
    if (ar > 50.0f) return (aether_hud_color_t){ 0.5f, 0.9f, 1.0f, 1.0f };
    return             (aether_hud_color_t){ 0.7f, 0.7f, 0.9f, 1.0f };
}

aether_hud_health_t *aether_hud_health_create(aether_hud_t *hud,
                                                aether_player_health_t *player) {
    if (!hud) return NULL;

    aether_hud_health_t *h = (aether_hud_health_t*)calloc(1, sizeof *h);
    if (!h) return NULL;

    h->player = player;
    h->show_armor = true;
    h->show_battery = false;
    h->damage_flash_time = 0.0f;

    h->base.name    = "health";
    h->base.visible = true;
    h->base.x       = 0.05f;
    h->base.y       = 0.90f;
    h->base.scale   = 1.0f;
    h->base.anchor  = AETHER_HUD_ANCHOR_BOT_LEFT;
    h->base.color   = (aether_hud_color_t){ 1.0f, 1.0f, 1.0f, 1.0f };
    h->base.user    = h;
    h->base.draw    = aether_hud_health_draw;

    i32 idx = aether_hud_register(hud, &h->base);
    if (idx < 0) { free(h); return NULL; }

    aether_log(AETHER_LOG_INFO, "hud", "health element registered");
    return h;
}

void aether_hud_health_trigger_damage_flash(aether_hud_health_t *h, f32 amount) {
    if (!h || amount <= 0) return;
    f32 flash = amount * 0.02f;
    if (flash > 1.0f) flash = 1.0f;
    if (flash < 0.15f) flash = 0.15f;
    h->damage_flash_time = flash;
}

void aether_hud_health_tick(aether_hud_health_t *h, f32 dt) {
    if (!h) return;
    if (h->damage_flash_time > 0.0f) {
        h->damage_flash_time -= dt;
        if (h->damage_flash_time < 0.0f) h->damage_flash_time = 0.0f;
    }
}

void aether_hud_health_draw(aether_hud_element_t *self, void *render_ctx) {
    if (!self || !self->user) return;
    aether_hud_health_t *h = (aether_hud_health_t*)self->user;
    if (!h->player) return;

    f32 hp = aether_player_health_get(h->player);
    f32 ar = aether_player_health_get_armor(h->player);

    aether_hud_color_t hc = health_color(hp);
    aether_hud_color_t ac = armor_color(ar);

    /* Flash overlay */
    if (h->damage_flash_time > 0.0f) {
        f32 t = h->damage_flash_time;
        if (t > 1.0f) t = 1.0f;
        aether_log(AETHER_LOG_TRACE, "hud",
                   "damage flash: %.2f alpha", t);
    }

    aether_log(AETHER_LOG_TRACE, "hud",
               "HUD health: hp=%.0f (%.2f,%.2f,%.2f)  armor=%.0f (%.2f,%.2f,%.2f)",
               hp, hc.r, hc.g, hc.b, ar, ac.r, ac.g, ac.b);

    (void)render_ctx;  /* Metal renderer will hook here later */
}
