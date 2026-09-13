/* AetherSaveWorld.c — World state serialization implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherSaveWorld.h"
#include <string.h>

void aether_save_world_pack(aether_save_world_t *out,
                             f32 game_time, u32 level_flags,
                             u32 secrets, u32 kills, f32 difficulty) {
    if (!out) return;
    memset(out, 0, sizeof *out);
    out->version          = AETHER_SAVE_VERSION;
    out->game_time        = game_time;
    out->level_flags      = level_flags;
    out->secrets_found    = secrets;
    out->kills            = kills;
    out->difficulty_scale = difficulty;
}

void aether_save_world_unpack(const aether_save_world_t *in,
                               f32 *out_game_time, u32 *out_level_flags,
                               u32 *out_secrets, u32 *out_kills,
                               f32 *out_difficulty) {
    if (!in) return;
    if (out_game_time)  *out_game_time  = in->game_time;
    if (out_level_flags)*out_level_flags= in->level_flags;
    if (out_secrets)    *out_secrets    = in->secrets_found;
    if (out_kills)      *out_kills      = in->kills;
    if (out_difficulty) *out_difficulty = in->difficulty_scale;
}

void aether_save_world_dump(const aether_save_world_t *w) {
    if (!w) return;
    aether_log(AETHER_LOG_INFO, "save-world",
               "time=%.1fs flags=0x%X secrets=%u kills=%u diff=%.2f",
               w->game_time, w->level_flags, w->secrets_found,
               w->kills, w->difficulty_scale);
}
