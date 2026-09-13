/* AetherPlayerHealth.c — Player health implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherPlayerHealth.h"
#include <string.h>

void aether_player_health_init(aether_player_health_t *h) {
    if (!h) return;
    memset(h, 0, sizeof *h);
    h->health      = AETHER_PLAYER_START_HEALTH;
    h->max_health  = AETHER_PLAYER_MAX_HEALTH;
    h->armor       = AETHER_PLAYER_START_ARMOR;
    h->max_armor   = AETHER_PLAYER_MAX_ARMOR;
    h->battery     = AETHER_PLAYER_START_BATTERY;
    h->max_battery = AETHER_PLAYER_MAX_BATTERY;
    h->has_suit    = false;
    h->flashlight_on = false;
    h->dead        = false;
    aether_log(AETHER_LOG_INFO, "health", "player health initialized (hp=%.0f)",
               h->health);
}

void aether_player_health_reset(aether_player_health_t *h) {
    if (!h) return;
    h->health = AETHER_PLAYER_START_HEALTH;
    h->armor  = AETHER_PLAYER_START_ARMOR;
    h->battery = AETHER_PLAYER_START_BATTERY;
    h->dead = false;
    h->flashlight_on = false;
}

void aether_player_health_heal(aether_player_health_t *h, f32 amount) {
    if (!h || amount <= 0) return;
    h->health += amount;
    if (h->health > h->max_health) h->health = h->max_health;
    aether_log(AETHER_LOG_DEBUG, "health", "healed %.1f -> hp=%.1f",
               amount, h->health);
}

void aether_player_health_damage(aether_player_health_t *h, f32 amount) {
    if (!h || amount <= 0 || h->dead) return;

    /* Armor absorbs 50% if battery has power (GoldSrc style) */
    if (h->armor > 0.0f) {
        f32 absorbed = amount * 0.5f;
        if (absorbed > h->armor) absorbed = h->armor;
        h->armor -= absorbed;
        amount   -= absorbed;
        aether_log(AETHER_LOG_DEBUG, "health",
                   "armor absorbed %.1f (armor=%.1f)", absorbed, h->armor);
    }

    h->health -= amount;
    aether_log(AETHER_LOG_DEBUG, "health", "took %.1f damage -> hp=%.1f",
               amount, h->health);

    if (h->health <= 0.0f) {
        h->health = 0.0f;
        h->dead = true;
        aether_log(AETHER_LOG_INFO, "health", "player DIED");
    }
}

void aether_player_health_add_armor(aether_player_health_t *h, f32 amount) {
    if (!h || amount <= 0) return;
    h->armor += amount;
    if (h->armor > h->max_armor) h->armor = h->max_armor;
}

void aether_player_health_add_battery(aether_player_health_t *h, f32 amount) {
    if (!h || amount <= 0) return;
    h->battery += amount;
    if (h->battery > h->max_battery) h->battery = h->max_battery;
}

void aether_player_health_use_battery(aether_player_health_t *h, f32 amount) {
    if (!h || amount <= 0) return;
    h->battery -= amount;
    if (h->battery < 0.0f) h->battery = 0.0f;
}

bool aether_player_health_is_alive(const aether_player_health_t *h) {
    return h && !h->dead && h->health > 0.0f;
}

bool aether_player_health_is_dead(const aether_player_health_t *h) {
    return h && h->dead;
}

f32 aether_player_health_get(const aether_player_health_t *h) {
    return h ? h->health : 0.0f;
}

f32 aether_player_health_get_armor(const aether_player_health_t *h) {
    return h ? h->armor : 0.0f;
}

f32 aether_player_health_get_battery(const aether_player_health_t *h) {
    return h ? h->battery : 0.0f;
}

void aether_player_health_give_suit(aether_player_health_t *h) {
    if (!h) return;
    h->has_suit = true;
    aether_log(AETHER_LOG_INFO, "health", "HEV suit acquired");
}

void aether_player_health_toggle_flashlight(aether_player_health_t *h) {
    if (!h || !h->has_suit) return;
    h->flashlight_on = !h->flashlight_on;
    aether_log(AETHER_LOG_INFO, "health", "flashlight %s",
               h->flashlight_on ? "ON" : "OFF");
}

void aether_player_health_dump(const aether_player_health_t *h) {
    if (!h) return;
    aether_log(AETHER_LOG_INFO, "health",
               "HP=%.0f  AR=%.0f  BAT=%.0f  SUIT=%s  DEAD=%s",
               h->health, h->armor, h->battery,
               h->has_suit ? "yes" : "no",
               h->dead ? "yes" : "no");
}
