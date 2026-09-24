/* AetherNetChat.c — Chat log implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetChat.h"
#include "AetherNetBuffer.h"
#include <string.h>
#include <stdio.h>

void aether_chat_init(aether_chat_log_t *cl) {
    if (!cl) return;
    memset(cl, 0, sizeof *cl);
}

void aether_chat_add(aether_chat_log_t *cl, u32 player_id, const char *text, f32 now) {
    if (!cl || !text) return;
    aether_chat_line_t *l = &cl->lines[cl->head];
    l->player_id = player_id;
    aether_str_copy(l->text, AETHER_NET_MAX_CHAT, text);
    l->time_added = now;
    cl->head = (cl->head + 1) % AETHER_CHAT_LOG_SIZE;
    if (cl->count < AETHER_CHAT_LOG_SIZE) cl->count++;
}

void aether_chat_add_system(aether_chat_log_t *cl, const char *text, f32 now) {
    aether_chat_add(cl, 0, text, now);
}

void aether_chat_set_visible(aether_chat_log_t *cl, bool visible) {
    if (cl) cl->visible = visible;
}

void aether_chat_handle_packet(aether_chat_log_t *cl, const u8 *data, u32 size, f32 now) {
    if (!cl || !data || size < 8) return;
    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);
    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver   = aether_netbuf_read_u16(&b);
    u8  msg   = aether_netbuf_read_u8(&b);
    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER) return;
    if (msg != AETHER_MSG_CHAT) return;
    u32 pid = aether_netbuf_read_u32(&b);
    u8 kind = AETHER_CHAT_CUE_TEXT;
    /* Dual-format: new packets insert cue_kind (0/1) before length-prefixed string.
     * Heuristic: if next byte is 0 or 1 AND remaining after it is length-prefixed text, treat as kind. */
    if (b.read_pos < b.write_pos) {
        u8 peek = b.data[b.read_pos];
        if (peek <= (u8)AETHER_CHAT_CUE_VOICE) {
            u32 after = b.read_pos + 1;
            if (after < b.write_pos) {
                u8 slen = b.data[after];
                if ((u32)slen + after + 1 <= b.write_pos) {
                    kind = aether_netbuf_read_u8(&b);
                }
            }
        }
    }
    char text[AETHER_NET_MAX_CHAT] = {0};
    aether_netbuf_read_string(&b, text, AETHER_NET_MAX_CHAT);
    cl->last_cue_kind = kind;
    aether_chat_add(cl, pid, text, now);
    aether_chat_set_visible(cl, true);
}

void aether_chat_dump(const aether_chat_log_t *cl) {
    if (!cl) return;
    aether_log(AETHER_LOG_INFO, "chat", "===== CHAT (%u) =====", cl->count);
    u32 start = (cl->count < AETHER_CHAT_LOG_SIZE) ? 0 : cl->head;
    for (u32 i = 0; i < cl->count; ++i) {
        const aether_chat_line_t *l = &cl->lines[(start + i) % AETHER_CHAT_LOG_SIZE];
        if (l->player_id == 0)
            aether_log(AETHER_LOG_INFO, "chat", "  [sys] %s", l->text);
        else
            aether_log(AETHER_LOG_INFO, "chat", "  #%u: %s", l->player_id, l->text);
    }
}

u32 aether_chat_encode(u8 *out, u32 cap, u32 player_id, const char *text) {
    if (!out || cap < 16) return 0;
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8(&b, AETHER_MSG_CHAT);
    aether_netbuf_write_u32(&b, player_id);
    aether_netbuf_write_u8(&b, (u8)AETHER_CHAT_CUE_TEXT);
    aether_netbuf_write_string(&b, text ? text : "", AETHER_NET_MAX_CHAT);
    u32 n = aether_netbuf_size(&b);
    if (n > cap || b.overflow) return 0;
    memcpy(out, b.data, n);
    return n;
}

u32 aether_chat_encode_voice_cue(u8 *out, u32 cap, u32 player_id, const char *cue) {
    if (!out || cap < 16) return 0;
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8(&b, AETHER_MSG_CHAT);
    aether_netbuf_write_u32(&b, player_id);
    aether_netbuf_write_u8(&b, (u8)AETHER_CHAT_CUE_VOICE);
    /* Prefix voice cues so HUD can show a voice icon stub. */
    char buf[AETHER_NET_MAX_CHAT];
    buf[0] = 0;
    if (cue && cue[0]) {
        snprintf(buf, sizeof buf, "[voice] %s", cue);
    } else {
        snprintf(buf, sizeof buf, "[voice]");
    }
    aether_netbuf_write_string(&b, buf, AETHER_NET_MAX_CHAT);
    u32 n = aether_netbuf_size(&b);
    if (n > cap || b.overflow) return 0;
    memcpy(out, b.data, n);
    return n;
}

u8 aether_chat_last_cue_kind(const aether_chat_log_t *cl) {
    return cl ? cl->last_cue_kind : 0;
}

u32 aether_chat_apply_net(aether_chat_log_t *cl, const u8 *data, u32 size, f32 now) {
    if (!cl || !data || size < 8) return 0;
    u32 before = cl->count;
    aether_chat_handle_packet(cl, data, size, now);
    return (cl->count >= before) ? 1u : 0u;
}
