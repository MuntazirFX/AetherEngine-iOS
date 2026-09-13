/* AetherNetScoreboard.h — Client-side scoreboard state.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_SCOREBOARD_H
#define AETHER_NET_SCOREBOARD_H

#include "AetherNetProtocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_scoreboard_entry {
    u32  player_id;
    char name[AETHER_NET_MAX_NAME];
    i32  score;
    i32  deaths;
    i32  ping_ms;
    bool active;
} aether_scoreboard_entry_t;

typedef struct aether_scoreboard {
    aether_scoreboard_entry_t entries[AETHER_NET_MAX_PLAYERS];
    u32 count;
    bool visible;
} aether_scoreboard_t;

void aether_scoreboard_init(aether_scoreboard_t *sb);
void aether_scoreboard_clear(aether_scoreboard_t *sb);
void aether_scoreboard_set_visible(aether_scoreboard_t *sb, bool visible);
void aether_scoreboard_update_from_packet(aether_scoreboard_t *sb,
                                           const u8 *data, u32 size);
void aether_scoreboard_dump(const aether_scoreboard_t *sb);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_SCOREBOARD_H */
