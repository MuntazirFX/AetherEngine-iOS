/* AetherTouch.h — Virtual touch controls (joystick, buttons, layouts).
 * iOS-specific logic lives in Swift; this holds the layout data model.
 */
#ifndef AETHER_TOUCH_H
#define AETHER_TOUCH_H

#include "../core/AetherCore.h"
#include "AetherInput.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_TOUCH_MAX_BUTTONS 16

typedef enum aether_touch_layout {
    AETHER_TOUCH_LAYOUT_RIGHT_HANDED = 0,
    AETHER_TOUCH_LAYOUT_LEFT_HANDED,
    AETHER_TOUCH_LAYOUT_CUSTOM,
    AETHER_TOUCH_LAYOUT_COUNT
} aether_touch_layout_t;

/* A single on-screen button. Coordinates are normalized (0..1) relative to
 * the screen so that iPhone / iPad scaling is automatic. */
typedef struct aether_touch_button {
    aether_input_action_t action;
    f32                   x;         /* 0..1 (center) */
    f32                   y;         /* 0..1 (center) */
    f32                   size;      /* 0..1 (fraction of min(screen)) */
    f32                   opacity;   /* 0..1 */
    bool                  visible;
} aether_touch_button_t;

typedef struct aether_touch_state {
    aether_touch_layout_t layout;
    f32                   joystick_x;       /* center, 0..1 */
    f32                   joystick_y;
    f32                   joystick_radius;
    f32                   joystick_knob_x;  /* current knob pos, 0..1 */
    f32                   joystick_knob_y;
    bool                  joystick_active;

    aether_touch_button_t buttons[AETHER_TOUCH_MAX_BUTTONS];
    u32                   button_count;
} aether_touch_state_t;

typedef struct aether_touch aether_touch_t;

aether_touch_t *aether_touch_create(void);
void            aether_touch_destroy(aether_touch_t *t);

/* Layout presets */
void aether_touch_apply_layout(aether_touch_t *t, aether_touch_layout_t layout);
aether_touch_layout_t aether_touch_get_layout(const aether_touch_t *t);

/* Manual button API (also used by presets). */
i32  aether_touch_add_button(aether_touch_t *t, aether_input_action_t action,
                             f32 x, f32 y, f32 size, f32 opacity);
bool aether_touch_set_button  (aether_touch_t *t, u32 index,
                               f32 x, f32 y, f32 size, f32 opacity);

/* Read-only access for the renderer / SwiftUI overlay. */
const aether_touch_state_t *aether_touch_get_state(const aether_touch_t *t);

/* Serialize current custom layout to a simple text blob (for saving in
 * user settings later). Returns bytes written excluding NUL. */
size_t aether_touch_serialize(const aether_touch_t *t, char *out, size_t cap);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_TOUCH_H */
