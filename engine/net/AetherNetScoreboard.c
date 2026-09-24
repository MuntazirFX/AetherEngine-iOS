/* AetherNetScoreboard.c — Scoreboard implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetScoreboard.h"
#include "AetherNetBuffer.h"
#include <string.h>

void aether_scoreboard_init(aether_scoreboard_t *sb) {
    if (!sb) return;
    memset(sb, 0, sizeof *sb);
}

void aether_scoreboard_clear(aether_scoreboard_t *sb) {
    if (!sb) return;
    memset(sb->entries, 0, sizeof sb->entries);
    sb->count = 0;
}

void aether_scoreboard_set_visible(aether_scoreboard_t *sb, bool visible) {
    if (sb) sb->visible = visible;
}

void aether_scoreboard_update_from_packet(aether_scoreboard_t *sb,
                                           const u8 *data, u32 size) {
    if (!sb || !data || size < 8) return;

    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);

    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver   = aether_netbuf_read_u16(&b);
    u8  msg   = aether_netbuf_read_u8(&b);
    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER) return;
    if (msg != AETHER_MSG_SCOREBOARD) return;

    u8 n = aether_netbuf_read_u8(&b);
    if (n > AETHER_NET_MAX_PLAYERS) n = AETHER_NET_MAX_PLAYERS;

    aether_scoreboard_clear(sb);
    for (u8 i = 0; i < n; ++i) {
        aether_scoreboard_entry_t *e = &sb->entries[i];
        e->player_id = aether_netbuf_read_u32(&b);
        aether_netbuf_read_string(&b, e->name, AETHER_NET_MAX_NAME);
        e->score   = aether_netbuf_read_i32(&b);
        e->deaths  = aether_netbuf_read_i32(&b);
        e->ping_ms = aether_netbuf_read_i32(&b);
        e->active  = true;
    }
    sb->count = n;
}

void aether_scoreboard_dump(const aether_scoreboard_t *sb) {
    if (!sb) return;
    aether_log(AETHER_LOG_INFO, "scoreboard", "===== SCOREBOARD (%u) =====", sb->count);
    for (u32 i = 0; i < sb->count; ++i) {
        const aether_scoreboard_entry_t *e = &sb->entries[i];
        aether_log(AETHER_LOG_INFO, "scoreboard",
                   "  #%-3u %-20s score=%d deaths=%d ping=%dms",
                   e->player_id, e->name, e->score, e->deaths, e->ping_ms);
    }
}

void aether_scoreboard_events_init(aether_scoreboard_events_t *ev) {
    if (!ev) return;
    memset(ev, 0, sizeof(*ev));
}
void aether_scoreboard_events_clear(aether_scoreboard_events_t *ev) {
    if (!ev) return;
    memset(ev, 0, sizeof(*ev));
}
void aether_scoreboard_events_push(aether_scoreboard_events_t *ev,
                                   aether_scoreboard_event_kind_t kind,
                                   u32 player_id, const char *name, f32 time) {
    if (!ev) return;
    aether_scoreboard_event_t *e = &ev->items[ev->head % AETHER_SCOREBOARD_MAX_EVENTS];
    memset(e, 0, sizeof(*e));
    e->kind = (u8)kind;
    e->player_id = player_id;
    e->time = time;
    if (name) {
        size_t n = strlen(name);
        if (n >= AETHER_NET_MAX_NAME) n = AETHER_NET_MAX_NAME - 1;
        memcpy(e->name, name, n);
    }
    ev->head = (ev->head + 1) % AETHER_SCOREBOARD_MAX_EVENTS;
    ev->count++;
    if (ev->live < AETHER_SCOREBOARD_MAX_EVENTS) ev->live++;
}
u32 aether_scoreboard_events_live(const aether_scoreboard_events_t *ev) {
    return ev ? ev->live : 0;
}
int aether_scoreboard_events_get(const aether_scoreboard_events_t *ev, u32 index,
                                 aether_scoreboard_event_t *out) {
    if (!ev || !out || index >= ev->live) return 0;
    /* oldest = (head - live + index) mod MAX */
    u32 start = (ev->head + AETHER_SCOREBOARD_MAX_EVENTS - ev->live) % AETHER_SCOREBOARD_MAX_EVENTS;
    u32 i = (start + index) % AETHER_SCOREBOARD_MAX_EVENTS;
    *out = ev->items[i];
    return 1;
}

void aether_scoreboard_apply_join(aether_scoreboard_t *sb,
                                  aether_scoreboard_events_t *ev,
                                  u32 player_id, const char *name, f32 time) {
    if (!sb) return;
    /* Update existing or append. */
    for (u32 i = 0; i < sb->count; ++i) {
        if (sb->entries[i].player_id == player_id) {
            sb->entries[i].active = true;
            if (name && name[0]) {
                memset(sb->entries[i].name, 0, sizeof sb->entries[i].name);
                size_t n = strlen(name);
                if (n >= AETHER_NET_MAX_NAME) n = AETHER_NET_MAX_NAME - 1;
                memcpy(sb->entries[i].name, name, n);
            }
            if (ev) aether_scoreboard_events_push(ev, AETHER_SB_EVENT_JOIN, player_id,
                                                  sb->entries[i].name, time);
            return;
        }
    }
    if (sb->count >= AETHER_NET_MAX_PLAYERS) return;
    aether_scoreboard_entry_t *e = &sb->entries[sb->count++];
    memset(e, 0, sizeof(*e));
    e->player_id = player_id;
    e->active = true;
    if (name) {
        size_t n = strlen(name);
        if (n >= AETHER_NET_MAX_NAME) n = AETHER_NET_MAX_NAME - 1;
        memcpy(e->name, name, n);
    }
    if (ev) aether_scoreboard_events_push(ev, AETHER_SB_EVENT_JOIN, player_id, e->name, time);
}

void aether_scoreboard_apply_leave(aether_scoreboard_t *sb,
                                   aether_scoreboard_events_t *ev,
                                   u32 player_id, f32 time) {
    if (!sb) return;
    char name[AETHER_NET_MAX_NAME];
    name[0] = 0;
    for (u32 i = 0; i < sb->count; ++i) {
        if (sb->entries[i].player_id != player_id) continue;
        memcpy(name, sb->entries[i].name, AETHER_NET_MAX_NAME);
        /* Compact: swap with last */
        sb->entries[i] = sb->entries[sb->count - 1];
        memset(&sb->entries[sb->count - 1], 0, sizeof(sb->entries[0]));
        sb->count--;
        break;
    }
    if (ev) aether_scoreboard_events_push(ev, AETHER_SB_EVENT_LEAVE, player_id, name, time);
}

u32 aether_scoreboard_encode_join(u8 *out, u32 cap,
                                  u32 player_id, const char *name) {
    if (!out || cap < 16) return 0;
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8(&b, AETHER_MSG_PLAYER_JOIN);
    aether_netbuf_write_u32(&b, player_id);
    aether_netbuf_write_string(&b, name ? name : "", AETHER_NET_MAX_NAME);
    u32 n = aether_netbuf_size(&b);
    if (n > cap || b.overflow) return 0;
    memcpy(out, b.data, n);
    return n;
}

u32 aether_scoreboard_encode_leave(u8 *out, u32 cap, u32 player_id) {
    if (!out || cap < 12) return 0;
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8(&b, AETHER_MSG_PLAYER_LEAVE);
    aether_netbuf_write_u32(&b, player_id);
    u32 n = aether_netbuf_size(&b);
    if (n > cap || b.overflow) return 0;
    memcpy(out, b.data, n);
    return n;
}

void aether_scoreboard_handle_packet(aether_scoreboard_t *sb,
                                     aether_scoreboard_events_t *ev,
                                     const u8 *data, u32 size, f32 time) {
    if (!sb || !data || size < 8) return;
    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);
    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver   = aether_netbuf_read_u16(&b);
    u8  msg   = aether_netbuf_read_u8(&b);
    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER) return;
    if (msg == AETHER_MSG_SCOREBOARD) {
        aether_scoreboard_update_from_packet(sb, data, size);
        return;
    }
    if (msg == AETHER_MSG_PLAYER_JOIN) {
        u32 id = aether_netbuf_read_u32(&b);
        char name[AETHER_NET_MAX_NAME];
        aether_netbuf_read_string(&b, name, sizeof name);
        aether_scoreboard_apply_join(sb, ev, id, name, time);
        return;
    }
    if (msg == AETHER_MSG_PLAYER_LEAVE) {
        u32 id = aether_netbuf_read_u32(&b);
        aether_scoreboard_apply_leave(sb, ev, id, time);
        return;
    }
}
