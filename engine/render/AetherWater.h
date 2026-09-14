#ifndef AETHER_WATER_H
#define AETHER_WATER_H
#include "../core/AetherCore.h"
typedef struct aether_water { f32 wave_time; f32 wave_speed; f32 opacity; bool enabled; } aether_water_t;
aether_result_t aether_water_init(aether_water_t *w);
void aether_water_shutdown(aether_water_t *w);
void aether_water_update(aether_water_t *w, f32 dt);
void aether_water_set_enabled(aether_water_t *w, bool enabled);
#endif
