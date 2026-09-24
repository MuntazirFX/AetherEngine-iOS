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

#ifdef __cplusplus
}
#endif
#endif /* AETHER_DYN_LIGHT_H */
