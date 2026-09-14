#ifndef AETHER_DECAL_H
#define AETHER_DECAL_H
#include "../core/AetherCore.h"
typedef struct aether_decal { f32 position[3]; f32 normal[3]; f32 size; f32 life; f32 age; bool active; } aether_decal_t;
#define AETHER_MAX_DECALS 256
typedef struct aether_decals { aether_decal_t items[AETHER_MAX_DECALS]; u32 count; } aether_decals_t;
aether_result_t aether_decals_init(aether_decals_t *d);
aether_result_t aether_decals_add(aether_decals_t *d, const f32 pos[3], const f32 normal[3], f32 size, f32 life);
void aether_decals_update(aether_decals_t *d, f32 dt);
void aether_decals_clear(aether_decals_t *d);
#endif
