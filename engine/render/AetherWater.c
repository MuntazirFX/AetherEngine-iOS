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
