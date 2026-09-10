#include "AetherTouch.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct aether_touch {
    aether_touch_state_t state;
};

aether_touch_t *aether_touch_create(void) {
    aether_touch_t *t = (aether_touch_t*)calloc(1, sizeof *t);
    if (!t) return NULL;
    aether_touch_apply_layout(t, AETHER_TOUCH_LAYOUT_RIGHT_HANDED);
    aether_log(AETHER_LOG_INFO, "touch", "touch system created (default: right-handed)");
    return t;
}

void aether_touch_destroy(aether_touch_t *t) {
    if (!t) return;
    free(t);
}

i32 aether_touch_add_button(aether_touch_t *t, aether_input_action_t action,
                            f32 x, f32 y, f32 size, f32 opacity) {
    if (!t) return -1;
    if (t->state.button_count >= AETHER_TOUCH_MAX_BUTTONS) return -1;

    aether_touch_button_t *b = &t->state.buttons[t->state.button_count];
    b->action  = action;
    b->x       = x;
    b->y       = y;
    b->size    = size;
    b->opacity = opacity;
    b->visible = true;

    return (i32)t->state.button_count++;
}

bool aether_touch_set_button(aether_touch_t *t, u32 index,
                             f32 x, f32 y, f32 size, f32 opacity) {
    if (!t || index >= t->state.button_count) return false;
    aether_touch_button_t *b = &t->state.buttons[index];
    b->x = x; b->y = y; b->size = size; b->opacity = opacity;
    return true;
}

void aether_touch_apply_layout(aether_touch_t *t, aether_touch_layout_t layout) {
    if (!t) return;
    if (layout >= AETHER_TOUCH_LAYOUT_COUNT) layout = AETHER_TOUCH_LAYOUT_RIGHT_HANDED;

    t->state.layout = layout;
    t->state.button_count = 0;

    /* Common button layout; only joystick + a couple of action keys swap sides. */
    bool right_handed = (layout != AETHER_TOUCH_LAYOUT_LEFT_HANDED);

    /* Joystick */
    t->state.joystick_x      = right_handed ? 0.20f : 0.80f;
    t->state.joystick_y      = 0.75f;
    t->state.joystick_radius = 0.12f;
    t->state.joystick_knob_x = t->state.joystick_x;
    t->state.joystick_knob_y = t->state.joystick_y;
    t->state.joystick_active = false;

    /* Look area (right/left half depending on handedness) */
    f32 look_x = right_handed ? 0.75f : 0.25f;
    (void)look_x; /* rendered as an invisible gesture area in SwiftUI */

    /* Action buttons: mirrored for left-handed */
    f32 cx = right_handed ? 0.82f : 0.18f;   /* right-side cluster center */
    f32 cy = 0.78f;

    aether_touch_add_button(t, AETHER_ACTION_FIRE,   cx,          cy - 0.12f, 0.11f, 0.85f);
    aether_touch_add_button(t, AETHER_ACTION_JUMP,   cx - 0.12f,  cy,        0.10f, 0.75f);
    aether_touch_add_button(t, AETHER_ACTION_DUCK,   cx + 0.10f,  cy + 0.02f, 0.10f, 0.75f);
    aether_touch_add_button(t, AETHER_ACTION_USE,    cx - 0.10f,  cy - 0.12f, 0.09f, 0.75f);
    aether_touch_add_button(t, AETHER_ACTION_RELOAD, cx + 0.02f,  cy - 0.20f, 0.09f, 0.70f);

    aether_log(AETHER_LOG_INFO, "touch", "layout applied: %s",
               right_handed ? "right-handed" : "left-handed");
}

aether_touch_layout_t aether_touch_get_layout(const aether_touch_t *t) {
    return t ? t->state.layout : AETHER_TOUCH_LAYOUT_RIGHT_HANDED;
}

const aether_touch_state_t *aether_touch_get_state(const aether_touch_t *t) {
    return t ? &t->state : NULL;
}

size_t aether_touch_serialize(const aether_touch_t *t, char *out, size_t cap) {
    if (!t || !out || cap == 0) return 0;
    size_t used = 0;
    int n = snprintf(out + used, cap - used, "layout=%d;",
                     (int)t->state.layout);
    if (n < 0 || (size_t)n >= cap - used) return used;
    used += (size_t)n;

    for (u32 i = 0; i < t->state.button_count; ++i) {
        const aether_touch_button_t *b = &t->state.buttons[i];
        n = snprintf(out + used, cap - used, "b%d=%d,%.4f,%.4f,%.4f,%.4f;",
                     (int)i, (int)b->action, b->x, b->y, b->size, b->opacity);
        if (n < 0 || (size_t)n >= cap - used) break;
        used += (size_t)n;
    }
    return used;
}
