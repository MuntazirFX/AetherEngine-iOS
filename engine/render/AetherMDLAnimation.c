#include "AetherMDLAnimation.h"
#include <math.h>
#include <string.h>

#ifndef AETHER_PI
#define AETHER_PI 3.14159265358979323846f
#endif

static void mat_identity(f32 m[16]) {
    memset(m, 0, sizeof(f32) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void mat_rotate_z(f32 m[16], f32 rad) {
    mat_identity(m);
    f32 c = cosf(rad), s = sinf(rad);
    m[0] = c; m[1] = s;
    m[4] = -s; m[5] = c;
}

aether_result_t aether_mdl_animation_init(aether_mdl_animation_t *a, u32 bones) {
    if (!a) return AETHER_ERR_INVALID_ARG;
    memset(a, 0, sizeof(*a));
    a->bone_count = bones;
    a->fps = 30.0f;
    return AETHER_OK;
}
void aether_mdl_animation_play(aether_mdl_animation_t *a, u32 seq, f32 fps, bool loop) {
    if (!a) return;
    a->sequence = seq;
    a->fps = fps > 0 ? fps : 30.0f;
    a->loop = loop;
    a->time = 0;
    a->frame = 0;
    a->playing = true;
}
void aether_mdl_animation_update(aether_mdl_animation_t *a, f32 dt, u32 frames) {
    if (!a || !a->playing || !frames) return;
    a->time += dt;
    a->frame = a->time * a->fps;
    if (a->frame >= (f32)frames) {
        if (a->loop) {
            a->frame = (f32)((u32)a->frame % frames);
            a->time = a->frame / a->fps;
        } else {
            a->frame = (f32)(frames - 1);
            a->playing = false;
        }
    }
}
void aether_mdl_animation_stop(aether_mdl_animation_t *a) {
    if (a) a->playing = false;
}

void aether_mdl_skin_identity(aether_mdl_skin_state_t *sk, u32 bone_count) {
    if (!sk) return;
    memset(sk, 0, sizeof(*sk));
    if (bone_count > AETHER_MDL_MAX_BONES) bone_count = AETHER_MDL_MAX_BONES;
    sk->bone_count = bone_count;
    for (u32 i = 0; i < bone_count; ++i) mat_identity(sk->bones[i].m);
}

void aether_mdl_skin_build_stub(aether_mdl_skin_state_t *sk, u32 bone_count, f32 time,
                                f32 sway_deg) {
    aether_mdl_skin_identity(sk, bone_count);
    if (!sk || bone_count == 0) return;
    sk->time = time;
    f32 rad = sway_deg * (AETHER_PI / 180.f) * sinf(time * 2.f);
    mat_rotate_z(sk->bones[0].m, rad);
    if (bone_count > 1) {
        mat_rotate_z(sk->bones[1].m, -rad * 0.5f);
        sk->bones[1].m[14] = 8.f + 2.f * sinf(time * 3.f);
    }
}

u32 aether_mdl_skin_fill_ubo(const aether_mdl_skin_state_t *sk, f32 *out, u32 max_floats) {
    if (!sk || !out || sk->bone_count == 0) return 0;
    u32 need = sk->bone_count * 16u;
    if (need > max_floats) return 0;
    for (u32 i = 0; i < sk->bone_count; ++i)
        memcpy(out + i * 16u, sk->bones[i].m, sizeof(f32) * 16u);
    return need;
}

void aether_mdl_skin_transform_point(const aether_mdl_skin_state_t *sk, u32 bone,
                                     f32 weight, const f32 in[3], f32 out[3]) {
    if (!out) return;
    out[0] = out[1] = out[2] = 0.f;
    if (!sk || !in || bone >= sk->bone_count) {
        if (in) { out[0]=in[0]; out[1]=in[1]; out[2]=in[2]; }
        return;
    }
    if (weight < 0.f) weight = 0.f;
    if (weight > 1.f) weight = 1.f;
    const f32 *m = sk->bones[bone].m;
    f32 x = in[0], y = in[1], z = in[2];
    f32 sx = m[0]*x + m[4]*y + m[8]*z + m[12];
    f32 sy = m[1]*x + m[5]*y + m[9]*z + m[13];
    f32 sz = m[2]*x + m[6]*y + m[10]*z + m[14];
    out[0] = in[0] * (1.f - weight) + sx * weight;
    out[1] = in[1] * (1.f - weight) + sy * weight;
    out[2] = in[2] * (1.f - weight) + sz * weight;
}
