#include "AetherPostFX.h"
#include <math.h>
#include <string.h>

aether_result_t aether_postfx_init(aether_postfx_t *p) {
    if (!p) return AETHER_ERR_INVALID_ARG;
    memset(p, 0, sizeof(*p));
    p->exposure = 1.0f; p->contrast = 1.0f; p->saturation = 1.0f;
    p->brightness = 0.0f; p->gamma = 1.0f; p->enabled = true;
    return AETHER_OK;
}
void aether_postfx_shutdown(aether_postfx_t *p) { if (p) memset(p, 0, sizeof(*p)); }
void aether_postfx_set_bloom(aether_postfx_t *p, f32 amount) {
    if (!p) return;
    if (amount < 0) amount = 0;
    if (amount > 1) amount = 1;
    p->bloom = amount;
}
void aether_postfx_set_brightness(aether_postfx_t *p, f32 brightness) {
    if (!p) return;
    if (brightness < -1.f) brightness = -1.f;
    if (brightness > 1.f) brightness = 1.f;
    p->brightness = brightness;
}
void aether_postfx_set_gamma(aether_postfx_t *p, f32 gamma) {
    if (!p) return;
    if (gamma < 0.2f) gamma = 0.2f;
    if (gamma > 3.f) gamma = 3.f;
    p->gamma = gamma;
}
void aether_postfx_set_from_cvars(aether_postfx_t *p, f32 brightness, f32 gamma) {
    if (!p) return;
    aether_postfx_set_brightness(p, brightness);
    aether_postfx_set_gamma(p, gamma);
    p->enabled = true;
}
void aether_postfx_apply_rgb(const aether_postfx_t *p, f32 rgb[3]) {
    if (!p || !rgb || !p->enabled) return;
    for (int i = 0; i < 3; ++i) {
        f32 v = rgb[i] * p->exposure + p->brightness;
        if (v < 0.f) v = 0.f;
        if (p->gamma > 1e-4f) v = powf(v, 1.f / p->gamma);
        if (v > 1.f) v = 1.f;
        rgb[i] = v;
    }
}
u32 aether_postfx_copy_fullscreen(aether_postfx_vertex_t *out, u32 max_out) {
    if (!out || max_out < 6) return 0;
    /* NDC fullscreen two-triangle quad. */
    f32 corners[4][4] = {
        {-1,-1, 0,0}, { 1,-1, 1,0}, { 1, 1, 1,1}, {-1, 1, 0,1}
    };
    int idx[6] = {0,1,2, 0,2,3};
    for (int k = 0; k < 6; ++k) {
        int c = idx[k];
        out[k].x = corners[c][0]; out[k].y = corners[c][1]; out[k].z = 0.f;
        out[k].u = corners[c][2]; out[k].v = corners[c][3];
    }
    return 6;
}
