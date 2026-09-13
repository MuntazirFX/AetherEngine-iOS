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
