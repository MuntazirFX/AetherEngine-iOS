#ifndef AETHER_SHADOW_H
#define AETHER_SHADOW_H
#include "../core/AetherCore.h"
typedef struct aether_shadow { f32 light_dir[3]; f32 bias; f32 strength; u32 map_size; bool enabled; } aether_shadow_t;
aether_result_t aether_shadow_init(aether_shadow_t *s, u32 map_size);
void aether_shadow_shutdown(aether_shadow_t *s);
void aether_shadow_set_light(aether_shadow_t *s, const f32 direction[3]);
void aether_shadow_set_enabled(aether_shadow_t *s, bool enabled);
#endif
