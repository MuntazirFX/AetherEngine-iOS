#ifndef AETHER_POSTFX_H
#define AETHER_POSTFX_H
#include "../core/AetherCore.h"
typedef struct aether_postfx { f32 bloom; f32 exposure; f32 contrast; f32 saturation; bool enabled; } aether_postfx_t;
aether_result_t aether_postfx_init(aether_postfx_t *p);
void aether_postfx_shutdown(aether_postfx_t *p);
void aether_postfx_set_bloom(aether_postfx_t *p, f32 amount);
#endif
