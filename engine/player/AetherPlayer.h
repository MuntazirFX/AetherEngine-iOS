/* AetherPlayer.h — First-person player with gravity + collision + fall impact.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_PLAYER_H
#define AETHER_PLAYER_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"
#include "../input/AetherInput.h"
#include "AetherCollision.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Waterlevel tiers (GoldSrc-style). Aliases: dry / wade / swim / under. */
#define AETHER_WATERLEVEL_DRY    0  /* dry  — no sample in water */
#define AETHER_WATERLEVEL_FEET   1  /* wade — feet only */
#define AETHER_WATERLEVEL_WAIST  2  /* swim — waist submerged */
#define AETHER_WATERLEVEL_EYE    3  /* under — eyes underwater */

#define AETHER_SPLASH_NONE   0
#define AETHER_SPLASH_ENTER  1
#define AETHER_SPLASH_EXIT   2

/* Seconds of breath before drowning flag trips (eye underwater). */
#define AETHER_PLAYER_AIR_MAX  12.0f

typedef struct aether_player {
    aether_vec3_t position;      /* feet center, in GoldSrc units */
    aether_vec3_t velocity;      /* units / second */
    f32           yaw;
    f32           pitch;
    f32           eye_height;
    f32           move_speed;
    f32           look_speed;
    f32           jump_speed;    /* initial upward velocity on jump */
    f32           gravity;       /* downward accel */
    bool          on_ground;
    bool          crouching;
    bool          in_water;      /* waterlevel >= FEET (CONTENTS_WATER) */
    i32           waterlevel;    /* AETHER_WATERLEVEL_* */
    f32           air;           /* breath remaining (0..air_max) */
    f32           air_max;
    bool          drowning;      /* air depleted while eye underwater → feed tick_drown */
    i32           splash_event;  /* pending ENTER/EXIT; consume via take_splash */
    f32           fall_velocity_z;   /* most-negative Z vel while airborne (impact speed) */
    f32           pending_fall_damage; /* set on dry land impact; consume via take_fall */
    f32           view_punch_pitch;  /* additive pitch punch (radians); decays toward 0 */
    bool          on_fire;       /* hazard: feed aether_player_tick_fire */
    bool          in_radiation;  /* hazard: feed aether_player_tick_radiation */
    i32           hull_index;    /* 1 or 2 */
    f32           step_height;   /* max auto-step (Quake 18); 0 disables */
} aether_player_t;

void aether_player_init(aether_player_t *p);
void aether_player_set_position(aether_player_t *p, aether_vec3_t pos);
void aether_player_update(aether_player_t *p,
                          const aether_input_state_t *in,
                          aether_collision_t *collision,
                          f32 dt);

aether_vec3_t aether_player_eye_position(const aether_player_t *p);
aether_vec3_t aether_player_forward     (const aether_player_t *p);
aether_vec3_t aether_player_right       (const aether_player_t *p);

/* Sample CONTENTS_WATER at feet / waist / eye → AETHER_WATERLEVEL_*. */
i32  aether_player_sample_waterlevel(const aether_collision_t *collision,
                                     aether_vec3_t feet,
                                     f32 eye_height,
                                     i32 hull_index);

i32  aether_player_waterlevel(const aether_player_t *p);
bool aether_player_eye_underwater(const aether_player_t *p);
f32  aether_player_air(const aether_player_t *p);
bool aether_player_is_drowning(const aether_player_t *p);
/* Returns pending splash (ENTER/EXIT/NONE) and clears it. */
i32  aether_player_take_splash_event(aether_player_t *p);
/* Manually queue a splash event (bridge / host tests). */
void aether_player_trigger_splash(aether_player_t *p, i32 splash_kind);

/* Consume pending fall damage from last land impact (0 if none / soft). */
f32  aether_player_take_fall_damage(aether_player_t *p);
/* Peak downward speed tracked while last airborne (negative Z). */
f32  aether_player_fall_velocity(const aether_player_t *p);
/* Additive view punch pitch (radians); decays in update. */
f32  aether_player_view_punch_pitch(const aether_player_t *p);

void aether_player_set_on_fire(aether_player_t *p, bool on_fire);
bool aether_player_is_on_fire(const aether_player_t *p);
void aether_player_set_in_radiation(aether_player_t *p, bool in_rad);
bool aether_player_is_in_radiation(const aether_player_t *p);
/* Sample collision contents at feet: LAVA→on_fire, SLIME→radiation. */
void aether_player_refresh_hazards(aether_player_t *p, const aether_collision_t *collision);

#ifdef __cplusplus
}
#endif
#endif
