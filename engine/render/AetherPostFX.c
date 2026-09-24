#include "AetherPostFX.h"
#include <math.h>
#include <string.h>

aether_result_t aether_postfx_init(aether_postfx_t *p) {
    if (!p) return AETHER_ERR_INVALID_ARG;
    memset(p, 0, sizeof(*p));
    p->exposure = 1.0f; p->contrast = 1.0f; p->saturation = 1.0f;
    p->brightness = 0.0f; p->gamma = 1.0f; p->enabled = true;
    p->bloom_threshold = 0.8f; p->bloom_intensity = 0.0f; p->bloom_blur_radius = 2.0f;
    p->bloom_enabled = false;
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
aether_result_t aether_postfx_ensure_offscreen(aether_postfx_t *p, u32 width, u32 height) {
    if (!p || width == 0 || height == 0) return AETHER_ERR_INVALID_ARG;
    p->target_width = width;
    p->target_height = height;
    p->offscreen_ready = true;
    return AETHER_OK;
}
bool aether_postfx_has_offscreen(const aether_postfx_t *p) {
    return p && p->offscreen_ready && p->target_width > 0 && p->target_height > 0;
}
u32 aether_postfx_target_width(const aether_postfx_t *p) {
    return p ? p->target_width : 0;
}
u32 aether_postfx_target_height(const aether_postfx_t *p) {
    return p ? p->target_height : 0;
}
void aether_postfx_fill_uniforms(const aether_postfx_t *p, aether_postfx_uniforms_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!p) { out->gamma = 1.f; out->exposure = 1.f; return; }
    out->brightness = p->brightness;
    out->gamma = p->gamma > 1e-4f ? p->gamma : 1.f;
    out->exposure = p->exposure > 1e-4f ? p->exposure : 1.f;
    out->enabled = (p->enabled && aether_postfx_has_offscreen(p)) ? 1.f : 0.f;
}

void aether_postfx_set_bloom_chain(aether_postfx_t *p, f32 threshold, f32 intensity, f32 blur_radius) {
    if (!p) return;
    if (threshold < 0.f) threshold = 0.f;
    if (threshold > 1.f) threshold = 1.f;
    if (intensity < 0.f) intensity = 0.f;
    if (intensity > 4.f) intensity = 4.f;
    if (blur_radius < 0.f) blur_radius = 0.f;
    if (blur_radius > 16.f) blur_radius = 16.f;
    p->bloom_threshold = threshold;
    p->bloom_intensity = intensity;
    p->bloom_blur_radius = blur_radius;
    p->bloom_enabled = (intensity > 1e-4f);
    p->bloom = intensity > 1.f ? 1.f : intensity;
}

void aether_postfx_fill_bloom(const aether_postfx_t *p, aether_postfx_bloom_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!p) { out->threshold = 0.8f; return; }
    out->threshold = p->bloom_threshold > 0.f ? p->bloom_threshold : 0.8f;
    out->intensity = p->bloom_intensity;
    out->blur_radius = p->bloom_blur_radius > 0.f ? p->bloom_blur_radius : 2.f;
    out->enabled = (p->bloom_enabled && aether_postfx_has_offscreen(p) && p->enabled) ? 1.f : 0.f;
}

void aether_postfx_fill_uniforms_ex(const aether_postfx_t *p, f32 out8[8]) {
    if (!out8) return;
    memset(out8, 0, sizeof(f32) * 8);
    aether_postfx_uniforms_t u;
    aether_postfx_fill_uniforms(p, &u);
    out8[0] = u.brightness; out8[1] = u.gamma; out8[2] = u.exposure; out8[3] = u.enabled;
    aether_postfx_bloom_t b;
    aether_postfx_fill_bloom(p, &b);
    out8[4] = b.threshold; out8[5] = b.intensity; out8[6] = b.blur_radius; out8[7] = b.enabled;
}


bool aether_postfx_bloom_encode_needed(const aether_postfx_t *p) {
    if (!p || !p->enabled || !p->bloom_enabled) return false;
    if (!aether_postfx_has_offscreen(p)) return false;
    return p->bloom_intensity > 1e-4f;
}

void aether_postfx_bloom_target_size(const aether_postfx_t *p, u32 *out_w, u32 *out_h) {
    u32 w = p && p->target_width ? p->target_width : 1;
    u32 h = p && p->target_height ? p->target_height : 1;
    w = w > 1 ? (w / 2) : 1;
    h = h > 1 ? (h / 2) : 1;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
}

void aether_postfx_bloom_encode_plan(const aether_postfx_t *p,
                                     aether_postfx_bloom_plan_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->threshold = 0.8f;
    out->blur_radius = 2.f;
    if (!p) return;
    out->needed = aether_postfx_bloom_encode_needed(p);
    out->threshold = p->bloom_threshold > 0.f ? p->bloom_threshold : 0.8f;
    out->intensity = p->bloom_intensity;
    out->blur_radius = p->bloom_blur_radius > 0.f ? p->bloom_blur_radius : 2.f;
    aether_postfx_bloom_target_size(p, &out->target_w, &out->target_h);
    out->separable = true;
    out->pass_count = out->needed ? 4u : 0u; /* bright + H + V + combine */
}

void aether_postfx_bloom_bright_sample(const aether_postfx_t *p,
                                       const f32 rgb_in[3], f32 rgb_out[3]) {
    if (!rgb_out) return;
    if (!rgb_in) { rgb_out[0]=rgb_out[1]=rgb_out[2]=0; return; }
    f32 thr = (p && p->bloom_threshold > 0.f) ? p->bloom_threshold : 0.8f;
    f32 lum = 0.2126f*rgb_in[0] + 0.7152f*rgb_in[1] + 0.0722f*rgb_in[2];
    /* Soft knee */
    f32 knee = thr + 0.15f;
    f32 m = 0.f;
    if (lum <= thr) m = 0.f;
    else if (lum >= knee) m = 1.f;
    else {
        f32 t = (lum - thr) / (knee - thr);
        m = t * t * (3.f - 2.f * t);
    }
    rgb_out[0] = rgb_in[0] * m;
    rgb_out[1] = rgb_in[1] * m;
    rgb_out[2] = rgb_in[2] * m;
}
