/* AetherNetChat.h — Client chat log buffer.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_CHAT_H
#define AETHER_NET_CHAT_H

#include "AetherNetProtocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_CHAT_LOG_SIZE    32

typedef struct aether_chat_line {
    u32   player_id;    /* 0 = system */
    char  text[AETHER_NET_MAX_CHAT];
    f32   time_added;
} aether_chat_line_t;

typedef struct aether_chat_log {
    aether_chat_line_t lines[AETHER_CHAT_LOG_SIZE];
    u32                head;      /* next write position */
    u32                count;
    bool               visible;
    char               input[AETHER_NET_MAX_CHAT];  /* current input */
    u32                input_len;
    u8                 last_cue_kind; /* aether_chat_cue_kind_t from last net apply */
} aether_chat_log_t;

void aether_chat_init(aether_chat_log_t *cl);
void aether_chat_add(aether_chat_log_t *cl, u32 player_id, const char *text, f32 now);
void aether_chat_add_system(aether_chat_log_t *cl, const char *text, f32 now);
void aether_chat_set_visible(aether_chat_log_t *cl, bool visible);
void aether_chat_handle_packet(aether_chat_log_t *cl, const u8 *data, u32 size, f32 now);
void aether_chat_dump(const aether_chat_log_t *cl);

/* Voice/chat cue kinds for HUD sync stub. */
typedef enum aether_chat_cue_kind {
    AETHER_CHAT_CUE_TEXT  = 0,
    AETHER_CHAT_CUE_VOICE = 1
} aether_chat_cue_kind_t;

/* Encode CHAT packet (optionally voice cue). Returns bytes. */
u32 aether_chat_encode(u8 *out, u32 cap, u32 player_id, const char *text);
u32 aether_chat_encode_voice_cue(u8 *out, u32 cap, u32 player_id, const char *cue);

/* Last cue kind applied by handle_packet (0=text, 1=voice). */
u8  aether_chat_last_cue_kind(const aether_chat_log_t *cl);

/* Apply encoded chat/voice packet → log (same as handle_packet; returns lines added). */
u32 aether_chat_apply_net(aether_chat_log_t *cl, const u8 *data, u32 size, f32 now);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_CHAT_H */
