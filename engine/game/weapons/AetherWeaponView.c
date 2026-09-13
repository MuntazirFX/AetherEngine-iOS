/* AetherWeaponView.c — Viewmodel animation implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWeaponView.h"
#include <math.h>
#include <string.h>

void aether_weapon_view_init(aether_weapon_view_t *v, aether_weapon_id_t id) {
    if (!v) return;
    memset(v, 0, sizeof *v);
    v->weapon = id;
    v->current_anim = AETHER_VIEW_ANIM_IDLE;
    v->anim_length = 1.0f;
    v->anim_looping = true;
    v->offset = (aether_vec3_t){ 0, 0, 0 };
    v->angles = (aether_vec3_t){ 0, 0, 0 };
    aether_log(AETHER_LOG_INFO, "viewmodel", "initialized for weapon %d", (int)id);
}

void aether_weapon_view_play(aether_weapon_view_t *v, aether_view_anim_t anim) {
    if (!v) return;
    v->current_anim = anim;
    v->anim_time = 0.0f;

    switch (anim) {
        case AETHER_VIEW_ANIM_IDLE:    v->anim_length = 1.5f; v->anim_looping = true;  break;
        case AETHER_VIEW_ANIM_FIRE:    v->anim_length = 0.3f; v->anim_looping = false; break;
        case AETHER_VIEW_ANIM_RELOAD:  v->anim_length = 2.5f; v->anim_looping = false; break;
        case AETHER_VIEW_ANIM_DRAW:    v->anim_length = 0.5f; v->anim_looping = false; break;
        case AETHER_VIEW_ANIM_HOLSTER: v->anim_length = 0.5f; v->anim_looping = false; break;
    }

    aether_log(AETHER_LOG_DEBUG, "viewmodel", "playing anim %d (%.1fs)",
               (int)anim, v->anim_length);
}

void aether_weapon_view_tick(aether_weapon_view_t *v, f32 dt,
                              f32 player_speed, bool on_ground) {
    if (!v) return;

    /* Advance animation */
    v->anim_time += dt;
    if (v->anim_looping) {
        if (v->anim_time >= v->anim_length) v->anim_time = 0.0f;
    } else {
        if (v->anim_time >= v->anim_length) {
            v->current_anim = AETHER_VIEW_ANIM_IDLE;
            v->anim_time = 0.0f;
            v->anim_length = 1.5f;
            v->anim_looping = true;
        }
    }

    /* Walk bob */
    if (on_ground && player_speed > 10.0f) {
        v->bob_phase += dt * player_speed * 0.02f;
        v->bob_amount = 0.5f;
    } else {
        v->bob_amount *= (1.0f - 2.0f * dt);
        if (v->bob_amount < 0.0f) v->bob_amount = 0.0f;
    }
}

void aether_weapon_view_compute_transform(const aether_weapon_view_t *v,
                                           aether_vec3_t eye_pos,
                                           aether_vec3_t eye_angles,
                                           aether_vec3_t *out_pos,
                                           aether_vec3_t *out_angles) {
    if (!v || !out_pos || !out_angles) return;

    /* Start at eye position */
    aether_vec3_t pos = eye_pos;
    pos.z -= 6.0f;   /* drop a bit */

    /* Walk bob */
    f32 bx = sinf(v->bob_phase)         * v->bob_amount * 0.5f;
    f32 by = sinf(v->bob_phase * 2.0f)  * v->bob_amount * 0.3f;
    pos.x += bx;
    pos.y += by;

    *out_pos = pos;
    *out_angles = eye_angles;
}

void aether_weapon_view_dump(const aether_weapon_view_t *v) {
    if (!v) return;
    aether_log(AETHER_LOG_INFO, "viewmodel",
               "weapon=%d anim=%d t=%.2f/%.2f bob=%.2f",
               (int)v->weapon, (int)v->current_anim,
               v->anim_time, v->anim_length, v->bob_amount);
}
