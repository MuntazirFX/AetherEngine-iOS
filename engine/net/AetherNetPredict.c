#include "AetherNetPredict.h"
#include "../player/AetherCollision.h"
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
    pr->error_decay = 12.f;
    pr->teleport_threshold = AETHER_NET_PREDICT_TELEPORT_DEFAULT;
    pr->teleported = false;
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


void aether_net_predict_set_collision(aether_net_predict_t *pr, aether_collision_t *col) {
    if (!pr) return;
    pr->collision = col;
}

aether_collision_t *aether_net_predict_get_collision(const aether_net_predict_t *pr) {
    return pr ? pr->collision : NULL;
}

void aether_net_predict_apply_cmd_clipped(aether_net_predict_t *pr,
                                          const aether_net_predict_cmd_t *cmd,
                                          i32 hull_index) {
    if (!pr || !cmd || !pr->active) return;
    f32 yaw = cmd->yaw_deg * (AETHER_PI / 180.f);
    f32 cy = cosf(yaw), sy = sinf(yaw);
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
    aether_vec3_t from = { pr->origin[0], pr->origin[1], pr->origin[2] };
    aether_vec3_t to = {
        pr->origin[0] + pr->velocity[0] * dt,
        pr->origin[1] + pr->velocity[1] * dt,
        pr->origin[2] + pr->velocity[2] * dt
    };
    if (pr->collision) {
        if (hull_index < 1) hull_index = 1;
        bool ground = false;
        aether_vec3_t resolved = aether_collision_move(pr->collision, from, to,
                                                       hull_index, AETHER_DEFAULT_STEP_HEIGHT,
                                                       &ground);
        pr->origin[0] = resolved.x;
        pr->origin[1] = resolved.y;
        pr->origin[2] = resolved.z;
        pr->on_ground = ground;
    } else {
        pr->origin[0] = to.x;
        pr->origin[1] = to.y;
        pr->origin[2] = to.z;
    }
    pr->cmd_seq = cmd->seq ? cmd->seq : (pr->cmd_seq + 1);
}

void aether_net_predict_set_error_decay(aether_net_predict_t *pr, f32 rate) {
    if (!pr) return;
    if (rate < 0.f) rate = 0.f;
    pr->error_decay = rate;
}

f32 aether_net_predict_error_length(const aether_net_predict_t *pr) {
    if (!pr) return 0.f;
    return sqrtf(pr->error[0]*pr->error[0] + pr->error[1]*pr->error[1] + pr->error[2]*pr->error[2]);
}

f32 aether_net_predict_smooth_tick(aether_net_predict_t *pr, f32 dt) {
    if (!pr || !pr->active) return 0.f;
    if (dt < 0.f) dt = 0.f;
    if (dt > 0.25f) dt = 0.25f;
    f32 rate = pr->error_decay > 0.f ? pr->error_decay : 12.f;
    /* Exponential decay: err *= exp(-rate*dt) ≈ 1 - rate*dt for small dt.
     * Apply the portion we remove to origin so motion eases toward auth. */
    f32 keep = expf(-rate * dt);
    if (keep < 0.f) keep = 0.f;
    if (keep > 1.f) keep = 1.f;
    f32 apply = 1.f - keep;
    pr->origin[0] += pr->error[0] * apply;
    pr->origin[1] += pr->error[1] * apply;
    pr->origin[2] += pr->error[2] * apply;
    pr->error[0] *= keep;
    pr->error[1] *= keep;
    pr->error[2] *= keep;
    /* Snap tiny residuals */
    if (aether_net_predict_error_length(pr) < 0.05f) {
        pr->origin[0] += pr->error[0];
        pr->origin[1] += pr->error[1];
        pr->origin[2] += pr->error[2];
        pr->error[0] = pr->error[1] = pr->error[2] = 0.f;
    }
    return aether_net_predict_error_length(pr);
}

void aether_net_predict_reconcile_smooth(aether_net_predict_t *pr,
                                         const aether_net_snapshot_t *snap,
                                         f32 snap_blend, f32 dt) {
    if (!pr || !snap) return;
    /* Hard-snap fraction, leave residual in error[] for smooth_tick. */
    if (snap_blend < 0.f) snap_blend = 0.f;
    if (snap_blend > 1.f) snap_blend = 1.f;
    aether_net_predict_reconcile(pr, snap, snap_blend);
    /* After reconcile, error[] holds full delta *before* blend was applied
     * in current reconcile — actually reconcile stores full delta then applies
     * blend to origin, leaving error as full auth-predict. Reduce residual to
     * the unapplied portion: error *= (1-blend). */
    f32 rem = 1.f - snap_blend;
    pr->error[0] *= rem;
    pr->error[1] *= rem;
    pr->error[2] *= rem;
    if (pr->error_decay <= 0.f) pr->error_decay = 12.f;
    (void)aether_net_predict_smooth_tick(pr, dt);
}

void aether_net_predict_set_teleport_threshold(aether_net_predict_t *pr, f32 units) {
    if (!pr) return;
    if (units < 1.f) units = 1.f;
    pr->teleport_threshold = units;
}

f32 aether_net_predict_get_teleport_threshold(const aether_net_predict_t *pr) {
    if (!pr) return AETHER_NET_PREDICT_TELEPORT_DEFAULT;
    return pr->teleport_threshold > 0.f ? pr->teleport_threshold : AETHER_NET_PREDICT_TELEPORT_DEFAULT;
}

int aether_net_predict_reconcile_teleport(aether_net_predict_t *pr,
                                          const aether_net_snapshot_t *snap,
                                          f32 soft_blend) {
    if (!pr || !snap) return 0;
    pr->teleported = false;
    const aether_net_snapshot_player_t *found = NULL;
    for (u32 i = 0; i < snap->player_count && i < AETHER_NET_MAX_PLAYERS; ++i) {
        if (snap->players[i].player_id == pr->local_id) {
            found = &snap->players[i];
            break;
        }
    }
    if (!found) return 0;
    f32 dx = found->origin[0] - pr->origin[0];
    f32 dy = found->origin[1] - pr->origin[1];
    f32 dz = found->origin[2] - pr->origin[2];
    f32 len = sqrtf(dx*dx + dy*dy + dz*dz);
    f32 thresh = aether_net_predict_get_teleport_threshold(pr);
    if (len >= thresh) {
        /* Hard snap — teleport / large correction edge case. */
        pr->origin[0] = found->origin[0];
        pr->origin[1] = found->origin[1];
        pr->origin[2] = found->origin[2];
        pr->error[0] = pr->error[1] = pr->error[2] = 0.f;
        pr->last_ack_tick = snap->tick;
        pr->teleported = true;
        return 1;
    }
    aether_net_predict_reconcile(pr, snap, soft_blend);
    return 0;
}
