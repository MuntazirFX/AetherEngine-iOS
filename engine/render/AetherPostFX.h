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
} aether_postfx_t;

/* Fullscreen quad for Metal post pass (pos.xy + uv). */
typedef struct aether_postfx_vertex {
    f32 x, y, z;
    f32 u, v;
} aether_postfx_vertex_t;

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

#ifdef __cplusplus
}
#endif
#endif
