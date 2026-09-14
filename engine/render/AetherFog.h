#ifndef AETHER_FOG_H
#define AETHER_FOG_H
#include "../core/AetherCore.h"
typedef struct aether_fog { f32 density; f32 start; f32 end; f32 color[4]; bool enabled; } aether_fog_t;
aether_result_t aether_fog_init(aether_fog_t *f);
void aether_fog_shutdown(aether_fog_t *f);
void aether_fog_set_range(aether_fog_t *f, f32 start, f32 end);
void aether_fog_set_density(aether_fog_t *f, f32 density);
#endif
