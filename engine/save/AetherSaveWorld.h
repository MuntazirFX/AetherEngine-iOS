/* AetherSaveWorld.h — World state serialization.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_SAVE_WORLD_H
#define AETHER_SAVE_WORLD_H

#include "AetherSaveFormat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_save_world {
    u32  version;
    f32  game_time;       /* seconds since level start */
    u32  level_flags;     /* bit flags for doors broken, triggers fired */
    u32  secrets_found;
    u32  kills;
    f32  difficulty_scale;
    u8   reserved[32];
} aether_save_world_t;

void aether_save_world_pack(aether_save_world_t *out,
                             f32 game_time,
                             u32 level_flags,
                             u32 secrets,
                             u32 kills,
                             f32 difficulty);

void aether_save_world_unpack(const aether_save_world_t *in,
                               f32 *out_game_time,
                               u32 *out_level_flags,
                               u32 *out_secrets,
                               u32 *out_kills,
                               f32 *out_difficulty);

void aether_save_world_dump(const aether_save_world_t *w);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_SAVE_WORLD_H */
