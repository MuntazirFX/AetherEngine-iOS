/* AetherDynLight.h — Dynamic / entity light stubs for Metal vertical slice.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_DYN_LIGHT_H
#define AETHER_DYN_LIGHT_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MAX_DYN_LIGHTS 64

typedef struct aether_dyn_light {
    f32 position[3];
    f32 color[3];     /* linear RGB 0..1 */
    f32 radius;
    f32 intensity;
    bool active;
} aether_dyn_light_t;

typedef struct aether_dyn_lights {
    aether_dyn_light_t items[AETHER_MAX_DYN_LIGHTS];
    u32 count;
} aether_dyn_lights_t;

/* GPU/bridge-friendly: pos.xyz + radius + color.rgb + intensity = 8 floats */
typedef struct aether_dyn_light_vertex {
    f32 x, y, z, radius;
    f32 r, g, b, intensity;
} aether_dyn_light_vertex_t;

aether_result_t aether_dyn_lights_init(aether_dyn_lights_t *dl);
void            aether_dyn_lights_clear(aether_dyn_lights_t *dl);
aether_result_t aether_dyn_lights_add(aether_dyn_lights_t *dl,
                                      const f32 pos[3], const f32 color[3],
                                      f32 radius, f32 intensity);
u32 aether_dyn_lights_active_count(const aether_dyn_lights_t *dl);
u32 aether_dyn_lights_copy_render(const aether_dyn_lights_t *dl,
                                  aether_dyn_light_vertex_t *out, u32 max_out);

/* Sample summed RGB tint at a world point (attenuation by radius). */
void aether_dyn_lights_sample_rgb(const aether_dyn_lights_t *dl,
                                  f32 x, f32 y, f32 z,
                                  f32 out_rgb[3]);

/* Forward decls (full types in AetherBSPGeometry / AetherLightmap). */
struct aether_mesh;
struct aether_lightmap;

/* Apply dyn-light tint into per-vertex RGB buffer (3 floats per mesh vertex).
 * out_rgb length >= vertex_count*3. Returns vertices tinted. */
u32 aether_dyn_lights_apply_mesh_tint(const aether_dyn_lights_t *dl,
                                      const struct aether_mesh *mesh,
                                      f32 *out_rgb, u32 max_floats);

/* Bake a simple additive influence into lightmap atlas samples (host/Metal stub). */
aether_result_t aether_dyn_lights_modulate_lightmap(const aether_dyn_lights_t *dl,
                                                    struct aether_lightmap *lm);

/* GPU UBO: packed lights for Metal fragment (pos.xyz, radius, color.rgb, intensity). */
#define AETHER_DYN_LIGHT_UBO_MAX 16

typedef struct aether_dyn_light_ubo {
    u32 count;
    u32 pad0, pad1, pad2;
    aether_dyn_light_vertex_t lights[AETHER_DYN_LIGHT_UBO_MAX];
} aether_dyn_light_ubo_t;

/* Fill UBO for GPU — up to AETHER_DYN_LIGHT_UBO_MAX active lights. Returns count packed. */
u32 aether_dyn_lights_fill_ubo(const aether_dyn_lights_t *dl, aether_dyn_light_ubo_t *ubo);

/* Flat float array uniforms for Metal fragment (count + pad3 + lights[N]*8).
 * Layout: [count,0,0,0, x,y,z,r, R,G,B,I, ...]. Returns floats written. */
u32 aether_dyn_lights_fill_array(const aether_dyn_lights_t *dl,
                                 f32 *out, u32 max_floats);

/* Sample with optional ambient floor (improves thin fragment path host-side). */
void aether_dyn_lights_sample_rgb_ex(const aether_dyn_lights_t *dl,
                                     f32 x, f32 y, f32 z,
                                     f32 ambient, f32 out_rgb[3]);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_DYN_LIGHT_H */
