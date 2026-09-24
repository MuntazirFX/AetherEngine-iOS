/* AetherNetServer.c — Server implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetServer.h"
#include "AetherNetScoreboard.h"
#include "AetherNetCmd.h"
#include "AetherNetSnapshot.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AETHER_SERVER_TIMEOUT      30.0f

aether_net_server_t *aether_net_server_create(u16 port, i32 max_clients) {
    aether_net_server_t *s = (aether_net_server_t*)calloc(1, sizeof *s);
    if (!s) return NULL;

    s->sock = aether_socket_create_udp_bound(port);
    if (!s->sock) { free(s); return NULL; }
    aether_socket_set_nonblocking(s->sock, true);

    s->port = port;
    s->listening = true;
    s->next_player_id = 1;
    s->max_clients = max_clients > 0 && max_clients <= AETHER_NET_MAX_PLAYERS
                     ? max_clients : AETHER_NET_MAX_PLAYERS;

    aether_str_copy(s->server_name, AETHER_NET_MAX_SERVER_NAME, "AetherEngine Server");
    aether_str_copy(s->map_name, AETHER_NET_MAX_MAP_NAME, "unknown");
    s->frag_limit = 30;
    s->time_limit_minutes = 30;
    s->sim_tick = 0;
    s->snap_accum = 0.f;
    s->snap_interval = 1.f / (f32)AETHER_NET_SNAPSHOT_RATE;
    s->has_snap = false;

    aether_log(AETHER_LOG_INFO, "net-server",
               "server listening on port %u (max=%d)", port, s->max_clients);
    return s;
}

void aether_net_server_destroy(aether_net_server_t *s) {
    if (!s) return;
    if (s->sock) aether_socket_destroy(s->sock);
    free(s);
}

void aether_net_server_set_info(aether_net_server_t *s,
                                 const char *name, const char *map,
                                 i32 frag_limit, i32 time_limit) {
    if (!s) return;
    if (name) aether_str_copy(s->server_name, AETHER_NET_MAX_SERVER_NAME, name);
    if (map)  aether_str_copy(s->map_name,    AETHER_NET_MAX_MAP_NAME,    map);
    s->frag_limit = frag_limit;
    s->time_limit_minutes = time_limit;
}

static aether_net_client_slot_t *find_slot_by_addr(aether_net_server_t *s,
                                                    const aether_net_addr_t *addr) {
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (s->clients[i].active &&
            aether_net_addr_equal(&s->clients[i].addr, addr)) return &s->clients[i];
    }
    return NULL;
}

static aether_net_client_slot_t *allocate_slot(aether_net_server_t *s,
                                                const aether_net_addr_t *addr) {
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) {
            memset(&s->clients[i], 0, sizeof s->clients[i]);
            s->clients[i].active = true;
            s->clients[i].player_id = s->next_player_id++;
            s->clients[i].addr = *addr;
            s->clients[i].last_recv_time = (f32)aether_net_time();
            s->clients[i].connect_time = s->clients[i].last_recv_time;
            s->clients[i].health = 100;
            s->clients[i].position = (aether_vec3_t){0, 0, 40};
            aether_net_cmd_history_init(&s->clients[i].cmd_hist);
            s->client_count++;
            return &s->clients[i];
        }
    }
    return NULL;
}

static void free_slot(aether_net_server_t *s, aether_net_client_slot_t *slot) {
    if (!slot || !slot->active) return;
    aether_log(AETHER_LOG_INFO, "net-server",
               "client #%u (%s) disconnected", slot->player_id, slot->name);
    slot->active = false;
    if (s->client_count > 0) s->client_count--;
}

static void send_reject(aether_net_server_t *s,
                        const aether_net_addr_t *to, u8 reason) {
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CONNECT_REJECT);
    aether_netbuf_write_u8 (&b, reason);
    aether_socket_send(s->sock, to, b.data, aether_netbuf_size(&b));
}

static void send_accept(aether_net_server_t *s,
                        const aether_net_addr_t *to, u32 player_id) {
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CONNECT_ACCEPT);
    aether_netbuf_write_u32(&b, player_id);
    aether_socket_send(s->sock, to, b.data, aether_netbuf_size(&b));
}

static void send_challenge(aether_net_server_t *s,
                           const aether_net_addr_t *to, u32 challenge) {
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CONNECT_CHALLENGE);
    aether_netbuf_write_u32(&b, challenge);
    aether_socket_send(s->sock, to, b.data, aether_netbuf_size(&b));
}

/* fwd */ aether_result_t aether_net_server_apply_cmd(aether_net_server_t *s, u32 player_id,
                                            const aether_net_cmd_t *cmd);

static void handle_packet(aether_net_server_t *s,
                          const aether_net_addr_t *from,
                          const u8 *data, u32 size) {
    if (size < 7) return;

    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);

    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver   = aether_netbuf_read_u16(&b);
    u8  msg   = aether_netbuf_read_u8(&b);

    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER) return;

    aether_net_client_slot_t *slot = find_slot_by_addr(s, from);

    switch ((aether_net_msg_t)msg) {
        case AETHER_MSG_CONNECT_REQUEST: {
            if (slot) {
                /* Already connected — resend accept */
                send_accept(s, from, slot->player_id);
                break;
            }
            if (s->client_count >= (u32)s->max_clients) {
                send_reject(s, from, AETHER_REJECT_SERVER_FULL);
                break;
            }
            /* Issue challenge */
            u32 challenge = (u32)aether_net_time() ^ 0xDEADBEEFu;
            send_challenge(s, from, challenge);
            break;
        }
        case AETHER_MSG_CONNECT_RESPONSE: {
            /* Client answers challenge — allocate slot */
            if (slot) { send_accept(s, from, slot->player_id); break; }
            if (s->client_count >= (u32)s->max_clients) {
                send_reject(s, from, AETHER_REJECT_SERVER_FULL);
                break;
            }
            slot = allocate_slot(s, from);
            if (!slot) { send_reject(s, from, AETHER_REJECT_SERVER_FULL); break; }

            char name[AETHER_NET_MAX_NAME] = {0};
            aether_netbuf_read_string(&b, name, AETHER_NET_MAX_NAME);
            aether_str_copy(slot->name, AETHER_NET_MAX_NAME, name);

            send_accept(s, from, slot->player_id);
            aether_log(AETHER_LOG_INFO, "net-server",
                       "client joined: '%s' id=%u", slot->name, slot->player_id);
            break;
        }
        case AETHER_MSG_DISCONNECT:
            if (slot) free_slot(s, slot);
            break;

        case AETHER_MSG_CLIENT_CMD:
            if (slot) {
                slot->last_recv_time = (f32)aether_net_time();
                aether_net_cmd_t cmd;
                /* Payload starts after 7-byte header already consumed into b;
                 * rebuild from original data+7 for robust decode. */
                if (aether_net_cmd_decode_payload(data + 7, size >= 7 ? size - 7 : 0, &cmd) == AETHER_OK) {
                    aether_net_server_apply_cmd(s, slot->player_id, &cmd);
                }
            }
            break;

        case AETHER_MSG_CHAT: {
            if (slot) {
                char text[AETHER_NET_MAX_CHAT] = {0};
                aether_netbuf_read_string(&b, text, AETHER_NET_MAX_CHAT);
                aether_net_server_broadcast_chat(s, slot->player_id, text);
            }
            break;
        }
        case AETHER_MSG_PING: {
            f32 t = aether_netbuf_read_f32(&b);
            aether_netbuf_t out;
            aether_netbuf_init_write(&out);
            aether_netbuf_write_u32(&out, AETHER_NET_PROTOCOL_ID);
            aether_netbuf_write_u16(&out, AETHER_NET_PROTOCOL_VER);
            aether_netbuf_write_u8 (&out, AETHER_MSG_PONG);
            aether_netbuf_write_f32(&out, t);
            aether_socket_send(s->sock, from, out.data, aether_netbuf_size(&out));
            break;
        }
        default: break;
    }
}

void aether_net_server_tick(aether_net_server_t *s, f32 dt) {
    if (!s) return;
    s->time += dt;

    /* Receive */
    u8 buf[AETHER_NET_MAX_PACKET];
    for (int i = 0; i < 32; ++i) {
        aether_net_addr_t from;
        i32 n = aether_socket_recv(s->sock, &from, buf, sizeof buf);
        if (n <= 0) break;
        handle_packet(s, &from, buf, (u32)n);
    }

    /* Timeout inactive clients */
    f32 now = (f32)aether_net_time();
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        aether_net_client_slot_t *c = &s->clients[i];
        if (!c->active) continue;
        if ((now - c->last_recv_time) > AETHER_SERVER_TIMEOUT) {
            free_slot(s, c);
        }
    }
}

void aether_net_server_broadcast_snapshot(aether_net_server_t *s,
                                           const u8 *data, u32 size) {
    if (!s || !data || size == 0) return;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_socket_send(s->sock, &s->clients[i].addr, data, size);
    }
}

void aether_net_server_broadcast_chat(aether_net_server_t *s,
                                       u32 from_player, const char *text) {
    if (!s || !text) return;

    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CHAT);
    aether_netbuf_write_u32(&b, from_player);
    aether_netbuf_write_u8 (&b, 0); /* AETHER_CHAT_CUE_TEXT */
    aether_netbuf_write_string(&b, text, AETHER_NET_MAX_CHAT);

    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_socket_send(s->sock, &s->clients[i].addr, b.data, aether_netbuf_size(&b));
    }
    aether_log(AETHER_LOG_INFO, "net-server", "chat from #%u: %s", from_player, text);
}

void aether_net_server_broadcast_scoreboard(aether_net_server_t *s) {
    if (!s) return;

    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_SCOREBOARD);
    aether_netbuf_write_u8 (&b, (u8)s->client_count);

    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_netbuf_write_u32(&b, s->clients[i].player_id);
        aether_netbuf_write_string(&b, s->clients[i].name, AETHER_NET_MAX_NAME);
        aether_netbuf_write_i32(&b, s->clients[i].score);
        aether_netbuf_write_i32(&b, s->clients[i].deaths);
        aether_netbuf_write_i32(&b, s->clients[i].ping_ms);
    }

    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_socket_send(s->sock, &s->clients[i].addr, b.data, aether_netbuf_size(&b));
    }
}

void aether_net_server_kick(aether_net_server_t *s, u32 player_id, u8 reason) {
    if (!s) return;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        aether_net_client_slot_t *c = &s->clients[i];
        if (c->active && c->player_id == player_id) {
            send_reject(s, &c->addr, reason);
            free_slot(s, c);
            return;
        }
    }
}

u32 aether_net_server_client_count(const aether_net_server_t *s) { return s ? s->client_count : 0; }

const aether_net_client_slot_t *aether_net_server_client_at(const aether_net_server_t *s, u32 idx) {
    if (!s || idx >= AETHER_NET_MAX_PLAYERS) return NULL;
    if (!s->clients[idx].active) return NULL;
    return &s->clients[idx];
}

const aether_net_client_slot_t *aether_net_server_client_by_id(const aether_net_server_t *s, u32 id) {
    if (!s) return NULL;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (s->clients[i].active && s->clients[i].player_id == id)
            return &s->clients[i];
    }
    return NULL;
}

void aether_net_server_dump(const aether_net_server_t *s) {
    if (!s) return;
    aether_log(AETHER_LOG_INFO, "net-server",
               "===== SERVER '%s' map='%s' clients=%u/%d =====",
               s->server_name, s->map_name, s->client_count, s->max_clients);
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_log(AETHER_LOG_INFO, "net-server",
                   "  #%u %-20s score=%d deaths=%d ping=%d",
                   s->clients[i].player_id, s->clients[i].name,
                   s->clients[i].score, s->clients[i].deaths, s->clients[i].ping_ms);
    }
}

aether_result_t aether_net_server_apply_cmd(aether_net_server_t *s, u32 player_id,
                                            const aether_net_cmd_t *cmd) {
    if (!s || !cmd) return AETHER_ERR_INVALID_ARG;
    aether_net_client_slot_t *slot = NULL;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (s->clients[i].active && s->clients[i].player_id == player_id) {
            slot = &s->clients[i]; break;
        }
    }
    if (!slot) return AETHER_ERR_NOT_FOUND;
    if (cmd->seq && cmd->seq <= slot->last_seq) return AETHER_OK; /* stale */
    slot->last_seq = cmd->seq;
    slot->buttons = cmd->buttons;
    slot->angles.y = cmd->yaw_deg;
    slot->angles.x = cmd->pitch_deg;
    aether_net_cmd_apply_move(&slot->position, &slot->angles.y, cmd, 320.f);
    aether_net_cmd_history_push(&slot->cmd_hist, cmd, s->time);
    return AETHER_OK;
}

u32 aether_net_server_build_snapshot(aether_net_server_t *s, aether_net_snapshot_t *out) {
    if (!s) return 0;
    aether_net_snapshot_t local;
    aether_net_snapshot_t *dst = out ? out : &local;
    memset(dst, 0, sizeof(*dst));
    dst->tick = s->sim_tick;
    dst->time = s->time;
    u32 n = 0;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS && n < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_net_snapshot_player_t *p = &dst->players[n++];
        p->player_id = s->clients[i].player_id;
        aether_str_copy(p->name, sizeof p->name, s->clients[i].name);
        p->score = s->clients[i].score;
        p->deaths = s->clients[i].deaths;
        p->ping_ms = s->clients[i].ping_ms;
        p->origin[0] = s->clients[i].position.x;
        p->origin[1] = s->clients[i].position.y;
        p->origin[2] = s->clients[i].position.z;
    }
    dst->player_count = n;
    s->last_snap = *dst;
    s->has_snap = true;
    return n;
}

u32 aether_net_server_tick_authority(aether_net_server_t *s, f32 dt) {
    if (!s) return 0;
    aether_net_server_tick(s, dt);
    s->sim_tick++;
    s->snap_accum += dt > 0.f ? dt : 0.f;
    f32 interval = s->snap_interval > 1e-4f ? s->snap_interval : 0.05f;
    if (s->snap_accum < interval) return 0;
    s->snap_accum = 0.f;
    aether_net_snapshot_t snap;
    aether_net_server_build_snapshot(s, &snap);
    u8 pkt[1400];
    u32 psz = aether_net_snapshot_encode(&snap, pkt, sizeof pkt);
    if (psz > 0) aether_net_server_broadcast_snapshot(s, pkt, psz);
    return 1;
}

const aether_net_cmd_t *aether_net_server_lagcomp_cmd(const aether_net_server_t *s,
                                                     u32 player_id, f32 lag_ms) {
    if (!s) return NULL;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active || s->clients[i].player_id != player_id) continue;
        return aether_net_cmd_history_at_lag(&s->clients[i].cmd_hist, s->time, lag_ms);
    }
    return NULL;
}

void aether_net_server_broadcast_join(aether_net_server_t *s, u32 player_id, const char *name) {
    if (!s) return;
    u8 pkt[256];
    u32 n = aether_scoreboard_encode_join(pkt, sizeof pkt, player_id, name);
    if (!n) return;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_socket_send(s->sock, &s->clients[i].addr, pkt, n);
    }
}

void aether_net_server_broadcast_leave(aether_net_server_t *s, u32 player_id) {
    if (!s) return;
    u8 pkt[64];
    u32 n = aether_scoreboard_encode_leave(pkt, sizeof pkt, player_id);
    if (!n) return;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_socket_send(s->sock, &s->clients[i].addr, pkt, n);
    }
}

bool aether_net_server_set_score(aether_net_server_t *s, u32 player_id,
                                 i32 score, i32 deaths) {
    if (!s) return false;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active || s->clients[i].player_id != player_id) continue;
        s->clients[i].score = score;
        s->clients[i].deaths = deaths;
        return true;
    }
    return false;
}

void aether_net_server_broadcast_kill(aether_net_server_t *s,
                                      u32 killer_id, const char *killer_name,
                                      u32 victim_id, const char *victim_name) {
    if (!s) return;
    u8 pkt[256];
    u32 n = aether_scoreboard_encode_kill(pkt, sizeof pkt, killer_id, killer_name,
                                          victim_id, victim_name);
    if (!n) return;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        aether_socket_send(s->sock, &s->clients[i].addr, pkt, n);
    }
}

bool aether_net_server_register_kill(aether_net_server_t *s,
                                     u32 killer_id, u32 victim_id) {
    if (!s) return false;
    const char *kn = "";
    const char *vn = "";
    bool ok = false;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (!s->clients[i].active) continue;
        if (s->clients[i].player_id == killer_id) {
            s->clients[i].score += 1;
            kn = s->clients[i].name;
            ok = true;
        }
        if (s->clients[i].player_id == victim_id) {
            s->clients[i].deaths += 1;
            vn = s->clients[i].name;
            ok = true;
        }
    }
    if (ok) aether_net_server_broadcast_kill(s, killer_id, kn, victim_id, vn);
    return ok;
}

u32 aether_net_server_fanout_scores(aether_net_server_t *s) {
    if (!s || !s->sock) return 0;
    aether_net_server_broadcast_scoreboard(s);
    u32 n = 0;
    for (u32 i = 0; i < AETHER_NET_MAX_PLAYERS; ++i)
        if (s->clients[i].active) n++;
    return n;
}

u32 aether_net_server_tick_authority_kill_score(aether_net_server_t *s, f32 dt,
                                                u32 killer_id, u32 victim_id,
                                                aether_net_server_authority_fanout_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!s) return 0;
    u32 kills = 0;
    if (killer_id || victim_id) {
        if (aether_net_server_register_kill(s, killer_id, victim_id))
            kills = 1;
    }
    u32 snaps = aether_net_server_tick_authority(s, dt);
    /* Always fanout scoreboard when a kill happened so HUD stays consistent */
    u32 reached = 0;
    u32 scoreboards = 0;
    if (kills > 0 || snaps > 0) {
        reached = aether_net_server_fanout_scores(s);
        scoreboards = reached > 0 ? 1u : 0u;
    }
    if (out) {
        out->snapshots = snaps;
        out->scoreboards = scoreboards;
        out->kills = kills;
        out->clients_reached = reached;
    }
    return snaps + kills + scoreboards;
}
