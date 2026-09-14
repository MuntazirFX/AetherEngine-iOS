#ifndef AETHER_SPRITE_H
#define AETHER_SPRITE_H
#include "../core/AetherCore.h"
typedef struct aether_sprite { f32 position[3]; f32 size[2]; f32 uv[4]; f32 rotation; f32 color[4]; bool visible; } aether_sprite_t;
aether_result_t aether_sprite_init(aether_sprite_t *s);
void aether_sprite_set_uv(aether_sprite_t *s, f32 u0, f32 v0, f32 u1, f32 v1);
void aether_sprite_set_color(aether_sprite_t *s, const f32 color[4]);
#endif
