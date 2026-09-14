#ifndef AETHER_SKY_H
#define AETHER_SKY_H
#include "../core/AetherCore.h"
typedef struct aether_sky { char name[64]; u32 face_count; bool enabled; } aether_sky_t;
aether_result_t aether_sky_init(aether_sky_t *s);
aether_result_t aether_sky_set_name(aether_sky_t *s, const char *name);
void aether_sky_shutdown(aether_sky_t *s);
#endif
