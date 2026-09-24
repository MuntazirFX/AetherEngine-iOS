#include "AetherInteract.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool aether_interact_ray_aabb(const f32 origin[3], const f32 dir[3], f32 max_dist,
                              const f32 mins[3], const f32 maxs[3],
                              f32 *out_t, f32 out_point[3]) {
    if (!origin || !dir || !mins || !maxs || max_dist <= 0.f) return false;
    f32 tmin = 0.f, tmax = max_dist;
    for (int i = 0; i < 3; ++i) {
        if (fabsf(dir[i]) < 1e-8f) {
            if (origin[i] < mins[i] || origin[i] > maxs[i]) return false;
            continue;
        }
        f32 inv = 1.f / dir[i];
        f32 t1 = (mins[i] - origin[i]) * inv;
        f32 t2 = (maxs[i] - origin[i]) * inv;
        if (t1 > t2) { f32 tmp = t1; t1 = t2; t2 = tmp; }
        if (t1 > tmin) tmin = t1;
        if (t2 < tmax) tmax = t2;
        if (tmin > tmax) return false;
    }
    if (out_t) *out_t = tmin;
    if (out_point) {
        out_point[0] = origin[0] + dir[0] * tmin;
        out_point[1] = origin[1] + dir[1] * tmin;
        out_point[2] = origin[2] + dir[2] * tmin;
    }
    return true;
}

void aether_interact_forward_from_view(f32 yaw_deg, f32 pitch_deg, f32 out_fwd[3]) {
    if (!out_fwd) return;
    f32 yaw = yaw_deg * (f32)M_PI / 180.f;
    f32 pitch = pitch_deg * (f32)M_PI / 180.f;
    f32 cp = cosf(pitch), sp = sinf(pitch);
    f32 cy = cosf(yaw), sy = sinf(yaw);
    /* Z-up: forward = (cp*cy, cp*sy, -sp) in many GoldSrc-ish conventions;
     * use (cp*cos, cp*sin, sp) with pitch-up positive for interact smoke. */
    out_fwd[0] = cp * cy;
    out_fwd[1] = cp * sy;
    out_fwd[2] = sp;
    f32 len = sqrtf(out_fwd[0]*out_fwd[0]+out_fwd[1]*out_fwd[1]+out_fwd[2]*out_fwd[2]);
    if (len > 1e-6f) { out_fwd[0]/=len; out_fwd[1]/=len; out_fwd[2]/=len; }
}

void aether_interact_trace(const f32 eye[3], const f32 forward[3], f32 max_dist,
                           f32 ground_z,
                           const aether_interact_target_t *targets, u32 target_count,
                           aether_interact_hit_t *out_hit) {
    if (!out_hit) return;
    memset(out_hit, 0, sizeof(*out_hit));
    out_hit->kind = AETHER_INTERACT_NONE;
    out_hit->entity_id = -1;
    out_hit->distance = max_dist;
    if (!eye || !forward || max_dist <= 0.f) return;

    f32 best_t = max_dist;
    aether_interact_hit_t best;
    memset(&best, 0, sizeof best);
    best.kind = AETHER_INTERACT_NONE;
    best.entity_id = -1;
    best.distance = max_dist;

    /* Ground plane z = ground_z */
    if (fabsf(forward[2]) > 1e-6f) {
        f32 t = (ground_z - eye[2]) / forward[2];
        if (t > 0.f && t < best_t) {
            best_t = t;
            best.kind = AETHER_INTERACT_WORLD;
            best.point[0] = eye[0] + forward[0] * t;
            best.point[1] = eye[1] + forward[1] * t;
            best.point[2] = ground_z;
            best.normal[0] = 0; best.normal[1] = 0; best.normal[2] = 1;
            best.distance = t;
            best.entity_id = -1;
            aether_str_copy(best.classname, sizeof best.classname, "worldspawn");
        }
    }

    if (targets) {
        for (u32 i = 0; i < target_count; ++i) {
            const aether_interact_target_t *tg = &targets[i];
            if (!tg->usable) continue;
            f32 t = 0.f, pt[3];
            if (!aether_interact_ray_aabb(eye, forward, max_dist, tg->mins, tg->maxs, &t, pt))
                continue;
            if (t < best_t) {
                best_t = t;
                best.kind = AETHER_INTERACT_ENTITY;
                best.point[0] = pt[0]; best.point[1] = pt[1]; best.point[2] = pt[2];
                best.normal[0] = 0; best.normal[1] = -1; best.normal[2] = 0;
                best.distance = t;
                best.entity_id = tg->id;
                aether_str_copy(best.classname, sizeof best.classname, tg->classname);
            }
        }
    }
    *out_hit = best;
}
