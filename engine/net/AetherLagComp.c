#include "AetherLagComp.h"
#include <math.h>
#include <string.h>

void aether_lagcomp_init(aether_lagcomp_history_t *h) {
    if (h) memset(h, 0, sizeof(*h));
}
void aether_lagcomp_clear(aether_lagcomp_history_t *h) {
    aether_lagcomp_init(h);
}

void aether_lagcomp_begin_frame(aether_lagcomp_history_t *h, f32 time) {
    if (!h) return;
    aether_lagcomp_frame_t *f = &h->frames[h->head % AETHER_LAGCOMP_MAX_FRAMES];
    memset(f, 0, sizeof(*f));
    f->time = time;
    h->head = (h->head + 1) % AETHER_LAGCOMP_MAX_FRAMES;
    if (h->count < AETHER_LAGCOMP_MAX_FRAMES) h->count++;
}

bool aether_lagcomp_push_aabb(aether_lagcomp_history_t *h, i32 id, u8 kind,
                              const f32 mins[3], const f32 maxs[3]) {
    if (!h || !mins || !maxs || h->count == 0) return false;
    u32 idx = (h->head + AETHER_LAGCOMP_MAX_FRAMES - 1) % AETHER_LAGCOMP_MAX_FRAMES;
    aether_lagcomp_frame_t *f = &h->frames[idx];
    if (f->count >= AETHER_LAGCOMP_MAX_ENTS) return false;
    aether_lagcomp_aabb_t *a = &f->items[f->count++];
    a->id = id;
    a->kind = kind;
    memcpy(a->mins, mins, sizeof a->mins);
    memcpy(a->maxs, maxs, sizeof a->maxs);
    return true;
}

static const aether_lagcomp_frame_t *pick_frame(const aether_lagcomp_history_t *h, f32 time) {
    if (!h || h->count == 0) return NULL;
    const aether_lagcomp_frame_t *best = NULL;
    f32 best_dt = 1e30f;
    for (u32 i = 0; i < h->count; ++i) {
        u32 idx = (h->head + AETHER_LAGCOMP_MAX_FRAMES - 1 - i) % AETHER_LAGCOMP_MAX_FRAMES;
        const aether_lagcomp_frame_t *f = &h->frames[idx];
        f32 dt = fabsf(f->time - time);
        if (dt < best_dt) { best_dt = dt; best = f; }
    }
    return best;
}

bool aether_lagcomp_query(const aether_lagcomp_history_t *h, f32 time, i32 id,
                          aether_lagcomp_aabb_t *out) {
    const aether_lagcomp_frame_t *f = pick_frame(h, time);
    if (!f || !out) return false;
    for (u32 i = 0; i < f->count; ++i) {
        if (f->items[i].id == id) {
            *out = f->items[i];
            return true;
        }
    }
    return false;
}

static bool ray_aabb(const f32 origin[3], const f32 dir[3], f32 max_dist,
                     const f32 mins[3], const f32 maxs[3], f32 *out_t, f32 out_pt[3]) {
    f32 tmin = 0.f, tmax = max_dist;
    for (int a = 0; a < 3; ++a) {
        f32 o = origin[a], d = dir[a];
        f32 mn = mins[a], mx = maxs[a];
        if (fabsf(d) < 1e-8f) {
            if (o < mn || o > mx) return false;
            continue;
        }
        f32 inv = 1.f / d;
        f32 t0 = (mn - o) * inv;
        f32 t1 = (mx - o) * inv;
        if (t0 > t1) { f32 tmp = t0; t0 = t1; t1 = tmp; }
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmin > tmax) return false;
    }
    if (tmin < 0.f) tmin = 0.f;
    if (out_t) *out_t = tmin;
    if (out_pt) {
        out_pt[0] = origin[0] + dir[0] * tmin;
        out_pt[1] = origin[1] + dir[1] * tmin;
        out_pt[2] = origin[2] + dir[2] * tmin;
    }
    return true;
}

bool aether_lagcomp_trace(const aether_lagcomp_history_t *h, f32 time,
                          const f32 origin[3], const f32 dir[3], f32 max_dist,
                          i32 *out_id, f32 *out_t, f32 out_point[3]) {
    const aether_lagcomp_frame_t *f = pick_frame(h, time);
    if (!f || !origin || !dir) return false;
    f32 best_t = max_dist + 1.f;
    i32 best_id = -1;
    f32 best_pt[3] = {0,0,0};
    for (u32 i = 0; i < f->count; ++i) {
        f32 t, pt[3];
        if (!ray_aabb(origin, dir, max_dist, f->items[i].mins, f->items[i].maxs, &t, pt))
            continue;
        if (t < best_t) {
            best_t = t;
            best_id = f->items[i].id;
            best_pt[0]=pt[0]; best_pt[1]=pt[1]; best_pt[2]=pt[2];
        }
    }
    if (best_id < 0) return false;
    if (out_id) *out_id = best_id;
    if (out_t) *out_t = best_t;
    if (out_point) { out_point[0]=best_pt[0]; out_point[1]=best_pt[1]; out_point[2]=best_pt[2]; }
    return true;
}

u32 aether_lagcomp_frame_count(const aether_lagcomp_history_t *h) {
    return h ? h->count : 0;
}

#include "AetherNetCmd.h"

void aether_lagcomp_look_dir(f32 yaw_deg, f32 pitch_deg, f32 out_dir[3]) {
    if (!out_dir) return;
    const f32 deg2rad = 0.01745329252f;
    f32 yaw = yaw_deg * deg2rad;
    f32 pitch = pitch_deg * deg2rad;
    f32 cp = cosf(pitch), sp = sinf(pitch);
    f32 cy = cosf(yaw), sy = sinf(yaw);
    /* GoldSrc: X=forward, Y=right, Z=up → forward = (cp*cy, cp*sy, -sp) */
    out_dir[0] = cp * cy;
    out_dir[1] = cp * sy;
    out_dir[2] = -sp;
}

bool aether_lagcomp_validate_hit(const aether_lagcomp_history_t *h,
                                 const struct aether_net_cmd_history *cmds,
                                 f32 now, f32 lag_ms,
                                 const f32 eye_origin[3], f32 max_dist,
                                 aether_lagcomp_hit_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!h || !cmds || !eye_origin || max_dist <= 0.f) return false;
    const aether_net_cmd_t *cmd = aether_net_cmd_history_at_lag(cmds, now, lag_ms);
    if (!cmd) return false;
    /* Only validate when attack button set (bit 1). */
    if ((cmd->buttons & 1u) == 0) return false;
    f32 rewind = now - (lag_ms * 0.001f);
    if (rewind < 0.f) rewind = 0.f;
    f32 dir[3];
    aether_lagcomp_look_dir(cmd->yaw_deg, cmd->pitch_deg, dir);
    i32 id = -1; f32 t = 0; f32 pt[3] = {0,0,0};
    if (!aether_lagcomp_trace(h, rewind, eye_origin, dir, max_dist, &id, &t, pt))
        return false;
    if (out) {
        out->id = id;
        out->t = t;
        out->point[0]=pt[0]; out->point[1]=pt[1]; out->point[2]=pt[2];
        out->rewind_time = rewind;
        out->cmd_seq = cmd->seq;
        out->valid = true;
    }
    return true;
}

/* ---------- Bone-hitbox lag rewind ---------- */

void aether_lagcomp_studio_init(aether_lagcomp_studio_history_t *h) {
    if (h) memset(h, 0, sizeof(*h));
}
void aether_lagcomp_studio_clear(aether_lagcomp_studio_history_t *h) {
    aether_lagcomp_studio_init(h);
}

void aether_lagcomp_studio_begin_frame(aether_lagcomp_studio_history_t *h, f32 time) {
    if (!h) return;
    aether_lagcomp_studio_frame_t *f = &h->frames[h->head % AETHER_LAGCOMP_MAX_FRAMES];
    memset(f, 0, sizeof(*f));
    f->time = time;
    h->head = (h->head + 1) % AETHER_LAGCOMP_MAX_FRAMES;
    if (h->count < AETHER_LAGCOMP_MAX_FRAMES) h->count++;
}

bool aether_lagcomp_studio_push(aether_lagcomp_studio_history_t *h, i32 id, u8 kind,
                                const f32 *bone_mats, u32 bone_count,
                                const aether_lagcomp_hitbox_t *boxes, u32 hitbox_count) {
    if (!h || !bone_mats || !boxes || h->count == 0) return false;
    if (bone_count == 0 || hitbox_count == 0) return false;
    u32 idx = (h->head + AETHER_LAGCOMP_MAX_FRAMES - 1) % AETHER_LAGCOMP_MAX_FRAMES;
    aether_lagcomp_studio_frame_t *f = &h->frames[idx];
    if (f->count >= AETHER_LAGCOMP_MAX_ENTS) return false;
    aether_lagcomp_studio_t *s = &f->items[f->count++];
    memset(s, 0, sizeof(*s));
    s->id = id;
    s->kind = kind;
    s->bone_count = (u8)((bone_count > AETHER_LAGCOMP_MAX_BONES) ? AETHER_LAGCOMP_MAX_BONES : bone_count);
    s->hitbox_count = (u8)((hitbox_count > AETHER_LAGCOMP_MAX_HITBOXES) ? AETHER_LAGCOMP_MAX_HITBOXES : hitbox_count);
    memcpy(s->bone_mats, bone_mats, (size_t)s->bone_count * 16u * sizeof(f32));
    memcpy(s->boxes, boxes, (size_t)s->hitbox_count * sizeof(aether_lagcomp_hitbox_t));
    return true;
}

static const aether_lagcomp_studio_frame_t *pick_studio_frame(
    const aether_lagcomp_studio_history_t *h, f32 time) {
    if (!h || h->count == 0) return NULL;
    const aether_lagcomp_studio_frame_t *best = NULL;
    f32 best_dt = 1e30f;
    for (u32 i = 0; i < h->count; ++i) {
        u32 idx = (h->head + AETHER_LAGCOMP_MAX_FRAMES - 1 - i) % AETHER_LAGCOMP_MAX_FRAMES;
        const aether_lagcomp_studio_frame_t *f = &h->frames[idx];
        f32 dt = fabsf(f->time - time);
        if (dt < best_dt) { best_dt = dt; best = f; }
    }
    return best;
}

bool aether_lagcomp_studio_query(const aether_lagcomp_studio_history_t *h, f32 time, i32 id,
                                 aether_lagcomp_studio_t *out) {
    const aether_lagcomp_studio_frame_t *f = pick_studio_frame(h, time);
    if (!f || !out) return false;
    for (u32 i = 0; i < f->count; ++i) {
        if (f->items[i].id == id) {
            *out = f->items[i];
            return true;
        }
    }
    return false;
}

void aether_lagcomp_hitbox_to_world(const aether_lagcomp_hitbox_t *box,
                                    const f32 *bone_mat16,
                                    f32 out_mins[3], f32 out_maxs[3]) {
    if (!box || !out_mins || !out_maxs) return;
    /* Transform 8 corners of local AABB by bone mat, then re-AABB. */
    f32 corners[8][3];
    int n = 0;
    for (int xi = 0; xi < 2; ++xi)
    for (int yi = 0; yi < 2; ++yi)
    for (int zi = 0; zi < 2; ++zi) {
        corners[n][0] = xi ? box->maxs[0] : box->mins[0];
        corners[n][1] = yi ? box->maxs[1] : box->mins[1];
        corners[n][2] = zi ? box->maxs[2] : box->mins[2];
        n++;
    }
    f32 mn[3] = {1e30f, 1e30f, 1e30f};
    f32 mx[3] = {-1e30f, -1e30f, -1e30f};
    for (int i = 0; i < 8; ++i) {
        f32 x = corners[i][0], y = corners[i][1], z = corners[i][2];
        f32 wx, wy, wz;
        if (bone_mat16) {
            wx = bone_mat16[0]*x + bone_mat16[4]*y + bone_mat16[8]*z  + bone_mat16[12];
            wy = bone_mat16[1]*x + bone_mat16[5]*y + bone_mat16[9]*z  + bone_mat16[13];
            wz = bone_mat16[2]*x + bone_mat16[6]*y + bone_mat16[10]*z + bone_mat16[14];
        } else {
            wx = x; wy = y; wz = z;
        }
        if (wx < mn[0]) mn[0] = wx;
        if (wy < mn[1]) mn[1] = wy;
        if (wz < mn[2]) mn[2] = wz;
        if (wx > mx[0]) mx[0] = wx;
        if (wy > mx[1]) mx[1] = wy;
        if (wz > mx[2]) mx[2] = wz;
    }
    out_mins[0]=mn[0]; out_mins[1]=mn[1]; out_mins[2]=mn[2];
    out_maxs[0]=mx[0]; out_maxs[1]=mx[1]; out_maxs[2]=mx[2];
}

bool aether_lagcomp_studio_trace(const aether_lagcomp_studio_history_t *h, f32 time,
                                 const f32 origin[3], const f32 dir[3], f32 max_dist,
                                 i32 *out_id, i32 *out_hitbox, f32 *out_t, f32 out_point[3]) {
    const aether_lagcomp_studio_frame_t *f = pick_studio_frame(h, time);
    if (!f || !origin || !dir) return false;
    f32 best_t = max_dist + 1.f;
    i32 best_id = -1, best_hb = -1;
    f32 best_pt[3] = {0,0,0};
    for (u32 ei = 0; ei < f->count; ++ei) {
        const aether_lagcomp_studio_t *s = &f->items[ei];
        for (u32 hi = 0; hi < s->hitbox_count; ++hi) {
            const aether_lagcomp_hitbox_t *box = &s->boxes[hi];
            const f32 *mat = NULL;
            if (box->bone >= 0 && (u32)box->bone < s->bone_count)
                mat = s->bone_mats + (u32)box->bone * 16u;
            f32 wmins[3], wmaxs[3];
            aether_lagcomp_hitbox_to_world(box, mat, wmins, wmaxs);
            f32 t, pt[3];
            if (!ray_aabb(origin, dir, max_dist, wmins, wmaxs, &t, pt))
                continue;
            if (t < best_t) {
                best_t = t;
                best_id = s->id;
                best_hb = (i32)hi;
                best_pt[0]=pt[0]; best_pt[1]=pt[1]; best_pt[2]=pt[2];
            }
        }
    }
    if (best_id < 0) return false;
    if (out_id) *out_id = best_id;
    if (out_hitbox) *out_hitbox = best_hb;
    if (out_t) *out_t = best_t;
    if (out_point) { out_point[0]=best_pt[0]; out_point[1]=best_pt[1]; out_point[2]=best_pt[2]; }
    return true;
}

u32 aether_lagcomp_studio_frame_count(const aether_lagcomp_studio_history_t *h) {
    return h ? h->count : 0;
}
