#include "AetherNetSpectator.h"
#include <math.h>
#include <string.h>

void aether_spectator_init(aether_spectator_t *sp) {
    if (!sp) return;
    memset(sp, 0, sizeof(*sp));
    sp->follow_distance = 96.f;
    sp->follow_height = 24.f;
    sp->smooth = 0.25f;
    sp->forward[0] = 1.f;
}

void aether_spectator_set_enabled(aether_spectator_t *sp, bool enabled) {
    if (sp) sp->enabled = enabled;
}

void aether_spectator_follow(aether_spectator_t *sp, u32 player_id) {
    if (!sp) return;
    sp->enabled = true;
    sp->target_player_id = player_id;
    sp->following = true;
}

void aether_spectator_stop(aether_spectator_t *sp) {
    if (!sp) return;
    sp->following = false;
    sp->target_player_id = 0;
}

static void norm3(f32 v[3]) {
    f32 L = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (L < 1e-5f) { v[0]=1; v[1]=0; v[2]=0; return; }
    v[0]/=L; v[1]/=L; v[2]/=L;
}

int aether_spectator_tick(aether_spectator_t *sp, f32 dt,
                          const f32 target_pos[3], const f32 target_fwd[3]) {
    (void)dt;
    if (!sp || !sp->enabled || !sp->following || !target_pos) return 0;
    f32 fwd[3] = {1,0,0};
    if (target_fwd) { fwd[0]=target_fwd[0]; fwd[1]=target_fwd[1]; fwd[2]=target_fwd[2]; }
    norm3(fwd);
    /* Desired camera: behind target along -fwd, raised */
    f32 desired[3] = {
        target_pos[0] - fwd[0] * sp->follow_distance,
        target_pos[1] - fwd[1] * sp->follow_distance,
        target_pos[2] + sp->follow_height
    };
    f32 a = sp->smooth;
    if (a < 0.f) a = 0.f;
    if (a > 1.f) a = 1.f;
    sp->eye[0] += (desired[0] - sp->eye[0]) * a;
    sp->eye[1] += (desired[1] - sp->eye[1]) * a;
    sp->eye[2] += (desired[2] - sp->eye[2]) * a;
    /* Look toward target */
    sp->forward[0] = target_pos[0] - sp->eye[0];
    sp->forward[1] = target_pos[1] - sp->eye[1];
    sp->forward[2] = target_pos[2] - sp->eye[2];
    norm3(sp->forward);
    return 1;
}

void aether_spectator_get_eye(const aether_spectator_t *sp, f32 out[3]) {
    if (!out) return;
    if (!sp) { out[0]=out[1]=out[2]=0; return; }
    out[0]=sp->eye[0]; out[1]=sp->eye[1]; out[2]=sp->eye[2];
}
void aether_spectator_get_forward(const aether_spectator_t *sp, f32 out[3]) {
    if (!out) return;
    if (!sp) { out[0]=1; out[1]=out[2]=0; return; }
    out[0]=sp->forward[0]; out[1]=sp->forward[1]; out[2]=sp->forward[2];
}
bool aether_spectator_is_following(const aether_spectator_t *sp) {
    return sp && sp->enabled && sp->following;
}
