/* AetherPlayer.c — Player movement with gravity + collision + swim.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherPlayer.h"
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
    p->hull_index  = 1;
    p->step_height = AETHER_DEFAULT_STEP_HEIGHT;
}

void aether_player_set_position(aether_player_t *p, aether_vec3_t pos) {
    if (!p) return;
    p->position = pos;
    p->velocity = (aether_vec3_t){ 0, 0, 0 };
    p->on_ground = false;
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

    /* 3b. Water contents at feet --- */
    bool in_water = false;
    if (collision) {
        i32 contents = aether_collision_point_contents(collision, p->position, p->hull_index);
        in_water = (contents == AETHER_CONTENTS_WATER);
    }
    p->in_water = in_water;

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

    /* Re-sample water after move (may have exited/entered). */
    if (collision) {
        i32 contents = aether_collision_point_contents(collision, p->position, p->hull_index);
        p->in_water = (contents == AETHER_CONTENTS_WATER);
    }

    /* If blocked along an axis, kill velocity along it (basic) */
    if (fabsf(final.x - target.x) > 1e-3f) p->velocity.x = 0.0f;
    if (fabsf(final.y - target.y) > 1e-3f) p->velocity.y = 0.0f;
    if (on_ground && p->velocity.z < 0.0f) p->velocity.z = 0.0f;
    /* Ceiling clamp: upward move stopped by hull headroom. */
    if (p->velocity.z > 0.0f && (final.z + 1e-3f) < target.z)
        p->velocity.z = 0.0f;
}
