#ifndef AETHER_LIGHTMAP_H
#define AETHER_LIGHTMAP_H
#include "../core/AetherCore.h"
typedef struct aether_lightmap { u32 width, height; u32 style_count; bool enabled; } aether_lightmap_t;
aether_result_t aether_lightmap_init(aether_lightmap_t *lm, u32 width, u32 height, u32 styles);
void aether_lightmap_shutdown(aether_lightmap_t *lm);
void aether_lightmap_enable(aether_lightmap_t *lm, bool enabled);
bool aether_lightmap_is_enabled(const aether_lightmap_t *lm);
#endif
