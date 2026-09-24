#include "AetherWater.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Water plane tessellation: segs x segs quads. */
#define AETHER_WATER_SEGS 16

static void set4(f32 dst[4], f32 r, f32 g, f32 b, f32 a) {
    dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
}

aether_result_t aether_water_init(aether_water_t *w) {
    if (!w) return AETHER_ERR_INVALID_ARG;
    memset(w, 0, sizeof(*w));
    w->wave_speed = 1.0f;
    w->wave_amp = 6.0f;
    w->wave_freq = 0.035f;
    w->opacity = 0.72f;
    w->size = 512.0f;
    w->height = 0.0f;
    w->origin[0] = 0.0f;
    w->origin[1] = 0.0f;
    w->enabled = true;
    /* Soft teal — clean-room placeholder, no game assets. */
    set4(w->color, 0.15f, 0.45f, 0.65f, 0.72f);
    return AETHER_OK;
}

void aether_water_shutdown(aether_water_t *w) {
    if (w) memset(w, 0, sizeof(*w));
}

void aether_water_update(aether_water_t *w, f32 dt) {
    if (!w || !w->enabled || dt <= 0.0f) return;
    w->wave_time += dt * w->wave_speed;
}

void aether_water_set_enabled(aether_water_t *w, bool enabled) {
    if (w) w->enabled = enabled;
}

aether_result_t aether_water_set_color(aether_water_t *w, const f32 rgba[4]) {
    if (!w || !rgba) return AETHER_ERR_INVALID_ARG;
    set4(w->color, rgba[0], rgba[1], rgba[2], rgba[3]);
    w->opacity = rgba[3];
    return AETHER_OK;
}

aether_result_t aether_water_set_size(aether_water_t *w, f32 size) {
    if (!w || size <= 0.0f) return AETHER_ERR_INVALID_ARG;
    w->size = size;
    return AETHER_OK;
}

aether_result_t aether_water_set_height(aether_water_t *w, f32 height) {
    if (!w) return AETHER_ERR_INVALID_ARG;
    w->height = height;
    return AETHER_OK;
}

aether_result_t aether_water_set_origin(aether_water_t *w, f32 x, f32 y) {
    if (!w) return AETHER_ERR_INVALID_ARG;
    w->origin[0] = x;
    w->origin[1] = y;
    return AETHER_OK;
}

aether_result_t aether_water_set_wave(aether_water_t *w, f32 speed, f32 amp, f32 freq) {
    if (!w || speed < 0.0f || amp < 0.0f || freq < 0.0f)
        return AETHER_ERR_INVALID_ARG;
    w->wave_speed = speed;
    w->wave_amp = amp;
    w->wave_freq = freq;
    return AETHER_OK;
}

u32 aether_water_render_vertex_count(void) {
    /* Each seg×seg quad → 2 triangles → 6 verts. */
    return (u32)(AETHER_WATER_SEGS * AETHER_WATER_SEGS * 6);
}

static f32 sample_height(const aether_water_t *w, f32 x, f32 y) {
    f32 t = w->wave_time;
    f32 f = w->wave_freq;
    f32 a = w->wave_amp;
    /* Two phase-offset sines — cheap procedurals, clean-room. */
    f32 h = sinf((x + y) * f + t) * a;
    h += sinf((x - y) * f * 1.7f + t * 1.3f) * (a * 0.45f);
    return w->height + h;
}

static void emit_vert(aether_water_vertex_t *v,
                      f32 x, f32 y, f32 z,
                      f32 u, f32 vv,
                      const f32 rgba[4], f32 height_n) {
    /* Tint brighter on wave peaks. */
    f32 boost = 0.08f * height_n;
    v->x = x; v->y = y; v->z = z;
    v->u = u; v->v = vv;
    v->r = rgba[0] + boost;
    v->g = rgba[1] + boost * 0.6f;
    v->b = rgba[2];
    v->a = rgba[3];
    if (v->r > 1.0f) v->r = 1.0f;
    if (v->g > 1.0f) v->g = 1.0f;
}

u32 aether_water_copy_render(const aether_water_t *w,
                             aether_water_vertex_t *out,
                             u32 max_out) {
    if (!w || !out || !w->enabled || max_out == 0) return 0;
    const u32 need = aether_water_render_vertex_count();
    if (max_out < need) return 0;

    const f32 size = (w->size > 0.0f) ? w->size : 512.0f;
    const f32 ox = w->origin[0];
    const f32 oy = w->origin[1];
    f32 rgba[4] = { w->color[0], w->color[1], w->color[2], w->opacity };
    if (rgba[3] < 0.0f) rgba[3] = 0.0f;
    if (rgba[3] > 1.0f) rgba[3] = 1.0f;
    const f32 amp = (w->wave_amp > 1e-4f) ? w->wave_amp : 1.0f;

    u32 written = 0;
    for (int j = 0; j < AETHER_WATER_SEGS; ++j) {
        f32 v0 = (f32)j / (f32)AETHER_WATER_SEGS;
        f32 v1 = (f32)(j + 1) / (f32)AETHER_WATER_SEGS;
        f32 y0 = oy + (v0 * 2.0f - 1.0f) * size;
        f32 y1 = oy + (v1 * 2.0f - 1.0f) * size;
        for (int i = 0; i < AETHER_WATER_SEGS; ++i) {
            f32 u0 = (f32)i / (f32)AETHER_WATER_SEGS;
            f32 u1 = (f32)(i + 1) / (f32)AETHER_WATER_SEGS;
            f32 x0 = ox + (u0 * 2.0f - 1.0f) * size;
            f32 x1 = ox + (u1 * 2.0f - 1.0f) * size;

            f32 z00 = sample_height(w, x0, y0);
            f32 z10 = sample_height(w, x1, y0);
            f32 z01 = sample_height(w, x0, y1);
            f32 z11 = sample_height(w, x1, y1);

            f32 n00 = (z00 - w->height) / amp;
            f32 n10 = (z10 - w->height) / amp;
            f32 n01 = (z01 - w->height) / amp;
            f32 n11 = (z11 - w->height) / amp;

            /* Two triangles (CCW looking down +Z). */
            emit_vert(&out[written++], x0, y0, z00, u0, v0, rgba, n00);
            emit_vert(&out[written++], x1, y0, z10, u1, v0, rgba, n10);
            emit_vert(&out[written++], x0, y1, z01, u0, v1, rgba, n01);

            emit_vert(&out[written++], x1, y0, z10, u1, v0, rgba, n10);
            emit_vert(&out[written++], x1, y1, z11, u1, v1, rgba, n11);
            emit_vert(&out[written++], x0, y1, z01, u0, v1, rgba, n01);
        }
    }
    return written;
}

void aether_water_reflect_point(const aether_water_t *w, const f32 in[3], f32 out[3]) {
    if (!out) return;
    if (!in) { out[0]=out[1]=out[2]=0; return; }
    f32 h = w ? w->height : 0.f;
    /* Reflect across z = h (normal = +Z). */
    out[0] = in[0];
    out[1] = in[1];
    out[2] = 2.f * h - in[2];
}

void aether_water_reflect_compute(const aether_water_t *w, const f32 eye[3],
                                  aether_water_reflect_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->plane_normal[0] = 0.f;
    out->plane_normal[1] = 0.f;
    out->plane_normal[2] = 1.f;
    f32 h = w ? w->height : 0.f;
    out->plane_origin[0] = w ? w->origin[0] : 0.f;
    out->plane_origin[1] = w ? w->origin[1] : 0.f;
    out->plane_origin[2] = h;
    /* Clip plane: 0*x + 0*y + 1*z - h = 0 → keep z >= h for above-water view */
    out->clip_plane[0] = 0.f;
    out->clip_plane[1] = 0.f;
    out->clip_plane[2] = 1.f;
    out->clip_plane[3] = -h;
    /* Mirror matrix: I - 2 n n^T with translation so plane stays fixed.
     * For n=(0,0,1), M = diag(1,1,-1) and translation z' = 2h - z:
     *   [1 0 0 0]
     *   [0 1 0 0]
     *   [0 0 -1 2h]
     *   [0 0 0 1]  (column-major) */
    out->mirror[0] = 1.f; out->mirror[5] = 1.f;
    out->mirror[10] = -1.f; out->mirror[15] = 1.f;
    out->mirror[14] = 2.f * h; /* column 3, row 2 */
    if (eye) {
        aether_water_reflect_point(w, eye, out->eye_reflected);
    }
    out->enabled = w ? w->enabled : false;
}

void aether_water_reflect_fill_uniforms(const aether_water_reflect_t *r,
                                        aether_water_reflect_uniforms_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!r) return;
    memcpy(out->mirror, r->mirror, sizeof out->mirror);
    memcpy(out->clip_plane, r->clip_plane, sizeof out->clip_plane);
    out->enabled = (r->enabled && aether_water_reflect_encode_needed(r)) ? 1.f : 0.f;
}

bool aether_water_reflect_encode_needed(const aether_water_reflect_t *r) {
    if (!r || !r->enabled) return false;
    /* Mirror scale on Z should be -1. */
    return fabsf(r->mirror[10] + 1.f) < 1e-4f;
}

aether_result_t aether_water_reflect_rt_init(aether_water_reflect_rt_t *rt) {
    if (!rt) return AETHER_ERR_INVALID_ARG;
    memset(rt, 0, sizeof(*rt));
    rt->scale = 0.5f;
    rt->sample_enabled = true;
    rt->enabled = true;
    return AETHER_OK;
}

void aether_water_reflect_rt_shutdown(aether_water_reflect_rt_t *rt) {
    if (rt) memset(rt, 0, sizeof(*rt));
}

void aether_water_reflect_rt_set_enabled(aether_water_reflect_rt_t *rt, bool enabled) {
    if (rt) rt->enabled = enabled;
}

aether_result_t aether_water_reflect_rt_ensure(aether_water_reflect_rt_t *rt,
                                               u32 fb_w, u32 fb_h, f32 scale) {
    if (!rt) return AETHER_ERR_INVALID_ARG;
    if (fb_w == 0 || fb_h == 0) return AETHER_ERR_INVALID_ARG;
    if (scale <= 0.05f) scale = 0.5f;
    if (scale > 1.f) scale = 1.f;
    u32 w = (u32)((f32)fb_w * scale);
    u32 h = (u32)((f32)fb_h * scale);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    rt->width = w;
    rt->height = h;
    rt->scale = scale;
    rt->allocated = true;
    /* Non-zero stub id so Metal/host can tell RT is planned (texture made on GPU). */
    rt->tex_stub_id = 0xAE7E0001u ^ (w * 65537u + h);
    if (rt->tex_stub_id == 0) rt->tex_stub_id = 1;
    return AETHER_OK;
}

bool aether_water_reflect_rt_sample_needed(const aether_water_reflect_rt_t *rt) {
    return rt && rt->enabled && rt->allocated && rt->sample_enabled
        && rt->width > 0 && rt->height > 0 && rt->tex_stub_id != 0;
}

bool aether_water_reflect_rt_encode_needed(const aether_water_reflect_rt_t *rt,
                                           const aether_water_reflect_t *reflect) {
    if (!aether_water_reflect_rt_sample_needed(rt)) return false;
    return reflect ? aether_water_reflect_encode_needed(reflect) : true;
}

void aether_water_reflect_rt_encode_plan(const aether_water_reflect_rt_t *rt,
                                         const aether_water_reflect_t *reflect,
                                         aether_water_reflect_rt_plan_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!rt) return;
    out->needed = aether_water_reflect_rt_encode_needed(rt, reflect);
    out->width = rt->width;
    out->height = rt->height;
    out->allocate = rt->allocated && rt->tex_stub_id != 0;
    out->sample = aether_water_reflect_rt_sample_needed(rt);
    out->pass_count = out->needed ? 1u : 0u;
}
