/* AetherInput.h — Abstract input layer (touch, controller, keyboard).
 * All input sources feed into a single normalized state.
 */
#ifndef AETHER_INPUT_H
#define AETHER_INPUT_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aether_input_action {
    AETHER_ACTION_NONE = 0,
    AETHER_ACTION_MOVE_FORWARD,
    AETHER_ACTION_MOVE_BACK,
    AETHER_ACTION_MOVE_LEFT,
    AETHER_ACTION_MOVE_RIGHT,
    AETHER_ACTION_FIRE,
    AETHER_ACTION_JUMP,
    AETHER_ACTION_DUCK,
    AETHER_ACTION_USE,
    AETHER_ACTION_RELOAD,
    AETHER_ACTION_WEAPON_NEXT,
    AETHER_ACTION_WEAPON_PREV,
    AETHER_ACTION_SCOREBOARD,
    AETHER_ACTION_PAUSE,
    AETHER_ACTION_COUNT
} aether_input_action_t;

typedef struct aether_input_state {
    f32  move_x;                            /* -1 .. 1 */
    f32  move_y;                            /* -1 .. 1 */
    f32  look_dx;                           /* accumulated delta */
    f32  look_dy;                           /* accumulated delta */
    bool actions[AETHER_ACTION_COUNT];      /* held/pressed */
} aether_input_state_t;

typedef struct aether_input aether_input_t;

aether_input_t *aether_input_create(void);
void            aether_input_destroy(aether_input_t *in);

/* Called once per engine frame by the engine. */
void aether_input_begin_frame(aether_input_t *in);   /* clears per-frame deltas */
void aether_input_end_frame  (aether_input_t *in);   /* preserves held states */

/* Event injection (called by iOS Swift layer). */
void aether_input_set_action(aether_input_t *in, aether_input_action_t action, bool pressed);
void aether_input_set_move  (aether_input_t *in, f32 x, f32 y);
void aether_input_add_look  (aether_input_t *in, f32 dx, f32 dy);

/* Query */
const aether_input_state_t *aether_input_state(const aether_input_t *in);
bool aether_input_held    (const aether_input_t *in, aether_input_action_t action);
bool aether_input_just_pressed(const aether_input_t *in, aether_input_action_t action);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_INPUT_H */
