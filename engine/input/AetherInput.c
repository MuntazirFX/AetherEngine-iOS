#include "AetherInput.h"
#include <stdlib.h>
#include <string.h>

struct aether_input {
    aether_input_state_t current;
    aether_input_state_t previous;
    f32                  prev_look_dx;
    f32                  prev_look_dy;
};

aether_input_t *aether_input_create(void) {
    aether_input_t *in = (aether_input_t*)calloc(1, sizeof *in);
    if (in) aether_log(AETHER_LOG_INFO, "input", "input system created");
    return in;
}

void aether_input_destroy(aether_input_t *in) {
    if (!in) return;
    free(in);
    aether_log(AETHER_LOG_INFO, "input", "input system destroyed");
}

void aether_input_begin_frame(aether_input_t *in) {
    if (!in) return;
    /* Snapshot for just_pressed detection. */
    in->previous = in->current;
    in->prev_look_dx = in->current.look_dx;
    in->prev_look_dy = in->current.look_dy;

    /* Clear per-frame deltas. Held states remain. */
    in->current.look_dx = 0.0f;
    in->current.look_dy = 0.0f;
}

void aether_input_end_frame(aether_input_t *in) {
    (void)in; /* reserved for future input smoothing/deadzone */
}

void aether_input_set_action(aether_input_t *in, aether_input_action_t action, bool pressed) {
    if (!in || action < 0 || action >= AETHER_ACTION_COUNT) return;
    in->current.actions[action] = pressed;
}

void aether_input_set_move(aether_input_t *in, f32 x, f32 y) {
    if (!in) return;
    /* Clamp to unit range. */
    if (x < -1.0f) x = -1.0f; else if (x > 1.0f) x = 1.0f;
    if (y < -1.0f) y = -1.0f; else if (y > 1.0f) y = 1.0f;
    in->current.move_x = x;
    in->current.move_y = y;
}

void aether_input_add_look(aether_input_t *in, f32 dx, f32 dy) {
    if (!in) return;
    in->current.look_dx += dx;
    in->current.look_dy += dy;
}

const aether_input_state_t *aether_input_state(const aether_input_t *in) {
    return in ? &in->current : NULL;
}

bool aether_input_held(const aether_input_t *in, aether_input_action_t action) {
    if (!in || action < 0 || action >= AETHER_ACTION_COUNT) return false;
    return in->current.actions[action];
}

bool aether_input_just_pressed(const aether_input_t *in, aether_input_action_t action) {
    if (!in || action < 0 || action >= AETHER_ACTION_COUNT) return false;
    return in->current.actions[action] && !in->previous.actions[action];
}
