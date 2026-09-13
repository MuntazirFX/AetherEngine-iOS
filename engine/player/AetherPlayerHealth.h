/* AetherPlayerHealth.h — Player health, armor, HEV suit systems.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_PLAYER_HEALTH_H
#define AETHER_PLAYER_HEALTH_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_PLAYER_MAX_HEALTH   100.0f
#define AETHER_PLAYER_MAX_ARMOR    100.0f
#define AETHER_PLAYER_MAX_BATTERY  100.0f
#define AETHER_PLAYER_START_HEALTH 100.0f
#define AETHER_PLAYER_START_ARMOR  0.0f
#define AETHER_PLAYER_START_BATTERY 0.0f

typedef struct aether_player_health {
    f32  health;
    f32  max_health;
    f32  armor;
    f32  max_armor;
    f32  battery;         /* HEV suit power */
    f32  max_battery;
    bool has_suit;        /* HEV suit owned */
    bool flashlight_on;
    bool dead;
} aether_player_health_t;

/* Lifecycle */
void aether_player_health_init(aether_player_health_t *h);
void aether_player_health_reset(aether_player_health_t *h);

/* Modify */
void aether_player_health_heal(aether_player_health_t *h, f32 amount);
void aether_player_health_damage(aether_player_health_t *h, f32 amount);
void aether_player_health_add_armor(aether_player_health_t *h, f32 amount);
void aether_player_health_add_battery(aether_player_health_t *h, f32 amount);
void aether_player_health_use_battery(aether_player_health_t *h, f32 amount);

/* Queries */
bool aether_player_health_is_alive(const aether_player_health_t *h);
bool aether_player_health_is_dead (const aether_player_health_t *h);
f32  aether_player_health_get(const aether_player_health_t *h);
f32  aether_player_health_get_armor(const aether_player_health_t *h);
f32  aether_player_health_get_battery(const aether_player_health_t *h);

/* Suit */
void aether_player_health_give_suit(aether_player_health_t *h);
void aether_player_health_toggle_flashlight(aether_player_health_t *h);

/* Diagnostic */
void aether_player_health_dump(const aether_player_health_t *h);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_PLAYER_HEALTH_H */
