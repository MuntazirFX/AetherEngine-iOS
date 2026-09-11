/* AetherPlayer.h — First-person player with gravity + collision (STEP 14).
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
    i32           hull_index;    /* 1 or 2 */
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

#ifdef __cplusplus
}
#endif
#endif
