/* AetherNetClient.c — Client implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetClient.h"
#include "AetherNetSnapshot.h"
#include <stdlib.h>
#include <string.h>

#define AETHER_CLIENT_TIMEOUT     15.0f
#define AETHER_CLIENT_PING_PERIOD  3.0f

struct aether_net_client {
    aether_socket_t    *sock;
    aether_net_addr_t   server;
    aether_net_state_t  state;
    u32                 player_id;
    u32                 challenge;
    f32                 last_recv_time;
    f32                 last_ping_time;
    f32                 last_send_time;
    u32                 outgoing_seq;
    aether_net_stats_t  stats;
    char                player_name[AETHER_NET_MAX_NAME];
    aether_net_snapshot_t last_snap;
    bool                has_snap;
    u32                 snap_count;
};

aether_net_client_t *aether_net_client_create(void) {
    aether_net_client_t *c = (aether_net_client_t*)calloc(1, sizeof *c);
    if (!c) return NULL;
    c->sock = aether_socket_create_udp();
    if (!c->sock) { free(c); return NULL; }
    aether_socket_set_nonblocking(c->sock, true);
    c->state = AETHER_NET_STATE_DISCONNECTED;
    c->player_id = 0xFFFFFFFFu;
    aether_str_copy(c->player_name, AETHER_NET_MAX_NAME, "player");
    aether_log(AETHER_LOG_INFO, "net-client", "client created");
    return c;
}

void aether_net_client_destroy(aether_net_client_t *c) {
    if (!c) return;
    if (c->sock) aether_socket_destroy(c->sock);
    free(c);
}

aether_result_t aether_net_client_connect(aether_net_client_t *c,
                                           const char *host, u16 port) {
    if (!c || !host) return AETHER_ERR_INVALID_ARG;

    if (!aether_net_resolve(host, port, &c->server)) {
        aether_log(AETHER_LOG_ERROR, "net-client", "cannot resolve %s", host);
        return AETHER_ERR_NOT_FOUND;
    }

    /* Send connect request */
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CONNECT_REQUEST);
    aether_netbuf_write_string(&b, c->player_name, AETHER_NET_MAX_NAME);

    aether_socket_send(c->sock, &c->server, b.data, aether_netbuf_size(&b));
    c->stats.packets_sent++;
    c->stats.bytes_sent += aether_netbuf_size(&b);

    c->state = AETHER_NET_STATE_CONNECTING;
    c->last_recv_time = (f32)aether_net_time();

    aether_log(AETHER_LOG_INFO, "net-client",
               "connect request sent to %s:%u", host, port);
    return AETHER_OK;
}

void aether_net_client_disconnect(aether_net_client_t *c) {
    if (!c) return;
    if (c->state == AETHER_NET_STATE_DISCONNECTED) return;

    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_DISCONNECT);

    aether_socket_send(c->sock, &c->server, b.data, aether_netbuf_size(&b));
    c->state = AETHER_NET_STATE_DISCONNECTED;
    aether_log(AETHER_LOG_INFO, "net-client", "disconnected");
}

/* Handle an incoming packet */
static void handle_packet(aether_net_client_t *c, const u8 *data, u32 size) {
    if (size < 7) return;

    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);

    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver   = aether_netbuf_read_u16(&b);
    u8  msg   = aether_netbuf_read_u8(&b);

    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER) return;

    c->last_recv_time = (f32)aether_net_time();

    switch ((aether_net_msg_t)msg) {
        case AETHER_MSG_CONNECT_CHALLENGE: {
            c->challenge = aether_netbuf_read_u32(&b);
            aether_log(AETHER_LOG_INFO, "net-client", "challenge=%u", c->challenge);

            /* Respond with challenge */
            aether_netbuf_t out;
            aether_netbuf_init_write(&out);
            aether_netbuf_write_u32(&out, AETHER_NET_PROTOCOL_ID);
            aether_netbuf_write_u16(&out, AETHER_NET_PROTOCOL_VER);
            aether_netbuf_write_u8 (&out, AETHER_MSG_CONNECT_RESPONSE);
            aether_netbuf_write_u32(&out, c->challenge);
            aether_netbuf_write_string(&out, c->player_name, AETHER_NET_MAX_NAME);
            aether_socket_send(c->sock, &c->server, out.data, aether_netbuf_size(&out));
            c->state = AETHER_NET_STATE_CHALLENGING;
            break;
        }
        case AETHER_MSG_CONNECT_ACCEPT: {
            c->player_id = aether_netbuf_read_u32(&b);
            c->state = AETHER_NET_STATE_ACTIVE;
            aether_log(AETHER_LOG_INFO, "net-client", "accepted, player_id=%u", c->player_id);
            break;
        }
        case AETHER_MSG_CONNECT_REJECT: {
            u8 reason = aether_netbuf_read_u8(&b);
            aether_log(AETHER_LOG_WARN, "net-client", "rejected: reason=%u", reason);
            c->state = AETHER_NET_STATE_DISCONNECTED;
            break;
        }
        case AETHER_MSG_SERVER_SNAPSHOT: {
            /* Full packet includes 7-byte header; decode from start. */
            aether_net_snapshot_t snap;
            if (aether_net_snapshot_decode(data, size, &snap) == AETHER_OK) {
                c->last_snap = snap;
                c->has_snap = true;
                c->snap_count++;
            }
            break;
        }
        case AETHER_MSG_PING:
            /* Respond with pong */
            break;
        case AETHER_MSG_PONG: {
            f32 sent_time = aether_netbuf_read_f32(&b);
            f32 now = (f32)aether_net_time();
            c->stats.ping_ms = (now - sent_time) * 1000.0f;
            break;
        }
        default: break;
    }
}

void aether_net_client_tick(aether_net_client_t *c, f32 dt) {
    if (!c) return;

    /* Receive */
    u8 buf[AETHER_NET_MAX_PACKET];
    for (int i = 0; i < 16; ++i) {
        aether_net_addr_t from;
        i32 n = aether_socket_recv(c->sock, &from, buf, sizeof buf);
        if (n <= 0) break;
        if (!aether_net_addr_equal(&from, &c->server)) continue;
        c->stats.packets_received++;
        c->stats.bytes_received += (u32)n;
        handle_packet(c, buf, (u32)n);
    }

    /* Timeout check */
    f32 now = (f32)aether_net_time();
    if (c->state != AETHER_NET_STATE_DISCONNECTED) {
        if ((now - c->last_recv_time) > AETHER_CLIENT_TIMEOUT) {
            aether_log(AETHER_LOG_WARN, "net-client", "server timeout");
            c->state = AETHER_NET_STATE_DISCONNECTED;
        }
    }

    /* Periodic ping */
    if (c->state == AETHER_NET_STATE_ACTIVE &&
        (now - c->last_ping_time) > AETHER_CLIENT_PING_PERIOD) {
        aether_netbuf_t b;
        aether_netbuf_init_write(&b);
        aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
        aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
        aether_netbuf_write_u8 (&b, AETHER_MSG_PING);
        aether_netbuf_write_f32(&b, now);
        aether_socket_send(c->sock, &c->server, b.data, aether_netbuf_size(&b));
        c->last_ping_time = now;
        c->stats.packets_sent++;
    }

    (void)dt;
}

aether_result_t aether_net_client_send_cmd(aether_net_client_t *c,
                                            aether_vec3_t move, f32 yaw, f32 pitch,
                                            u32 buttons) {
    if (!c || c->state != AETHER_NET_STATE_ACTIVE) return AETHER_ERR_NOT_READY;

    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CLIENT_CMD);
    aether_netbuf_write_u32(&b, c->outgoing_seq++);
    aether_netbuf_write_vec3(&b, move);
    aether_netbuf_write_f32(&b, yaw);
    aether_netbuf_write_f32(&b, pitch);
    aether_netbuf_write_u32(&b, buttons);

    i32 n = aether_socket_send(c->sock, &c->server, b.data, aether_netbuf_size(&b));
    if (n > 0) { c->stats.packets_sent++; c->stats.bytes_sent += (u32)n; }
    return n > 0 ? AETHER_OK : AETHER_ERR_IO;
}

aether_result_t aether_net_client_send_chat(aether_net_client_t *c, const char *text) {
    if (!c || !text || c->state != AETHER_NET_STATE_ACTIVE) return AETHER_ERR_NOT_READY;

    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CHAT);
    aether_netbuf_write_string(&b, text, AETHER_NET_MAX_CHAT);

    i32 n = aether_socket_send(c->sock, &c->server, b.data, aether_netbuf_size(&b));
    return n > 0 ? AETHER_OK : AETHER_ERR_IO;
}

aether_net_state_t aether_net_client_state(const aether_net_client_t *c) {
    return c ? c->state : AETHER_NET_STATE_DISCONNECTED;
}
u32 aether_net_client_player_id(const aether_net_client_t *c) { return c ? c->player_id : 0; }
const aether_net_stats_t *aether_net_client_stats(const aether_net_client_t *c) { return c ? &c->stats : NULL; }
const aether_net_addr_t *aether_net_client_server_addr(const aether_net_client_t *c) { return c ? &c->server : NULL; }

void aether_net_client_dump(const aether_net_client_t *c) {
    if (!c) return;
    char addr[64] = "?";
    aether_net_addr_to_string(&c->server, addr, sizeof addr);
    aether_log(AETHER_LOG_INFO, "net-client",
               "state=%d server=%s:%u player_id=%u sent=%u/%u recv=%u/%u ping=%.1fms",
               (int)c->state, addr, c->server.port, c->player_id,
               c->stats.packets_sent, c->stats.bytes_sent,
               c->stats.packets_received, c->stats.bytes_received,
               c->stats.ping_ms);
}

const aether_net_snapshot_t *aether_net_client_last_snapshot(const aether_net_client_t *c) {
    if (!c || !c->has_snap) return NULL;
    return &c->last_snap;
}
u32 aether_net_client_snapshot_count(const aether_net_client_t *c) {
    return c ? c->snap_count : 0;
}
u32 aether_net_client_apply_snapshot_hud(aether_net_client_t *c,
                                         aether_scoreboard_t *sb,
                                         aether_chat_log_t *chat,
                                         f32 now) {
    if (!c || !c->has_snap || !sb) return 0;
    aether_net_snapshot_apply_hud(&c->last_snap, sb, chat, now);
    return c->last_snap.player_count;
}
aether_result_t aether_net_client_ingest_snapshot_packet(aether_net_client_t *c,
                                                         const u8 *data, u32 size) {
    if (!c || !data || size < 8) return AETHER_ERR_INVALID_ARG;
    handle_packet(c, data, size);
    return c->has_snap ? AETHER_OK : AETHER_ERR_INVALID_ARG;
}
