#include "AetherNetSpectator.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void aether_spectator_init(aether_spectator_t *sp) {
    if (!sp) return;
    memset(sp, 0, sizeof(*sp));
    sp->follow_distance = 96.f;
    sp->follow_height = 24.f;
    sp->smooth = 0.25f;
    sp->forward[0] = 1.f;
    sp->cam_mode = AETHER_SPEC_CAM_FOLLOW;
    sp->roster_index = -1;
    sp->target_hp = -1;
    sp->target_hp_valid = false;
    sp->target_name[0] = '\0';
}

void aether_spectator_set_enabled(aether_spectator_t *sp, bool enabled) {
    if (sp) sp->enabled = enabled;
}

static void refresh_hud(aether_spectator_t *sp) {
    if (!sp) return;
    sp->hud_label[0] = '\0';
    sp->hud_visible = false;
    if (!sp->enabled || !sp->following) return;
    const char *name = NULL;
    if (sp->target_name[0]) name = sp->target_name;
    if (!name) {
        for (u32 i = 0; i < sp->roster_count; ++i) {
            if (sp->roster[i].active && sp->roster[i].player_id == sp->target_player_id) {
                name = sp->roster[i].name;
                break;
            }
        }
    }
    char base[48];
    if (name && name[0]) {
        snprintf(base, sizeof base, "SPEC: %s", name);
    } else {
        snprintf(base, sizeof base, "SPEC: #%u", sp->target_player_id);
    }
    if (sp->target_hp_valid && sp->target_hp >= 0) {
        snprintf(sp->hud_label, sizeof sp->hud_label, "%s [%d]", base, sp->target_hp);
    } else {
        snprintf(sp->hud_label, sizeof sp->hud_label, "%s", base);
    }
    sp->hud_visible = true;
}

void aether_spectator_refresh_hud(aether_spectator_t *sp) { refresh_hud(sp); }

void aether_spectator_follow(aether_spectator_t *sp, u32 player_id) {
    if (!sp) return;
    sp->enabled = true;
    sp->target_player_id = player_id;
    sp->following = true;
    /* Sync roster_index if player is in roster */
    sp->roster_index = -1;
    for (u32 i = 0; i < sp->roster_count; ++i) {
        if (sp->roster[i].active && sp->roster[i].player_id == player_id) {
            sp->roster_index = (i32)i;
            break;
        }
    }
    refresh_hud(sp);
}

void aether_spectator_stop(aether_spectator_t *sp) {
    if (!sp) return;
    sp->following = false;
    sp->target_player_id = 0;
    sp->roster_index = -1;
    sp->hud_label[0] = '\0';
    sp->hud_visible = false;
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

    f32 desired[3];
    if (sp->cam_mode == AETHER_SPEC_CAM_COPY_EYE) {
        /* Spec camera copies target eye (optional height bias for standing eye). */
        desired[0] = target_pos[0];
        desired[1] = target_pos[1];
        desired[2] = target_pos[2] + sp->follow_height * 0.25f;
        sp->forward[0] = fwd[0];
        sp->forward[1] = fwd[1];
        sp->forward[2] = fwd[2];
    } else {
        desired[0] = target_pos[0] - fwd[0] * sp->follow_distance;
        desired[1] = target_pos[1] - fwd[1] * sp->follow_distance;
        desired[2] = target_pos[2] + sp->follow_height;
    }

    f32 a = sp->smooth;
    if (a < 0.f) a = 0.f;
    if (a > 1.f) a = 1.f;
    /* Instant snap when copy_eye + smooth>=1, else lerp */
    if (sp->cam_mode == AETHER_SPEC_CAM_COPY_EYE && a >= 0.99f) {
        sp->eye[0] = desired[0]; sp->eye[1] = desired[1]; sp->eye[2] = desired[2];
    } else {
        sp->eye[0] += (desired[0] - sp->eye[0]) * a;
        sp->eye[1] += (desired[1] - sp->eye[1]) * a;
        sp->eye[2] += (desired[2] - sp->eye[2]) * a;
    }
    if (sp->cam_mode != AETHER_SPEC_CAM_COPY_EYE) {
        sp->forward[0] = target_pos[0] - sp->eye[0];
        sp->forward[1] = target_pos[1] - sp->eye[1];
        sp->forward[2] = target_pos[2] - sp->eye[2];
        norm3(sp->forward);
    }
    refresh_hud(sp);
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

void aether_spectator_set_cam_mode(aether_spectator_t *sp, aether_spectator_cam_mode_t mode) {
    if (sp) sp->cam_mode = mode;
}
aether_spectator_cam_mode_t aether_spectator_get_cam_mode(const aether_spectator_t *sp) {
    return sp ? sp->cam_mode : AETHER_SPEC_CAM_FOLLOW;
}

void aether_spectator_roster_clear(aether_spectator_t *sp) {
    if (!sp) return;
    memset(sp->roster, 0, sizeof sp->roster);
    sp->roster_count = 0;
    sp->roster_index = -1;
}
int aether_spectator_roster_add(aether_spectator_t *sp, u32 player_id, const char *name) {
    if (!sp || player_id == 0 || sp->roster_count >= AETHER_SPECTATOR_MAX_PLAYERS) return 0;
    for (u32 i = 0; i < sp->roster_count; ++i) {
        if (sp->roster[i].player_id == player_id) {
            sp->roster[i].active = true;
            if (name) {
                strncpy(sp->roster[i].name, name, sizeof sp->roster[i].name - 1);
                sp->roster[i].name[sizeof sp->roster[i].name - 1] = '\0';
            }
            return 1;
        }
    }
    aether_spectator_player_t *p = &sp->roster[sp->roster_count++];
    memset(p, 0, sizeof(*p));
    p->player_id = player_id;
    p->active = true;
    if (name) {
        strncpy(p->name, name, sizeof p->name - 1);
        p->name[sizeof p->name - 1] = '\0';
    }
    return 1;
}
u32 aether_spectator_roster_count(const aether_spectator_t *sp) {
    return sp ? sp->roster_count : 0;
}

static u32 cycle_dir(aether_spectator_t *sp, int dir) {
    if (!sp || sp->roster_count == 0) return 0;
    i32 start = sp->roster_index;
    i32 n = (i32)sp->roster_count;
    for (i32 step = 0; step < n; ++step) {
        i32 idx;
        if (start < 0) idx = (dir > 0) ? 0 : (n - 1);
        else idx = (start + dir * (step + 1) + n * 8) % n;
        if (!sp->roster[idx].active) continue;
        aether_spectator_follow(sp, sp->roster[idx].player_id);
        sp->roster_index = idx;
        return sp->target_player_id;
    }
    return 0;
}
u32 aether_spectator_cycle_next(aether_spectator_t *sp) { return cycle_dir(sp, +1); }
u32 aether_spectator_cycle_prev(aether_spectator_t *sp) { return cycle_dir(sp, -1); }
u32 aether_spectator_target_id(const aether_spectator_t *sp) {
    return (sp && sp->following) ? sp->target_player_id : 0;
}

u32 aether_spectator_hud_indicator(const aether_spectator_t *sp, char *out, u32 cap) {
    if (!out || cap == 0) return 0;
    out[0] = '\0';
    if (!sp || !sp->hud_visible || !sp->hud_label[0]) return 0;
    strncpy(out, sp->hud_label, cap - 1);
    out[cap - 1] = '\0';
    return (u32)strlen(out);
}
bool aether_spectator_hud_visible(const aether_spectator_t *sp) {
    return sp && sp->hud_visible && sp->hud_label[0] != '\0';
}

void aether_spectator_set_target_hp(aether_spectator_t *sp, i32 hp) {
    if (!sp) return;
    sp->target_hp = hp;
    sp->target_hp_valid = (hp >= 0);
    refresh_hud(sp);
}
void aether_spectator_set_target_name(aether_spectator_t *sp, const char *name) {
    if (!sp) return;
    sp->target_name[0] = '\0';
    if (name && name[0]) {
        strncpy(sp->target_name, name, sizeof sp->target_name - 1);
        sp->target_name[sizeof sp->target_name - 1] = '\0';
    }
    refresh_hud(sp);
}
i32 aether_spectator_get_target_hp(const aether_spectator_t *sp) {
    if (!sp || !sp->target_hp_valid) return -1;
    return sp->target_hp;
}
u32 aether_spectator_get_target_name(const aether_spectator_t *sp, char *out, u32 cap) {
    if (!out || cap == 0) return 0;
    out[0] = '\0';
    if (!sp) return 0;
    const char *name = sp->target_name[0] ? sp->target_name : NULL;
    if (!name) {
        for (u32 i = 0; i < sp->roster_count; ++i) {
            if (sp->roster[i].active && sp->roster[i].player_id == sp->target_player_id) {
                name = sp->roster[i].name;
                break;
            }
        }
    }
    if (!name || !name[0]) return 0;
    strncpy(out, name, cap - 1);
    out[cap - 1] = '\0';
    return (u32)strlen(out);
}
