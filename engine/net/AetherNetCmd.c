#include "AetherNetCmd.h"
#include "AetherNetBuffer.h"
#include <math.h>
#include <string.h>

#ifndef AETHER_PI
#define AETHER_PI 3.14159265358979323846f
#endif

void aether_net_cmd_clear(aether_net_cmd_t *cmd) {
    if (cmd) memset(cmd, 0, sizeof(*cmd));
}

void aether_net_cmd_from_move(aether_net_cmd_t *cmd, f32 forward, f32 side, f32 up,
                              f32 yaw_deg, f32 pitch_deg, u32 buttons, f32 dt, u32 seq) {
    if (!cmd) return;
    cmd->seq = seq;
    cmd->forward = forward;
    cmd->side = side;
    cmd->up = up;
    cmd->yaw_deg = yaw_deg;
    cmd->pitch_deg = pitch_deg;
    cmd->buttons = buttons;
    cmd->dt = dt > 0.f ? dt : (1.f / 60.f);
}

u32 aether_net_cmd_encode(const aether_net_cmd_t *cmd, u8 *out, u32 cap) {
    if (!cmd || !out || cap < 40) return 0;
    aether_netbuf_t b;
    aether_netbuf_init_write(&b);
    aether_netbuf_write_u32(&b, AETHER_NET_PROTOCOL_ID);
    aether_netbuf_write_u16(&b, AETHER_NET_PROTOCOL_VER);
    aether_netbuf_write_u8 (&b, AETHER_MSG_CLIENT_CMD);
    aether_netbuf_write_u32(&b, cmd->seq);
    aether_netbuf_write_f32(&b, cmd->forward);
    aether_netbuf_write_f32(&b, cmd->side);
    aether_netbuf_write_f32(&b, cmd->up);
    aether_netbuf_write_f32(&b, cmd->yaw_deg);
    aether_netbuf_write_f32(&b, cmd->pitch_deg);
    aether_netbuf_write_u32(&b, cmd->buttons);
    aether_netbuf_write_f32(&b, cmd->dt);
    u32 n = aether_netbuf_size(&b);
    if (n > cap) return 0;
    memcpy(out, b.data, n);
    return n;
}

aether_result_t aether_net_cmd_decode_payload(const u8 *payload, u32 size, aether_net_cmd_t *out) {
    if (!payload || !out || size < 32) return AETHER_ERR_INVALID_ARG;
    aether_netbuf_t b;
    aether_netbuf_init_read(&b, payload, size);
    out->seq = aether_netbuf_read_u32(&b);
    out->forward = aether_netbuf_read_f32(&b);
    out->side = aether_netbuf_read_f32(&b);
    out->up = aether_netbuf_read_f32(&b);
    out->yaw_deg = aether_netbuf_read_f32(&b);
    out->pitch_deg = aether_netbuf_read_f32(&b);
    out->buttons = aether_netbuf_read_u32(&b);
    out->dt = aether_netbuf_read_f32(&b);
    if (out->dt <= 0.f) out->dt = 1.f / 60.f;
    if (out->dt > 0.25f) out->dt = 0.25f;
    return AETHER_OK;
}

aether_result_t aether_net_cmd_decode(const u8 *data, u32 size, aether_net_cmd_t *out) {
    if (!data || !out || size < 39) return AETHER_ERR_INVALID_ARG;
    aether_netbuf_t b;
    aether_netbuf_init_read(&b, data, size);
    u32 magic = aether_netbuf_read_u32(&b);
    u16 ver = aether_netbuf_read_u16(&b);
    u8 msg = aether_netbuf_read_u8(&b);
    if (magic != AETHER_NET_PROTOCOL_ID || ver != AETHER_NET_PROTOCOL_VER ||
        msg != AETHER_MSG_CLIENT_CMD)
        return AETHER_ERR_INVALID_ARG;
    return aether_net_cmd_decode_payload(data + 7, size - 7, out);
}

void aether_net_cmd_history_init(aether_net_cmd_history_t *h) {
    if (!h) return;
    memset(h, 0, sizeof(*h));
}

void aether_net_cmd_history_push(aether_net_cmd_history_t *h, const aether_net_cmd_t *cmd, f32 now) {
    if (!h || !cmd) return;
    h->cmds[h->head % AETHER_NET_CMD_HISTORY] = *cmd;
    h->head++;
    if (h->count < AETHER_NET_CMD_HISTORY) h->count++;
    h->time_accum = now;
}

const aether_net_cmd_t *aether_net_cmd_history_latest(const aether_net_cmd_history_t *h) {
    if (!h || h->count == 0) return NULL;
    u32 idx = (h->head + AETHER_NET_CMD_HISTORY - 1) % AETHER_NET_CMD_HISTORY;
    return &h->cmds[idx];
}

const aether_net_cmd_t *aether_net_cmd_history_at_lag(const aether_net_cmd_history_t *h,
                                                     f32 now, f32 lag_ms) {
    if (!h || h->count == 0) return NULL;
    (void)now;
    /* Simple stub: walk back ~lag/dt frames from newest. */
    f32 lag_s = lag_ms * 0.001f;
    if (lag_s < 0.f) lag_s = 0.f;
    u32 steps = (u32)(lag_s * 60.f + 0.5f);
    if (steps >= h->count) steps = h->count - 1;
    u32 idx = (h->head + AETHER_NET_CMD_HISTORY - 1 - steps) % AETHER_NET_CMD_HISTORY;
    return &h->cmds[idx];
}

void aether_net_cmd_apply_move(aether_vec3_t *origin, f32 *yaw_deg,
                               const aether_net_cmd_t *cmd, f32 speed) {
    if (!origin || !cmd) return;
    if (yaw_deg) *yaw_deg = cmd->yaw_deg;
    f32 yaw = cmd->yaw_deg * (AETHER_PI / 180.f);
    f32 cy = cosf(yaw), sy = sinf(yaw);
    f32 fx = cy, fy = sy;
    f32 rx = -sy, ry = cy;
    f32 mx = fx * cmd->forward + rx * cmd->side;
    f32 my = fy * cmd->forward + ry * cmd->side;
    f32 len = sqrtf(mx * mx + my * my);
    if (len > 1e-4f) { mx /= len; my /= len; }
    f32 sp = speed > 0.f ? speed : 320.f;
    f32 dt = cmd->dt > 0.f ? cmd->dt : (1.f / 60.f);
    if (dt > 0.25f) dt = 0.25f;
    origin->x += mx * sp * dt;
    origin->y += my * sp * dt;
    if (cmd->buttons & 2u) origin->z += 20.f * dt; /* jump hint */
    if (cmd->buttons & 4u) origin->z -= 10.f * dt; /* duck hint */
}
