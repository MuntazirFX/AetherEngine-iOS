#include "AetherNetDelta.h"
#include "AetherNetBuffer.h"
#include <string.h>
#include <math.h>

static int player_changed(const aether_net_snapshot_player_t *a,
                          const aether_net_snapshot_player_t *b) {
    if (!a || !b) return 1;
    if (a->player_id != b->player_id) return 1;
    if (a->score != b->score || a->deaths != b->deaths || a->ping_ms != b->ping_ms) return 1;
    if (strncmp(a->name, b->name, AETHER_NET_MAX_NAME) != 0) return 1;
    for (int i = 0; i < 3; ++i) {
        if (fabsf(a->origin[i] - b->origin[i]) > 0.01f) return 1;
    }
    return 0;
}

static const aether_net_snapshot_player_t *find_player(
    const aether_net_snapshot_t *s, u32 id) {
    if (!s) return NULL;
    for (u32 i = 0; i < s->player_count && i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (s->players[i].player_id == id) return &s->players[i];
    }
    return NULL;
}

u32 aether_net_delta_changed_count(const aether_net_snapshot_t *baseline,
                                   const aether_net_snapshot_t *current) {
    if (!current) return 0;
    u32 n = 0;
    for (u32 i = 0; i < current->player_count && i < AETHER_NET_MAX_PLAYERS; ++i) {
        const aether_net_snapshot_player_t *cur = &current->players[i];
        const aether_net_snapshot_player_t *base = find_player(baseline, cur->player_id);
        if (!base || player_changed(base, cur)) n++;
    }
    return n;
}

u32 aether_net_delta_encode(const aether_net_snapshot_t *baseline,
                            const aether_net_snapshot_t *current,
                            u8 *out, u32 cap) {
    if (!current || !out || cap < 16) return 0;
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_SERVER_DELTA);
    aether_netbuf_write_u32(&b, current->tick);
    aether_netbuf_write_f32(&b, current->time);
    aether_netbuf_write_u32(&b, baseline ? baseline->tick : 0);

    u8 changed = (u8)aether_net_delta_changed_count(baseline, current);
    if (changed > AETHER_NET_MAX_PLAYERS) changed = AETHER_NET_MAX_PLAYERS;
    aether_netbuf_write_u8(&b, changed);

    u8 written = 0;
    for (u32 i = 0; i < current->player_count && written < changed; ++i) {
        const aether_net_snapshot_player_t *cur = &current->players[i];
        const aether_net_snapshot_player_t *base = find_player(baseline, cur->player_id);
        if (base && !player_changed(base, cur)) continue;
        aether_netbuf_write_u32(&b, cur->player_id);
        aether_netbuf_write_string(&b, cur->name, AETHER_NET_MAX_NAME);
        aether_netbuf_write_i32(&b, cur->score);
        aether_netbuf_write_i32(&b, cur->deaths);
        aether_netbuf_write_i32(&b, cur->ping_ms);
        aether_netbuf_write_f32(&b, cur->origin[0]);
        aether_netbuf_write_f32(&b, cur->origin[1]);
        aether_netbuf_write_f32(&b, cur->origin[2]);
        written++;
    }

    aether_netbuf_write_u32(&b, current->chat_from);
    aether_netbuf_write_string(&b, current->chat_line, AETHER_NET_MAX_CHAT);
    u32 sz = aether_netbuf_size(&b);
    if (sz > cap || b.overflow) return 0;
    memcpy(out, b.data, sz);
    return sz;
}

aether_result_t aether_net_delta_apply(const u8 *data, u32 size,
                                       aether_net_snapshot_t *inout) {
    if (!data || !inout || size < 8) return AETHER_ERR_INVALID_ARG;
    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);
    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver   = aether_netbuf_read_u16(&b);
    u8  msg   = aether_netbuf_read_u8(&b);
    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER)
        return AETHER_ERR_UNSUPPORTED;
    if (msg != AETHER_MSG_SERVER_DELTA) return AETHER_ERR_UNSUPPORTED;

    inout->tick = aether_netbuf_read_u32(&b);
    inout->time = aether_netbuf_read_f32(&b);
    (void)aether_netbuf_read_u32(&b); /* baseline tick */
    u8 n = aether_netbuf_read_u8(&b);
    if (n > AETHER_NET_MAX_PLAYERS) n = AETHER_NET_MAX_PLAYERS;

    for (u8 i = 0; i < n; ++i) {
        aether_net_snapshot_player_t tmp;
        memset(&tmp, 0, sizeof tmp);
        tmp.player_id = aether_netbuf_read_u32(&b);
        aether_netbuf_read_string(&b, tmp.name, AETHER_NET_MAX_NAME);
        tmp.score = aether_netbuf_read_i32(&b);
        tmp.deaths = aether_netbuf_read_i32(&b);
        tmp.ping_ms = aether_netbuf_read_i32(&b);
        tmp.origin[0] = aether_netbuf_read_f32(&b);
        tmp.origin[1] = aether_netbuf_read_f32(&b);
        tmp.origin[2] = aether_netbuf_read_f32(&b);

        /* Upsert into inout by player_id */
        int found = -1;
        for (u32 j = 0; j < inout->player_count && j < AETHER_NET_MAX_PLAYERS; ++j) {
            if (inout->players[j].player_id == tmp.player_id) { found = (int)j; break; }
        }
        if (found >= 0) {
            inout->players[found] = tmp;
        } else if (inout->player_count < AETHER_NET_MAX_PLAYERS) {
            inout->players[inout->player_count++] = tmp;
        }
    }
    inout->chat_from = aether_netbuf_read_u32(&b);
    aether_netbuf_read_string(&b, inout->chat_line, AETHER_NET_MAX_CHAT);
    return AETHER_OK;
}
