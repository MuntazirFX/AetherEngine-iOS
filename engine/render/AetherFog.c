#include "AetherFog.h"
#include <string.h>

static void set4(f32 dst[4], f32 r, f32 g, f32 b, f32 a) {
    dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
}

aether_result_t aether_fog_init(aether_fog_t *f) {
    if (!f) return AETHER_ERR_INVALID_ARG;
    memset(f, 0, sizeof(*f));
    f->density = 0.45f;
    f->start = 128.0f;
    f->end = 4096.0f;
    f->factor = 0.35f;
    f->enabled = true;
    /* Soft cool haze — clean-room placeholder, no game assets. */
    set4(f->color, 0.55f, 0.65f, 0.78f, 1.0f);
    return AETHER_OK;
}

void aether_fog_shutdown(aether_fog_t *f) {
    if (f) memset(f, 0, sizeof(*f));
}

void aether_fog_set_enabled(aether_fog_t *f, bool enabled) {
    if (f) f->enabled = enabled;
}

void aether_fog_set_range(aether_fog_t *f, f32 s, f32 e) {
    if (!f) return;
    if (s < 0.0f) s = 0.0f;
    if (e < s) e = s;
    f->start = s;
    f->end = e;
}

void aether_fog_set_density(aether_fog_t *f, f32 d) {
    if (!f) return;
    if (d < 0.0f) d = 0.0f;
    if (d > 1.0f) d = 1.0f;
    f->density = d;
    if (d > 0.0f) f->enabled = true;
}

aether_result_t aether_fog_set_color(aether_fog_t *f, const f32 rgba[4]) {
    if (!f || !rgba) return AETHER_ERR_INVALID_ARG;
    set4(f->color, rgba[0], rgba[1], rgba[2], rgba[3]);
    return AETHER_OK;
}

aether_result_t aether_fog_set_factor(aether_fog_t *f, f32 factor) {
    if (!f) return AETHER_ERR_INVALID_ARG;
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    f->factor = factor;
    return AETHER_OK;
}

u32 aether_fog_render_vertex_count(void) {
    return 6u; /* two triangles covering NDC */
}

u32 aether_fog_copy_render(const aether_fog_t *f,
                           aether_fog_vertex_t *out,
                           u32 max_out) {
    if (!f || !out || !f->enabled || max_out < 6u) return 0;

    f32 alpha = f->density * f->factor * f->color[3];
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    if (alpha <= 1e-5f) return 0;

    const f32 r = f->color[0];
    const f32 g = f->color[1];
    const f32 b = f->color[2];

    /* Fullscreen NDC quad as two CCW triangles (clip space, z=0 unused). */
    static const f32 corners[4][4] = {
        /* x, y, u, v */
        {-1.0f, -1.0f, 0.0f, 1.0f},
        { 1.0f, -1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f, 0.0f},
    };
    static const int idx[6] = {0, 1, 2, 0, 2, 3};

    for (int i = 0; i < 6; ++i) {
        const f32 *c = corners[idx[i]];
        out[i].x = c[0];
        out[i].y = c[1];
        out[i].u = c[2];
        out[i].v = c[3];
        out[i].r = r;
        out[i].g = g;
        out[i].b = b;
        out[i].a = alpha;
    }
    return 6u;
}
