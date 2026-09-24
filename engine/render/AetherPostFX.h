#ifndef AETHER_POSTFX_H
#define AETHER_POSTFX_H
#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_postfx {
    f32 bloom;
    f32 exposure;
    f32 contrast;
    f32 saturation;
    f32 brightness; /* additive lift, default 0 */
    f32 gamma;      /* display gamma, default 1 */
    bool enabled;
    /* Offscreen color-target descriptor (Metal allocates the real texture). */
    u32  target_width;
    u32  target_height;
    bool offscreen_ready;
} aether_postfx_t;

/* Fullscreen quad for Metal post pass (pos.xy + uv). */
typedef struct aether_postfx_vertex {
    f32 x, y, z;
    f32 u, v;
} aether_postfx_vertex_t;

/* Pack of uniforms Metal/host can mirror (brightness, gamma, exposure, enabled). */
typedef struct aether_postfx_uniforms {
    f32 brightness;
    f32 gamma;
    f32 exposure;
    f32 enabled; /* 1 when offscreen path should sample scene */
} aether_postfx_uniforms_t;

aether_result_t aether_postfx_init(aether_postfx_t *p);
void aether_postfx_shutdown(aether_postfx_t *p);
void aether_postfx_set_bloom(aether_postfx_t *p, f32 amount);
void aether_postfx_set_brightness(aether_postfx_t *p, f32 brightness);
void aether_postfx_set_gamma(aether_postfx_t *p, f32 gamma);
void aether_postfx_set_from_cvars(aether_postfx_t *p, f32 brightness, f32 gamma);

/* Apply brightness/gamma to an RGB sample (host smoke / CPU path). */
void aether_postfx_apply_rgb(const aether_postfx_t *p, f32 rgb[3]);

/* Emit fullscreen triangle/quad for Metal post pass. Returns 6. */
u32 aether_postfx_copy_fullscreen(aether_postfx_vertex_t *out, u32 max_out);

/* Ensure offscreen color-target size; marks offscreen_ready when w,h > 0. */
aether_result_t aether_postfx_ensure_offscreen(aether_postfx_t *p, u32 width, u32 height);
bool aether_postfx_has_offscreen(const aether_postfx_t *p);
u32  aether_postfx_target_width(const aether_postfx_t *p);
u32  aether_postfx_target_height(const aether_postfx_t *p);

/* Fill GPU/host uniform block; enabled=1 only when offscreen_ready && p->enabled. */
void aether_postfx_fill_uniforms(const aether_postfx_t *p, aether_postfx_uniforms_t *out);

#ifdef __cplusplus
}
#endif
#endif
