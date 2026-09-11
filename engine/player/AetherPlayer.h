/* AetherPlayer.h — First-person player state + movement (STEP 13).
 * Free-fly (no collision yet). AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_PLAYER_H
#define AETHER_PLAYER_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"
#include "../input/AetherInput.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_player {
    aether_vec3_t position;
    aether_vec3_t velocity;
    f32           yaw;         /* radians */
    f32           pitch;       /* radians, clamped ±89° */
    f32           eye_height;  /* crouch changes this */
    f32           move_speed;  /* units/sec */
    f32           look_speed;  /* rad/unit input */
    bool          on_ground;
    bool          crouching;
} aether_player_t;

void            aether_player_init(aether_player_t *p);
void            aether_player_set_position(aether_player_t *p, aether_vec3_t pos);
void            aether_player_update(aether_player_t *p,
                                     const aether_input_state_t *in,
                                     f32 dt);
aether_vec3_t   aether_player_eye_position(const aether_player_t *p);
aether_vec3_t   aether_player_forward   (const aether_player_t *p);
aether_vec3_t   aether_player_right     (const aether_player_t *p);

#ifdef __cplusplus
}
#endif
#endif
