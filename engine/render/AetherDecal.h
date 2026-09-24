#ifndef AETHER_DECAL_H
#define AETHER_DECAL_H
#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_decal {
    f32 position[3];
    f32 normal[3];
    f32 size;
    f32 life;
    f32 age;
    bool active;
} aether_decal_t;

#define AETHER_MAX_DECALS 256

typedef struct aether_decals {
    aether_decal_t items[AETHER_MAX_DECALS];
    u32 count;
} aether_decals_t;

/* Metal slice: center + normal + size + fade = 8 floats */
typedef struct aether_decal_vertex {
    f32 x, y, z;
    f32 nx, ny, nz;
    f32 size;
    f32 fade; /* 1..0 over life */
} aether_decal_vertex_t;

aether_result_t aether_decals_init(aether_decals_t *d);
aether_result_t aether_decals_add(aether_decals_t *d, const f32 pos[3], const f32 normal[3],
                                  f32 size, f32 life);
void aether_decals_update(aether_decals_t *d, f32 dt);
void aether_decals_clear(aether_decals_t *d);
u32  aether_decals_active_count(const aether_decals_t *d);
u32  aether_decals_copy_render(const aether_decals_t *d,
                               aether_decal_vertex_t *out, u32 max_out);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_DECAL_H */
