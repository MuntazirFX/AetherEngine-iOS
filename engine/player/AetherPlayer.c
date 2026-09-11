/* AetherPlayer.c — GoldSrc-style player movement (free-fly).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherPlayer.h"
#include <math.h>
#include <string.h>

#define DEFAULT_MOVE_SPEED 320.0f    /* GoldSrc "units" per second */
#define CROUCH_SPEED_SCALE 0.4f
#define DEFAULT_EYE_HEIGHT 28.0f
#define CROUCH_EYE_HEIGHT  12.0f
#define DEFAULT_LOOK_SPEED 0.0035f   /* rad per pixel-ish */
#define PITCH_LIMIT (89.0f * (3.14159265f / 180.0f))

void aether_player_init(aether_player_t *p) {
    if (!p) return;
    memset(p, 0, sizeof *p);
    p->position    = (aether_vec3_t){ 0, 0, 0 };
    p->velocity    = (aether_vec3_t){ 0, 0, 0 };
    p->yaw         = 0.0f;
    p->pitch       = 0.0f;
    p->eye_height  = DEFAULT_EYE_HEIGHT;
    p->move_speed  = DEFAULT_MOVE_SPEED;
    p->look_speed  = DEFAULT_LOOK_SPEED;
    p->on_ground   = true;
    p->crouching   = false;
}

void aether_player_set_position(aether_player_t *p, aether_vec3_t pos) {
    if (!p) return;
    p->position = pos;
    p->velocity = (aether_vec3_t){ 0, 0, 0 };
}

aether_vec3_t aether_player_eye_position(const aether_player_t *p) {
    return (aether_vec3_t){ p->position.x, p->position.y, p->position.z + p->eye_height };
}

aether_vec3_t aether_player_forward(const aether_player_t *p) {
    /* GoldSrc: forward is +X when yaw=0. Camera convention in Metal view
       uses a right-handed system, but our math stays simple: forward is
       computed from yaw/pitch as a 3D unit vector. */
    f32 cy = cosf(p->yaw);
    f32 sy = sinf(p->yaw);
    f32 cp = cosf(p->pitch);
    f32 sp = sinf(p->pitch);
    return (aether_vec3_t){ cy * cp, sy * cp, sp };
}

aether_vec3_t aether_player_right(const aether_player_t *p) {
    /* Perpendicular to forward in the horizontal plane. */
    f32 cy = cosf(p->yaw);
    f32 sy = sinf(p->yaw);
    return (aether_vec3_t){ -sy, cy, 0 };
}

void aether_player_update(aether_player_t *p,
                          const aether_input_state_t *in,
                          f32 dt) {
    if (!p || !in) return;

    /* 1. Look — from look deltas */
    p->yaw   -= in->look_dx * p->look_speed;
    p->pitch -= in->look_dy * p->look_speed;
    if (p->pitch >  PITCH_LIMIT) p->pitch =  PITCH_LIMIT;
    if (p->pitch < -PITCH_LIMIT) p->pitch = -PITCH_LIMIT;

    /* 2. Crouch state */
    p->crouching = aether_input_held(NULL, AETHER_ACTION_DUCK); /* unused path */
    /* Actually check via actions array directly: */
    p->crouching = in->actions[AETHER_ACTION_DUCK];
    f32 target_eye = p->crouching ? CROUCH_EYE_HEIGHT : DEFAULT_EYE_HEIGHT;
    p->eye_height += (target_eye - p->eye_height) * 8.0f * dt;
    if (p->eye_height < 0) p->eye_height = 0;

    /* 3. Movement — WASD in camera-relative direction */
    aether_vec3_t forward = aether_player_forward(p);
    aether_vec3_t right   = aether_player_right(p);

    /* Zero out the pitch component for horizontal movement */
    forward.z = 0;
    if (aether_vec3_len(forward) > 1e-4f) forward = aether_vec3_normalize(forward);

    f32 mx = in->move_x;   /* -1 .. 1 */
    f32 my = in->move_y;   /* -1 .. 1 */

    aether_vec3_t wish = (aether_vec3_t){ 0, 0, 0 };
    wish = aether_vec3_add(wish, aether_vec3_scale(forward, my));
    wish = aether_vec3_add(wish, aether_vec3_scale(right,   mx));

    f32 len = aether_vec3_len(wish);
    if (len > 1.0f) wish = aether_vec3_scale(wish, 1.0f / len);

    f32 speed = p->move_speed * (p->crouching ? CROUCH_SPEED_SCALE : 1.0f);

    /* Instant movement (no accel yet — GoldSrc-style air control comes later) */
    aether_vec3_t delta = aether_vec3_scale(wish, speed * dt);
    p->position = aether_vec3_add(p->position, delta);

    /* 4. Jump — no gravity yet, just nudge up a bit for feedback */
    if (in->actions[AETHER_ACTION_JUMP]) {
        /* Placeholder: STEP 14 will add proper gravity. */
        p->on_ground = false;
    }

    /* 5. Keep player from going infinitely far in Z (sanity) */
    if (p->position.z < -100000.0f) p->position.z = -100000.0f;
    if (p->position.z >  100000.0f) p->position.z =  100000.0f;
}
