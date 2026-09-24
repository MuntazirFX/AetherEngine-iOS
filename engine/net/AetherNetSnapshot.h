/* AetherNetSnapshot.h — Snapshot tick → scoreboard/chat HUD bridge fields.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_SNAPSHOT_H
#define AETHER_NET_SNAPSHOT_H

#include "AetherNetProtocol.h"
#include "AetherNetScoreboard.h"
#include "AetherNetChat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_net_snapshot_player {
    u32  player_id;
    char name[AETHER_NET_MAX_NAME];
    i32  score;
    i32  deaths;
    i32  ping_ms;
    f32  origin[3];
} aether_net_snapshot_player_t;

typedef struct aether_net_snapshot {
    u32 tick;
    f32 time;
    u32 player_count;
    aether_net_snapshot_player_t players[AETHER_NET_MAX_PLAYERS];
    char chat_line[AETHER_NET_MAX_CHAT]; /* optional one-liner this tick (empty = none) */
    u32  chat_from;                      /* player_id for chat_line */
} aether_net_snapshot_t;

/* Pack snapshot into wire buffer (AETHER_MSG_SERVER_SNAPSHOT). Returns bytes written. */
u32 aether_net_snapshot_encode(const aether_net_snapshot_t *snap, u8 *out, u32 cap);

/* Decode wire buffer into snapshot. */
aether_result_t aether_net_snapshot_decode(const u8 *data, u32 size,
                                           aether_net_snapshot_t *out);

/* Apply decoded snapshot → scoreboard entries + optional chat line. */
void aether_net_snapshot_apply_hud(const aether_net_snapshot_t *snap,
                                   aether_scoreboard_t *sb,
                                   aether_chat_log_t *chat,
                                   f32 now);

/* Convenience: build a demo snapshot for host/iOS smokes. */
void aether_net_snapshot_make_demo(aether_net_snapshot_t *snap, u32 tick, f32 time);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_SNAPSHOT_H */
