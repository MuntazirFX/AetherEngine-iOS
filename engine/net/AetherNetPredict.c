#include "AetherNetPredict.h"
#include <math.h>
#include <string.h>

#ifndef AETHER_PI
#define AETHER_PI 3.14159265358979323846f
#endif

void aether_net_predict_init(aether_net_predict_t *pr, u32 local_id) {
    if (!pr) return;
    memset(pr, 0, sizeof(*pr));
    pr->local_id = local_id;
    pr->speed = 320.f; /* GoldSrc-ish walk */
    pr->active = true;
}

void aether_net_predict_set_speed(aether_net_predict_t *pr, f32 speed) {
    if (!pr) return;
    if (speed < 0.f) speed = 0.f;
    pr->speed = speed;
}

void aether_net_predict_apply_cmd(aether_net_predict_t *pr, const aether_net_predict_cmd_t *cmd) {
    if (!pr || !cmd || !pr->active) return;
    f32 yaw = cmd->yaw_deg * (AETHER_PI / 180.f);
    f32 cy = cosf(yaw), sy = sinf(yaw);
    /* Forward is +X when yaw=0 in engine Z-up convention used elsewhere; use XY plane. */
    f32 fx = cy, fy = sy;
    f32 rx = -sy, ry = cy;
    f32 mx = fx * cmd->forward + rx * cmd->side;
    f32 my = fy * cmd->forward + ry * cmd->side;
    f32 len = sqrtf(mx*mx + my*my);
    if (len > 1e-4f) { mx /= len; my /= len; }
    f32 dt = cmd->dt > 0.f ? cmd->dt : (1.f/60.f);
    if (dt > 0.25f) dt = 0.25f;
    pr->velocity[0] = mx * pr->speed;
    pr->velocity[1] = my * pr->speed;
    pr->velocity[2] = 0.f;
    pr->origin[0] += pr->velocity[0] * dt;
    pr->origin[1] += pr->velocity[1] * dt;
    pr->cmd_seq = cmd->seq ? cmd->seq : (pr->cmd_seq + 1);
}

void aether_net_predict_reconcile(aether_net_predict_t *pr,
                                  const aether_net_snapshot_t *snap,
                                  f32 blend) {
    if (!pr || !snap) return;
    if (blend < 0.f) blend = 0.f;
    if (blend > 1.f) blend = 1.f;
    const aether_net_snapshot_player_t *found = NULL;
    for (u32 i = 0; i < snap->player_count && i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (snap->players[i].player_id == pr->local_id) {
            found = &snap->players[i];
            break;
        }
    }
    if (!found) return;
    pr->error[0] = found->origin[0] - pr->origin[0];
    pr->error[1] = found->origin[1] - pr->origin[1];
    pr->error[2] = found->origin[2] - pr->origin[2];
    pr->origin[0] += pr->error[0] * blend;
    pr->origin[1] += pr->error[1] * blend;
    pr->origin[2] += pr->error[2] * blend;
    pr->last_ack_tick = snap->tick;
}

void aether_net_predict_get_origin(const aether_net_predict_t *pr, f32 out[3]) {
    if (!out) return;
    if (!pr) { out[0]=out[1]=out[2]=0; return; }
    out[0]=pr->origin[0]; out[1]=pr->origin[1]; out[2]=pr->origin[2];
}
