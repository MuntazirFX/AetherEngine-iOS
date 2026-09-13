/* AetherNetChat.c — Chat log implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherNetChat.h"
#include "AetherNetBuffer.h"
#include <string.h>

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
    char text[AETHER_NET_MAX_CHAT] = {0};
    aether_netbuf_read_string(&b, text, AETHER_NET_MAX_CHAT);
    aether_chat_add(cl, pid, text, now);
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
