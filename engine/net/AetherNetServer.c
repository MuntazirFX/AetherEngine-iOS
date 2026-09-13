/* AetherNetServer.c — Server implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetServer.h"
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
                u32 seq = aether_netbuf_read_u32(&b);
                slot->last_seq = seq;
                slot->position = aether_netbuf_read_vec3(&b);
                (void)slot->position;  /* not used yet */
                slot->angles.x = aether_netbuf_read_f32(&b);
                slot->angles.y = aether_netbuf_read_f32(&b);
                slot->angles.z = aether_netbuf_read_f32(&b);
                /* buttons */
                (void)aether_netbuf_read_u32(&b);
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
