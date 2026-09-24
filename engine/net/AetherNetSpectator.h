/* AetherNetSpectator.h — Simple spectator follow camera stub.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_SPECTATOR_H
#define AETHER_NET_SPECTATOR_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_spectator {
    bool enabled;
    u32  target_player_id;
    f32  eye[3];
    f32  forward[3];
    f32  follow_distance; /* behind target */
    f32  follow_height;
    f32  smooth;          /* 0..1 lerp toward target */
    bool following;
} aether_spectator_t;

void aether_spectator_init(aether_spectator_t *sp);
void aether_spectator_set_enabled(aether_spectator_t *sp, bool enabled);
void aether_spectator_follow(aether_spectator_t *sp, u32 player_id);
void aether_spectator_stop(aether_spectator_t *sp);

/* Update eye/forward to follow target_pos + target_fwd. Returns 1 if active. */
int  aether_spectator_tick(aether_spectator_t *sp, f32 dt,
                           const f32 target_pos[3], const f32 target_fwd[3]);

/* Copy current spectator eye/forward. */
void aether_spectator_get_eye(const aether_spectator_t *sp, f32 out[3]);
void aether_spectator_get_forward(const aether_spectator_t *sp, f32 out[3]);
bool aether_spectator_is_following(const aether_spectator_t *sp);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_SPECTATOR_H */
