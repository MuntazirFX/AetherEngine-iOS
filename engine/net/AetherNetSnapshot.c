/* AetherNetSnapshot.c — Snapshot encode/decode + HUD apply.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetSnapshot.h"
#include "AetherNetBuffer.h"
#include <string.h>

u32 aether_net_snapshot_encode(const aether_net_snapshot_t *snap, u8 *out, u32 cap) {
    if (!snap || !out || cap < 16) return 0;
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_SERVER_SNAPSHOT);
    aether_netbuf_write_u32(&b, snap->tick);
    aether_netbuf_write_f32(&b, snap->time);
    u8 n = (u8)(snap->player_count > AETHER_NET_MAX_PLAYERS
                    ? AETHER_NET_MAX_PLAYERS : snap->player_count);
    aether_netbuf_write_u8(&b, n);
    for (u8 i = 0; i < n; ++i) {
        const aether_net_snapshot_player_t *p = &snap->players[i];
        aether_netbuf_write_u32(&b, p->player_id);
        aether_netbuf_write_string(&b, p->name, AETHER_NET_MAX_NAME);
        aether_netbuf_write_i32(&b, p->score);
        aether_netbuf_write_i32(&b, p->deaths);
        aether_netbuf_write_i32(&b, p->ping_ms);
        aether_netbuf_write_f32(&b, p->origin[0]);
        aether_netbuf_write_f32(&b, p->origin[1]);
        aether_netbuf_write_f32(&b, p->origin[2]);
    }
    aether_netbuf_write_u32(&b, snap->chat_from);
    aether_netbuf_write_string(&b, snap->chat_line, AETHER_NET_MAX_CHAT);
    u32 sz = aether_netbuf_size(&b);
    if (sz > cap || b.overflow) return 0;
    memcpy(out, b.data, sz);
    return sz;
}

aether_result_t aether_net_snapshot_decode(const u8 *data, u32 size,
                                           aether_net_snapshot_t *out) {
    if (!data || !out || size < 8) return AETHER_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);
    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver   = aether_netbuf_read_u16(&b);
    u8  msg   = aether_netbuf_read_u8(&b);
    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER)
        return AETHER_ERR_UNSUPPORTED;
    if (msg != AETHER_MSG_SERVER_SNAPSHOT) return AETHER_ERR_UNSUPPORTED;
    out->tick = aether_netbuf_read_u32(&b);
    out->time = aether_netbuf_read_f32(&b);
    u8 n = aether_netbuf_read_u8(&b);
    if (n > AETHER_NET_MAX_PLAYERS) n = AETHER_NET_MAX_PLAYERS;
    out->player_count = n;
    for (u8 i = 0; i < n; ++i) {
        aether_net_snapshot_player_t *p = &out->players[i];
        p->player_id = aether_netbuf_read_u32(&b);
        aether_netbuf_read_string(&b, p->name, AETHER_NET_MAX_NAME);
        p->score = aether_netbuf_read_i32(&b);
        p->deaths = aether_netbuf_read_i32(&b);
        p->ping_ms = aether_netbuf_read_i32(&b);
        p->origin[0] = aether_netbuf_read_f32(&b);
        p->origin[1] = aether_netbuf_read_f32(&b);
        p->origin[2] = aether_netbuf_read_f32(&b);
    }
    out->chat_from = aether_netbuf_read_u32(&b);
    aether_netbuf_read_string(&b, out->chat_line, AETHER_NET_MAX_CHAT);
    return AETHER_OK;
}

void aether_net_snapshot_apply_hud(const aether_net_snapshot_t *snap,
                                   aether_scoreboard_t *sb,
                                   aether_chat_log_t *chat,
                                   f32 now) {
    if (!snap) return;
    if (sb) {
        aether_scoreboard_clear(sb);
        u32 n = snap->player_count;
        if (n > AETHER_NET_MAX_PLAYERS) n = AETHER_NET_MAX_PLAYERS;
        for (u32 i = 0; i < n; ++i) {
            aether_scoreboard_entry_t *e = &sb->entries[i];
            e->player_id = snap->players[i].player_id;
            aether_str_copy(e->name, AETHER_NET_MAX_NAME, snap->players[i].name);
            e->score = snap->players[i].score;
            e->deaths = snap->players[i].deaths;
            e->ping_ms = snap->players[i].ping_ms;
            e->active = true;
        }
        sb->count = n;
        sb->visible = true;
    }
    if (chat && snap->chat_line[0]) {
        aether_chat_add(chat, snap->chat_from, snap->chat_line, now);
        aether_chat_set_visible(chat, true);
    }
}

void aether_net_snapshot_make_demo(aether_net_snapshot_t *snap, u32 tick, f32 time) {
    if (!snap) return;
    memset(snap, 0, sizeof(*snap));
    snap->tick = tick;
    snap->time = time;
    snap->player_count = 2;
    snap->players[0].player_id = 1;
    aether_str_copy(snap->players[0].name, AETHER_NET_MAX_NAME, "Freeman");
    snap->players[0].score = 10 + (i32)(tick % 5);
    snap->players[0].deaths = 1;
    snap->players[0].ping_ms = 32;
    snap->players[0].origin[2] = 40.f;
    snap->players[1].player_id = 2;
    aether_str_copy(snap->players[1].name, AETHER_NET_MAX_NAME, "Barney");
    snap->players[1].score = 7;
    snap->players[1].deaths = 3;
    snap->players[1].ping_ms = 48;
    snap->players[1].origin[0] = 64.f;
    snap->chat_from = 2;
    aether_str_copy(snap->chat_line, AETHER_NET_MAX_CHAT, "snapshot tick hud bridge");
}
