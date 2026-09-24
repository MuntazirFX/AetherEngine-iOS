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


static void mat_translate(f32 m[16], f32 x, f32 y, f32 z) {
    mat_identity(m);
    m[12] = x; m[13] = y; m[14] = z;
}

static void mat_mul(f32 out[16], const f32 a[16], const f32 b[16]) {
    /* Column-major: out = a * b (apply b then a). */
    f32 r[16];
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            r[col*4+row] =
                a[0*4+row]*b[col*4+0] +
                a[1*4+row]*b[col*4+1] +
                a[2*4+row]*b[col*4+2] +
                a[3*4+row]*b[col*4+3];
        }
    }
    memcpy(out, r, sizeof r);
}

void aether_mdl_sequence_init_sway(aether_mdl_sequence_t *seq, u32 bone_count, u32 frames, f32 fps) {
    if (!seq) return;
    memset(seq, 0, sizeof(*seq));
    if (bone_count == 0) bone_count = 2;
    if (bone_count > AETHER_MDL_MAX_BONES) bone_count = AETHER_MDL_MAX_BONES;
    if (frames < 2) frames = 2;
    if (frames > AETHER_MDL_MAX_SEQ_FRAMES) frames = AETHER_MDL_MAX_SEQ_FRAMES;
    seq->bone_count = bone_count;
    seq->frame_count = frames;
    seq->fps = fps > 0.f ? fps : 10.f;
    seq->loop = true;
    for (u32 f = 0; f < frames; ++f) {
        f32 t = (f32)f / (f32)(frames - 1);
        f32 swing = sinf(t * AETHER_PI * 2.f) * 25.f;
        for (u32 b = 0; b < bone_count; ++b) {
            aether_mdl_seq_bone_key_t *k = &seq->keys[f][b];
            k->pos[0] = 0.f;
            k->pos[1] = 0.f;
            k->pos[2] = (b == 0) ? 0.f : (8.f + 2.f * sinf(t * AETHER_PI * 2.f));
            k->angles_deg[0] = 0.f;
            k->angles_deg[1] = (b == 0) ? swing : (-swing * 0.5f);
            k->angles_deg[2] = 0.f;
        }
    }
}

void aether_mdl_skin_build_from_sequence(aether_mdl_skin_state_t *sk,
                                         const aether_mdl_sequence_t *seq,
                                         f32 frame) {
    if (!sk || !seq || seq->frame_count == 0 || seq->bone_count == 0) {
        if (sk) aether_mdl_skin_identity(sk, seq ? seq->bone_count : 0);
        return;
    }
    u32 nframes = seq->frame_count;
    u32 nbones = seq->bone_count;
    if (nbones > AETHER_MDL_MAX_BONES) nbones = AETHER_MDL_MAX_BONES;
    aether_mdl_skin_identity(sk, nbones);
    sk->time = frame / (seq->fps > 0.f ? seq->fps : 10.f);

    f32 f = frame;
    if (seq->loop) {
        while (f < 0.f) f += (f32)nframes;
        f = fmodf(f, (f32)nframes);
    } else {
        if (f < 0.f) f = 0.f;
        if (f > (f32)(nframes - 1)) f = (f32)(nframes - 1);
    }
    u32 i0 = (u32)f;
    u32 i1 = i0 + 1;
    if (i1 >= nframes) i1 = seq->loop ? 0u : (nframes - 1);
    f32 frac = f - (f32)i0;

    for (u32 b = 0; b < nbones; ++b) {
        const aether_mdl_seq_bone_key_t *a = &seq->keys[i0][b];
        const aether_mdl_seq_bone_key_t *c = &seq->keys[i1][b];
        f32 yaw = (a->angles_deg[1] * (1.f - frac) + c->angles_deg[1] * frac) * (AETHER_PI / 180.f);
        f32 px = a->pos[0] * (1.f - frac) + c->pos[0] * frac;
        f32 py = a->pos[1] * (1.f - frac) + c->pos[1] * frac;
        f32 pz = a->pos[2] * (1.f - frac) + c->pos[2] * frac;
        f32 R[16], T[16];
        mat_rotate_z(R, yaw);
        mat_translate(T, px, py, pz);
        mat_mul(sk->bones[b].m, T, R);
    }
}

void aether_mdl_skin_transform_point2(const aether_mdl_skin_state_t *sk,
                                      u32 bone0, f32 w0, u32 bone1, f32 w1,
                                      const f32 in[3], f32 out[3]) {
    if (!out) return;
    f32 a[3] = {0,0,0}, b[3] = {0,0,0};
    f32 sum = w0 + w1;
    if (sum < 1e-6f) { if (in) { out[0]=in[0]; out[1]=in[1]; out[2]=in[2]; } return; }
    aether_mdl_skin_transform_point(sk, bone0, 1.f, in, a);
    aether_mdl_skin_transform_point(sk, bone1, 1.f, in, b);
    f32 inv = 1.f / sum;
    out[0] = (a[0]*w0 + b[0]*w1) * inv;
    out[1] = (a[1]*w0 + b[1]*w1) * inv;
    out[2] = (a[2]*w0 + b[2]*w1) * inv;
}

u32 aether_mdl_skin_mesh(const aether_mdl_skin_state_t *sk,
                         const u8 *bone_indices, const f32 *weights,
                         const f32 *in_xyz, f32 *out_xyz, u32 vert_count) {
    if (!sk || !in_xyz || !out_xyz || vert_count == 0) return 0;
    for (u32 i = 0; i < vert_count; ++i) {
        u32 bone = bone_indices ? bone_indices[i] : 0;
        f32 w = weights ? weights[i] : 1.f;
        aether_mdl_skin_transform_point(sk, bone, w, in_xyz + i*3, out_xyz + i*3);
    }
    return vert_count;
}
