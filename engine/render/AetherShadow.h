#ifndef AETHER_SHADOW_H
#define AETHER_SHADOW_H
#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_shadow {
    f32 light_dir[3];
    f32 bias;
    f32 strength;
    u32 map_size;
    bool enabled;
} aether_shadow_t;

/* Soft blob shadow quad under an entity (ground plane Z ≈ ground_z). */
typedef struct aether_blob_shadow_vertex {
    f32 x, y, z;
    f32 u, v;
    f32 alpha;
    f32 pad;
} aether_blob_shadow_vertex_t;

aether_result_t aether_shadow_init(aether_shadow_t *s, u32 map_size);
void aether_shadow_shutdown(aether_shadow_t *s);
void aether_shadow_set_light(aether_shadow_t *s, const f32 direction[3]);
void aether_shadow_set_enabled(aether_shadow_t *s, bool enabled);

/* Emit 6 verts (2 tris) for a soft oval blob at (px,py,ground_z), radius r.
 * Returns verts written (0 or 6). */
u32 aether_shadow_copy_blob(const aether_shadow_t *s,
                            f32 px, f32 py, f32 ground_z, f32 radius,
                            aether_blob_shadow_vertex_t *out, u32 max_out);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_SHADOW_H */
