#include "AetherSky.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Hemisphere tessellation: rings (latitude) x segs (longitude). */
#define AETHER_SKY_RINGS 8
#define AETHER_SKY_SEGS  16

static void copy4(f32 dst[4], const f32 src[4]) {
    dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
}

static void set4(f32 dst[4], f32 r, f32 g, f32 b, f32 a) {
    dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
}

aether_result_t aether_sky_init(aether_sky_t *s) {
    if (!s) return AETHER_ERR_INVALID_ARG;
    memset(s, 0, sizeof(*s));
    s->face_count = AETHER_SKY_FACE_COUNT;
    s->enabled = true;
    s->radius = 4096.0f;
    aether_str_copy(s->name, sizeof(s->name), "desert");
    /* Default face tints — soft blue day sky (clean-room placeholder). */
    set4(s->face_color[AETHER_SKY_FACE_RT], 0.45f, 0.65f, 0.95f, 1.0f);
    set4(s->face_color[AETHER_SKY_FACE_LF], 0.45f, 0.65f, 0.95f, 1.0f);
    set4(s->face_color[AETHER_SKY_FACE_FT], 0.50f, 0.70f, 0.98f, 1.0f);
    set4(s->face_color[AETHER_SKY_FACE_BK], 0.40f, 0.60f, 0.90f, 1.0f);
    set4(s->face_color[AETHER_SKY_FACE_UP], 0.25f, 0.45f, 0.90f, 1.0f);
    set4(s->face_color[AETHER_SKY_FACE_DN], 0.70f, 0.75f, 0.80f, 1.0f);
    aether_sky_rebuild_gradient(s);
    return AETHER_OK;
}

aether_result_t aether_sky_set_name(aether_sky_t *s, const char *n) {
    if (!s || !n) return AETHER_ERR_INVALID_ARG;
    aether_str_copy(s->name, sizeof(s->name), n);
    s->face_count = AETHER_SKY_FACE_COUNT;
    return AETHER_OK;
}

aether_result_t aether_sky_set_enabled(aether_sky_t *s, bool enabled) {
    if (!s) return AETHER_ERR_INVALID_ARG;
    s->enabled = enabled;
    return AETHER_OK;
}

aether_result_t aether_sky_set_radius(aether_sky_t *s, f32 radius) {
    if (!s || radius <= 0.0f) return AETHER_ERR_INVALID_ARG;
    s->radius = radius;
    return AETHER_OK;
}

aether_result_t aether_sky_set_face_color(aether_sky_t *s,
                                          aether_sky_face_t face,
                                          const f32 rgba[4]) {
    if (!s || !rgba || (int)face < 0 || face >= AETHER_SKY_FACE_COUNT)
        return AETHER_ERR_INVALID_ARG;
    copy4(s->face_color[face], rgba);
    aether_sky_rebuild_gradient(s);
    return AETHER_OK;
}

void aether_sky_rebuild_gradient(aether_sky_t *s) {
    if (!s) return;
    copy4(s->top_color, s->face_color[AETHER_SKY_FACE_UP]);
    copy4(s->bottom_color, s->face_color[AETHER_SKY_FACE_DN]);
    /* Horizon = average of the four side faces. */
    for (int k = 0; k < 4; ++k) {
        s->horizon_color[k] =
            0.25f * (s->face_color[AETHER_SKY_FACE_RT][k] +
                     s->face_color[AETHER_SKY_FACE_LF][k] +
                     s->face_color[AETHER_SKY_FACE_FT][k] +
                     s->face_color[AETHER_SKY_FACE_BK][k]);
    }
}

void aether_sky_shutdown(aether_sky_t *s) {
    if (s) memset(s, 0, sizeof(*s));
}

u32 aether_sky_render_vertex_count(void) {
    /* Each ring band × each segment → 2 triangles → 6 verts. */
    return (u32)(AETHER_SKY_RINGS * AETHER_SKY_SEGS * 6);
}

static void lerp4(f32 out[4], const f32 a[4], const f32 b[4], f32 t) {
    out[0] = a[0] + (b[0] - a[0]) * t;
    out[1] = a[1] + (b[1] - a[1]) * t;
    out[2] = a[2] + (b[2] - a[2]) * t;
    out[3] = a[3] + (b[3] - a[3]) * t;
}

static void color_at_elev(const aether_sky_t *s, f32 elev01, f32 out[4]) {
    /* elev01: 0 = horizon, 1 = zenith. Below horizon blends to bottom. */
    if (elev01 >= 0.0f) {
        lerp4(out, s->horizon_color, s->top_color, elev01);
    } else {
        f32 t = elev01 + 1.0f; /* -1..0 → 0..1 */
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        lerp4(out, s->bottom_color, s->horizon_color, t);
    }
}

static void emit_vert(aether_sky_vertex_t *v, f32 x, f32 y, f32 z,
                      const f32 rgba[4], f32 radius) {
    v->x = x * radius;
    v->y = y * radius;
    v->z = z * radius;
    v->r = rgba[0];
    v->g = rgba[1];
    v->b = rgba[2];
    v->a = rgba[3];
}

u32 aether_sky_copy_render(const aether_sky_t *s,
                           aether_sky_vertex_t *out,
                           u32 max_out) {
    if (!s || !out || !s->enabled || max_out == 0) return 0;
    const u32 need = aether_sky_render_vertex_count();
    if (max_out < need) return 0;

    const f32 radius = (s->radius > 0.0f) ? s->radius : 4096.0f;
    u32 written = 0;

    /* Full sphere so the camera can look down; gradient covers top→bottom. */
    for (int ring = 0; ring < AETHER_SKY_RINGS; ++ring) {
        f32 t0 = (f32)ring / (f32)AETHER_SKY_RINGS;
        f32 t1 = (f32)(ring + 1) / (f32)AETHER_SKY_RINGS;
        /* Map ring 0..RINGS to elevation +1 (zenith) → -1 (nadir). */
        f32 elev0 = 1.0f - 2.0f * t0;
        f32 elev1 = 1.0f - 2.0f * t1;
        f32 phi0 = (f32)((0.5 - t0) * M_PI); /* +pi/2 .. -pi/2 */
        f32 phi1 = (f32)((0.5 - t1) * M_PI);
        f32 cos0 = cosf(phi0), sin0 = sinf(phi0);
        f32 cos1 = cosf(phi1), sin1 = sinf(phi1);
        f32 col0[4], col1[4];
        color_at_elev(s, elev0, col0);
        color_at_elev(s, elev1, col1);

        for (int seg = 0; seg < AETHER_SKY_SEGS; ++seg) {
            f32 a0 = (f32)(seg * 2.0 * M_PI / AETHER_SKY_SEGS);
            f32 a1 = (f32)((seg + 1) * 2.0 * M_PI / AETHER_SKY_SEGS);
            f32 c0 = cosf(a0), sn0 = sinf(a0);
            f32 c1 = cosf(a1), sn1 = sinf(a1);

            /* Quad as two triangles (inward-facing so camera inside sees it). */
            f32 x00 = c0 * cos0, y00 = sn0 * cos0, z00 = sin0;
            f32 x10 = c1 * cos0, y10 = sn1 * cos0, z10 = sin0;
            f32 x01 = c0 * cos1, y01 = sn0 * cos1, z01 = sin1;
            f32 x11 = c1 * cos1, y11 = sn1 * cos1, z11 = sin1;

            emit_vert(&out[written++], x00, y00, z00, col0, radius);
            emit_vert(&out[written++], x01, y01, z01, col1, radius);
            emit_vert(&out[written++], x10, y10, z10, col0, radius);

            emit_vert(&out[written++], x10, y10, z10, col0, radius);
            emit_vert(&out[written++], x01, y01, z01, col1, radius);
            emit_vert(&out[written++], x11, y11, z11, col1, radius);
        }
    }
    return written;
}
