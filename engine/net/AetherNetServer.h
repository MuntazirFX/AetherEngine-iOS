/* AetherNetServer.h — Server-side connection manager (listen + clients).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_SERVER_H
#define AETHER_NET_SERVER_H

#include "AetherNet.h"
#include "AetherNetBuffer.h"
#include "AetherNetSnapshot.h"
#include "AetherNetCmd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_net_client_slot {
    bool                active;
    u32                 player_id;
    aether_net_addr_t   addr;
    char                name[AETHER_NET_MAX_NAME];
    f32                 last_recv_time;
    f32                 connect_time;
    u32                 challenge;
    u32                 last_seq;
    /* Gameplay */
    aether_vec3_t       position;
    aether_vec3_t       angles;
    aether_vec3_t       velocity;
    i32                 health;
    i32                 armor;
    i32                 score;
    i32                 deaths;
    i32                 assists; /* kill assist stub */
    i32                 ping_ms;
    aether_net_cmd_history_t cmd_hist;
    u32                 buttons;
} aether_net_client_slot_t;

typedef struct aether_net_server {
    aether_socket_t         *sock;
    u16                      port;
    bool                     listening;
    aether_net_client_slot_t clients[AETHER_NET_MAX_PLAYERS];
    u32                      client_count;
    u32                      next_player_id;
    f32                      time;

    /* Server info */
    char                     server_name[AETHER_NET_MAX_SERVER_NAME];
    char                     map_name[AETHER_NET_MAX_MAP_NAME];
    i32                      max_clients;
    /* Game rules */
    i32                      frag_limit;
    i32                      time_limit_minutes;
    /* Authority / snapshot broadcast */
    u32                      sim_tick;
    f32                      snap_accum;
    f32                      snap_interval; /* default 1/20 */
    aether_net_snapshot_t    last_snap;
    bool                     has_snap;
} aether_net_server_t;

aether_net_server_t *aether_net_server_create(u16 port, i32 max_clients);
void                 aether_net_server_destroy(aether_net_server_t *s);

void aether_net_server_set_info(aether_net_server_t *s,
                                 const char *name, const char *map,
                                 i32 frag_limit, i32 time_limit);

/* Per-frame pump: receive, handle handshake, timeouts */
void aether_net_server_tick(aether_net_server_t *s, f32 dt);

/* Broadcast a snapshot to all active clients */
void aether_net_server_broadcast_snapshot(aether_net_server_t *s,
                                           const u8 *snapshot_data, u32 size);

/* Broadcast a chat message from a specific client */
void aether_net_server_broadcast_chat(aether_net_server_t *s,
                                       u32 from_player, const char *text);

/* Scoreboard broadcast */
void aether_net_server_broadcast_scoreboard(aether_net_server_t *s);

/* Kick a client */
void aether_net_server_kick(aether_net_server_t *s, u32 player_id, u8 reason);

/* Access */
u32 aether_net_server_client_count(const aether_net_server_t *s);
const aether_net_client_slot_t *aether_net_server_client_at(const aether_net_server_t *s, u32 idx);
const aether_net_client_slot_t *aether_net_server_client_by_id(const aether_net_server_t *s, u32 id);

void aether_net_server_dump(const aether_net_server_t *s);

/* Apply authoritative client cmd to slot (move/look/buttons + history). */
aether_result_t aether_net_server_apply_cmd(aether_net_server_t *s, u32 player_id,
                                            const aether_net_cmd_t *cmd);

/* Build snapshot from current client slots into out (or s->last_snap). */
u32 aether_net_server_build_snapshot(aether_net_server_t *s, aether_net_snapshot_t *out);

/* Tick authority: receive → apply cmds → optional snapshot broadcast when due.
 * Returns snapshots broadcast this call (0 or 1 typically). */
u32 aether_net_server_tick_authority(aether_net_server_t *s, f32 dt);

/* Lag-comp stub: fetch historical cmd for player at lag_ms. */
const aether_net_cmd_t *aether_net_server_lagcomp_cmd(const aether_net_server_t *s,
                                                     u32 player_id, f32 lag_ms);

/* Broadcast PLAYER_JOIN / PLAYER_LEAVE to all active clients. */
void aether_net_server_broadcast_join(aether_net_server_t *s, u32 player_id, const char *name);
void aether_net_server_broadcast_leave(aether_net_server_t *s, u32 player_id);

/* Set frags/deaths on a slot (creates inactive→active score fields). */
bool aether_net_server_set_score(aether_net_server_t *s, u32 player_id,
                                 i32 score, i32 deaths);
/* Killer +1 frag, victim +1 death; broadcasts kill feed packet. */
bool aether_net_server_register_kill(aether_net_server_t *s,
                                     u32 killer_id, u32 victim_id);
void aether_net_server_broadcast_kill(aether_net_server_t *s,
                                      u32 killer_id, const char *killer_name,
                                      u32 victim_id, const char *victim_name);

#ifdef __cplusplus
}
#endif

/* ---------- Live MP authority: kill/score fanout to clients ---------- */
typedef struct aether_net_server_authority_fanout {
    u32 snapshots;       /* snapshot broadcasts this tick */
    u32 scoreboards;     /* scoreboard packet broadcasts */
    u32 kills;           /* kill feed packets sent */
    u32 clients_reached; /* active clients that received fanout */
} aether_net_server_authority_fanout_t;

/* Tick authority + fan out scoreboard after any pending kill/score change.
 * If killer_id/victim_id non-zero, register_kill first then fanout. */
u32 aether_net_server_tick_authority_kill_score(aether_net_server_t *s, f32 dt,
                                                u32 killer_id, u32 victim_id,
                                                aether_net_server_authority_fanout_t *out);

/* Fan out current scores (scoreboard packet) to all clients. Returns clients reached. */
u32 aether_net_server_fanout_scores(aether_net_server_t *s);

/* Optional kill-assist stub: +1 assist on assister (no frag). Returns true if slot found. */
bool aether_net_server_register_assist(aether_net_server_t *s, u32 assister_id, u32 victim_id);
i32  aether_net_server_get_assists(const aether_net_server_t *s, u32 player_id);

#endif /* AETHER_NET_SERVER_H */
