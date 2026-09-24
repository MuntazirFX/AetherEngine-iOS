/* AetherPlayer.c — Player movement with gravity + collision + swim + fall impact.
 * Air drain/recover sets drowning; host/bridge applies tick_drown + fall damage.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherPlayer.h"
#include "AetherPlayerDamage.h"
#include <math.h>
#include <string.h>

#define MOVE_SPEED           320.0f
#define CROUCH_SPEED_SCALE   0.4f
#define DEFAULT_EYE_HEIGHT   28.0f
#define CROUCH_EYE_HEIGHT    12.0f
#define LOOK_SPEED           0.0035f
#define PITCH_LIMIT          (89.0f * (3.14159265f / 180.0f))

#define GRAVITY             800.0f    /* GoldSrc default */
#define JUMP_SPEED          270.0f    /* GoldSrc default */
#define GROUND_FRICTION     10.0f
#define AIR_ACCEL           10.0f
#define STOP_SPEED          100.0f
#define WATER_SPEED_SCALE   0.55f
#define WATER_FRICTION      4.0f
#define WATER_GRAVITY_SCALE 0.25f
#define WATER_BUOYANCY      220.0f    /* upward accel while submerged */
#define SWIM_VERT_SPEED     180.0f
#define AIR_RECOVER_SCALE   2.0f      /* air refill rate vs drain */
#define VIEW_PUNCH_DECAY    8.0f      /* punch pitch → 0 per second */
#define FALL_PUNCH_SCALE    0.0025f   /* radians per damage point */

static bool point_in_water(const aether_collision_t *collision,
                           aether_vec3_t point,
                           i32 hull_index) {
    if (!collision) return false;
    return aether_collision_point_contents(collision, point, hull_index)
        == AETHER_CONTENTS_WATER;
}

i32 aether_player_sample_waterlevel(const aether_collision_t *collision,
                                    aether_vec3_t feet,
                                    f32 eye_height,
                                    i32 hull_index) {
    if (!collision) return AETHER_WATERLEVEL_DRY;
    if (eye_height < 1.0f) eye_height = DEFAULT_EYE_HEIGHT;

    aether_vec3_t eye = { feet.x, feet.y, feet.z + eye_height };
    aether_vec3_t waist = { feet.x, feet.y, feet.z + eye_height * 0.5f };

    /* Highest submerged sample wins (eye > waist > feet). */
    if (point_in_water(collision, eye, hull_index))
        return AETHER_WATERLEVEL_EYE;
    if (point_in_water(collision, waist, hull_index))
        return AETHER_WATERLEVEL_WAIST;
    if (point_in_water(collision, feet, hull_index))
        return AETHER_WATERLEVEL_FEET;
    return AETHER_WATERLEVEL_DRY;
}

static void player_refresh_water(aether_player_t *p,
                                 aether_collision_t *collision) {
    i32 prev = p->waterlevel;
    i32 lvl = aether_player_sample_waterlevel(collision, p->position,
                                              p->eye_height, p->hull_index);
    p->waterlevel = lvl;
    p->in_water = (lvl >= AETHER_WATERLEVEL_FEET);

    /* Enter/exit splash on dry <-> wet transitions (not tier changes). */
    bool was_wet = (prev >= AETHER_WATERLEVEL_FEET);
    bool now_wet = (lvl >= AETHER_WATERLEVEL_FEET);
    if (!was_wet && now_wet)
        p->splash_event = AETHER_SPLASH_ENTER;
    else if (was_wet && !now_wet)
        p->splash_event = AETHER_SPLASH_EXIT;
}

static void player_tick_air(aether_player_t *p, f32 dt) {
    if (dt <= 0.0f) return;
    bool eye_under = (p->waterlevel >= AETHER_WATERLEVEL_EYE);
    if (eye_under) {
        p->air -= dt;
        if (p->air < 0.0f) p->air = 0.0f;
        p->drowning = (p->air <= 0.0f);
    } else {
        p->air += dt * AIR_RECOVER_SCALE;
        if (p->air > p->air_max) p->air = p->air_max;
        p->drowning = false;
    }
}

void aether_player_init(aether_player_t *p) {
    if (!p) return;
    memset(p, 0, sizeof *p);
    p->position    = (aether_vec3_t){ 0, 0, 0 };
    p->velocity    = (aether_vec3_t){ 0, 0, 0 };
    p->yaw         = 0.0f;
    p->pitch       = 0.0f;
    p->eye_height  = DEFAULT_EYE_HEIGHT;
    p->move_speed  = MOVE_SPEED;
    p->look_speed  = LOOK_SPEED;
    p->jump_speed  = JUMP_SPEED;
    p->gravity     = GRAVITY;
    p->on_ground   = false;
    p->crouching   = false;
    p->in_water    = false;
    p->waterlevel  = AETHER_WATERLEVEL_DRY;
    p->air_max     = AETHER_PLAYER_AIR_MAX;
    p->air         = AETHER_PLAYER_AIR_MAX;
    p->drowning    = false;
    p->splash_event = AETHER_SPLASH_NONE;
    p->fall_velocity_z = 0.0f;
    p->pending_fall_damage = 0.0f;
    p->view_punch_pitch = 0.0f;
    p->hull_index  = 1;
    p->step_height = AETHER_DEFAULT_STEP_HEIGHT;
}

void aether_player_set_position(aether_player_t *p, aether_vec3_t pos) {
    if (!p) return;
    p->position = pos;
    p->velocity = (aether_vec3_t){ 0, 0, 0 };
    p->on_ground = false;
    /* Position teleports clear pending splash / fall; waterlevel re-sampled on update. */
    p->splash_event = AETHER_SPLASH_NONE;
    p->fall_velocity_z = 0.0f;
    p->pending_fall_damage = 0.0f;
}

aether_vec3_t aether_player_eye_position(const aether_player_t *p) {
    return (aether_vec3_t){ p->position.x, p->position.y, p->position.z + p->eye_height };
}

aether_vec3_t aether_player_forward(const aether_player_t *p) {
    f32 cy = cosf(p->yaw), sy = sinf(p->yaw);
    f32 cp = cosf(p->pitch), sp = sinf(p->pitch);
    return (aether_vec3_t){ cy * cp, sy * cp, sp };
}

aether_vec3_t aether_player_right(const aether_player_t *p) {
    f32 cy = cosf(p->yaw), sy = sinf(p->yaw);
    return (aether_vec3_t){ -sy, cy, 0 };
}

i32 aether_player_waterlevel(const aether_player_t *p) {
    return p ? p->waterlevel : AETHER_WATERLEVEL_DRY;
}

bool aether_player_eye_underwater(const aether_player_t *p) {
    return p && p->waterlevel >= AETHER_WATERLEVEL_EYE;
}

f32 aether_player_air(const aether_player_t *p) {
    return p ? p->air : 0.0f;
}

bool aether_player_is_drowning(const aether_player_t *p) {
    return p && p->drowning;
}

i32 aether_player_take_splash_event(aether_player_t *p) {
    if (!p) return AETHER_SPLASH_NONE;
    i32 ev = p->splash_event;
    p->splash_event = AETHER_SPLASH_NONE;
    return ev;
}

void aether_player_trigger_splash(aether_player_t *p, i32 splash_kind) {
    if (!p) return;
    if (splash_kind == AETHER_SPLASH_ENTER || splash_kind == AETHER_SPLASH_EXIT)
        p->splash_event = splash_kind;
}

f32 aether_player_take_fall_damage(aether_player_t *p) {
    if (!p) return 0.0f;
    f32 dmg = p->pending_fall_damage;
    p->pending_fall_damage = 0.0f;
    return dmg;
}

f32 aether_player_fall_velocity(const aether_player_t *p) {
    return p ? p->fall_velocity_z : 0.0f;
}

f32 aether_player_view_punch_pitch(const aether_player_t *p) {
    return p ? p->view_punch_pitch : 0.0f;
}

void aether_player_update(aether_player_t *p,
                          const aether_input_state_t *in,
                          aether_collision_t *collision,
                          f32 dt) {
    if (!p || !in) return;

    /* 1. Look --- */
    p->yaw   -= in->look_dx * p->look_speed;
    p->pitch -= in->look_dy * p->look_speed;
    if (p->pitch >  PITCH_LIMIT) p->pitch =  PITCH_LIMIT;
    if (p->pitch < -PITCH_LIMIT) p->pitch = -PITCH_LIMIT;

    /* 2. Crouch / duck --- hull 2 is a shorter Z AABB than standing. */
    bool want_crouch = in->actions[AETHER_ACTION_DUCK];
    if (want_crouch && !p->crouching) {
        p->crouching = true;
        p->hull_index = 2;
    } else if (!want_crouch && p->crouching) {
        /* Stand up only if standing hull fits at current feet (ceiling headroom). */
        bool blocked = false;
        if (collision)
            blocked = aether_collision_point_in_solid(collision, p->position, 1);
        if (!blocked) {
            p->crouching = false;
            p->hull_index = 1;
        }
    }
    f32 target_eye = p->crouching ? CROUCH_EYE_HEIGHT : DEFAULT_EYE_HEIGHT;
    p->eye_height += (target_eye - p->eye_height) * 10.0f * dt;

    /* 3. Horizontal wish direction (camera-relative, no pitch) --- */
    aether_vec3_t fwd = aether_player_forward(p);
    fwd.z = 0;
    if (aether_vec3_len(fwd) > 1e-4f) fwd = aether_vec3_normalize(fwd);
    aether_vec3_t right = aether_player_right(p);

    aether_vec3_t wish = { 0, 0, 0 };
    wish = aether_vec3_add(wish, aether_vec3_scale(fwd,   in->move_y));
    wish = aether_vec3_add(wish, aether_vec3_scale(right, in->move_x));
    f32 wl = aether_vec3_len(wish);
    if (wl > 1.0f) wish = aether_vec3_scale(wish, 1.0f / wl);

    /* 3b. Waterlevel at feet / waist / eye --- */
    player_refresh_water(p, collision);
    bool in_water = p->in_water;

    f32 speed = p->move_speed * (p->crouching ? CROUCH_SPEED_SCALE : 1.0f);
    if (in_water) speed *= WATER_SPEED_SCALE;

    /* 4. Ground / air / swim acceleration --- */
    if (in_water) {
        /* Water friction + swim toward wish (incl. vertical via jump/duck). */
        f32 f = WATER_FRICTION * dt;
        if (f > 1.0f) f = 1.0f;
        p->velocity.x *= (1.0f - f);
        p->velocity.y *= (1.0f - f);
        p->velocity.z *= (1.0f - f * 0.5f);

        f32 accel = speed * 5.0f;
        p->velocity.x += wish.x * accel * dt;
        p->velocity.y += wish.y * accel * dt;

        if (in->actions[AETHER_ACTION_JUMP])
            p->velocity.z += SWIM_VERT_SPEED * 4.0f * dt;
        if (in->actions[AETHER_ACTION_DUCK])
            p->velocity.z -= SWIM_VERT_SPEED * 4.0f * dt;

        f32 hs = sqrtf(p->velocity.x*p->velocity.x + p->velocity.y*p->velocity.y);
        if (hs > speed) {
            f32 k = speed / hs;
            p->velocity.x *= k;
            p->velocity.y *= k;
        }
        if (p->velocity.z > SWIM_VERT_SPEED) p->velocity.z = SWIM_VERT_SPEED;
        if (p->velocity.z < -SWIM_VERT_SPEED) p->velocity.z = -SWIM_VERT_SPEED;
    } else if (p->on_ground) {
        /* Friction when not moving */
        if (wl < 0.01f) {
            f32 f = GROUND_FRICTION * dt;
            if (f > 1.0f) f = 1.0f;
            p->velocity.x *= (1.0f - f);
            p->velocity.y *= (1.0f - f);
        }
        /* Instant-ish velocity toward wish dir */
        f32 accel = speed * 6.0f;
        p->velocity.x += wish.x * accel * dt;
        p->velocity.y += wish.y * accel * dt;

        /* Cap horizontal speed */
        f32 hs = sqrtf(p->velocity.x*p->velocity.x + p->velocity.y*p->velocity.y);
        if (hs > speed) {
            f32 k = speed / hs;
            p->velocity.x *= k;
            p->velocity.y *= k;
        }
    } else {
        /* Air: small accel, no friction */
        f32 accel = speed * AIR_ACCEL * dt;
        p->velocity.x += wish.x * accel;
        p->velocity.y += wish.y * accel;
        f32 hs = sqrtf(p->velocity.x*p->velocity.x + p->velocity.y*p->velocity.y);
        if (hs > speed) {
            f32 k = speed / hs;
            p->velocity.x *= k;
            p->velocity.y *= k;
        }
    }

    /* 5. Jump (dry land only — water uses swim-up above) --- */
    if (!in_water && p->on_ground && in->actions[AETHER_ACTION_JUMP]) {
        p->velocity.z = p->jump_speed;
        p->on_ground = false;
    }

    /* 6. Gravity / buoyancy --- */
    if (in_water) {
        p->velocity.z -= p->gravity * WATER_GRAVITY_SCALE * dt;
        p->velocity.z += WATER_BUOYANCY * dt; /* net slight float when idle */
    } else {
        p->velocity.z -= p->gravity * dt;
    }
    if (p->velocity.z < -2000.0f) p->velocity.z = -2000.0f;

    /* 7. Integrate + collision --- */
    bool was_ground = p->on_ground;
    /* Track peak downward speed while airborne (before ground clamps velocity). */
    if (!was_ground && p->velocity.z < p->fall_velocity_z)
        p->fall_velocity_z = p->velocity.z;

    aether_vec3_t target;
    target.x = p->position.x + p->velocity.x * dt;
    target.y = p->position.y + p->velocity.y * dt;
    target.z = p->position.z + p->velocity.z * dt;

    aether_vec3_t final = target;
    bool on_ground = false;
    f32 step = in_water ? 0.0f : p->step_height; /* no auto-step while swimming */
    if (collision) {
        final = aether_collision_move(collision, p->position, target,
                                      p->hull_index, step, &on_ground);
    }
    p->position = final;
    p->on_ground = on_ground;

    /* Re-sample waterlevel after move (may have exited/entered). */
    player_refresh_water(p, collision);
    player_tick_air(p, dt);

    /* 7b. Fall impact on land / soft water entry --- */
    {
        f32 impact_vz = p->fall_velocity_z;
        if (p->velocity.z < impact_vz)
            impact_vz = p->velocity.z;

        bool landed = !was_ground && on_ground;
        bool water_soft = (p->waterlevel >= AETHER_WATERLEVEL_FEET);

        if (landed) {
            if (water_soft) {
                /* Wade/swim/under landings: no fall damage. */
                p->pending_fall_damage = 0.0f;
            } else {
                f32 dmg = aether_player_calc_fall_damage(impact_vz);
                p->pending_fall_damage = dmg;
                if (dmg > 0.0f) {
                    /* Optional view punch (look down on hard land). */
                    f32 punch = dmg * FALL_PUNCH_SCALE;
                    if (punch > 0.35f) punch = 0.35f;
                    p->view_punch_pitch -= punch;
                }
            }
            p->fall_velocity_z = 0.0f;
        } else if (!was_ground && water_soft && impact_vz < -1.0f) {
            /* Entered water while falling: soft — clear tracked speed, no HP hit. */
            p->pending_fall_damage = 0.0f;
            p->fall_velocity_z = 0.0f;
        } else if (on_ground) {
            p->fall_velocity_z = 0.0f;
        }
    }

    /* If blocked along an axis, kill velocity along it (basic) */
    if (fabsf(final.x - target.x) > 1e-3f) p->velocity.x = 0.0f;
    if (fabsf(final.y - target.y) > 1e-3f) p->velocity.y = 0.0f;
    if (on_ground && p->velocity.z < 0.0f) p->velocity.z = 0.0f;
    /* Ceiling clamp: upward move stopped by hull headroom. */
    if (p->velocity.z > 0.0f && (final.z + 1e-3f) < target.z)
        p->velocity.z = 0.0f;

    /* View punch decay (GoldSrc-style punchangle stub). */
    if (p->view_punch_pitch != 0.0f && dt > 0.0f) {
        f32 k = VIEW_PUNCH_DECAY * dt;
        if (k > 1.0f) k = 1.0f;
        p->view_punch_pitch *= (1.0f - k);
        if (fabsf(p->view_punch_pitch) < 1e-4f)
            p->view_punch_pitch = 0.0f;
    }
}
