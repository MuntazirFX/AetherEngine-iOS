/* AetherNetCmd.h — Client input cmd stream + history (lag-comp stub).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_CMD_H
#define AETHER_NET_CMD_H

#include "AetherNetProtocol.h"
#include "../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Wire payload after 7-byte header for AETHER_MSG_CLIENT_CMD. */
typedef struct aether_net_cmd {
    u32 seq;
    f32 forward;   /* -1..1 move along yaw */
    f32 side;      /* -1..1 strafe */
    f32 up;        /* jump/duck hint (-1..1) */
    f32 yaw_deg;
    f32 pitch_deg;
    u32 buttons;   /* bitfield: 1=attack, 2=jump, 4=duck, 8=use */
    f32 dt;        /* client frame dt hint */
} aether_net_cmd_t;

#define AETHER_NET_CMD_HISTORY 16

typedef struct aether_net_cmd_history {
    aether_net_cmd_t cmds[AETHER_NET_CMD_HISTORY];
    u32 head;      /* next write slot */
    u32 count;
    f32 time_accum; /* server-side time of newest cmd */
} aether_net_cmd_history_t;

void aether_net_cmd_clear(aether_net_cmd_t *cmd);
void aether_net_cmd_from_move(aether_net_cmd_t *cmd, f32 forward, f32 side, f32 up,
                              f32 yaw_deg, f32 pitch_deg, u32 buttons, f32 dt, u32 seq);

/* Encode full CLIENT_CMD packet (header + payload). Returns bytes or 0. */
u32 aether_net_cmd_encode(const aether_net_cmd_t *cmd, u8 *out, u32 cap);

/* Decode payload after magic/ver/msg already consumed, OR full packet. */
aether_result_t aether_net_cmd_decode(const u8 *data, u32 size, aether_net_cmd_t *out);
aether_result_t aether_net_cmd_decode_payload(const u8 *payload, u32 size, aether_net_cmd_t *out);

void aether_net_cmd_history_init(aether_net_cmd_history_t *h);
void aether_net_cmd_history_push(aether_net_cmd_history_t *h, const aether_net_cmd_t *cmd, f32 now);
const aether_net_cmd_t *aether_net_cmd_history_latest(const aether_net_cmd_history_t *h);
/* Lag-comp stub: find cmd closest to (now - lag_ms). Returns NULL if empty. */
const aether_net_cmd_t *aether_net_cmd_history_at_lag(const aether_net_cmd_history_t *h,
                                                     f32 now, f32 lag_ms);

/* Apply cmd to origin/yaw (server authority stub, no collision). */
void aether_net_cmd_apply_move(aether_vec3_t *origin, f32 *yaw_deg,
                               const aether_net_cmd_t *cmd, f32 speed);

#ifdef __cplusplus
}
#endif
#endif
