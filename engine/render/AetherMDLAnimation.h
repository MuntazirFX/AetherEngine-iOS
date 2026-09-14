#ifndef AETHER_MDL_ANIMATION_H
#define AETHER_MDL_ANIMATION_H
#include "../core/AetherCore.h"
typedef struct aether_mdl_animation { u32 sequence; u32 bone_count; f32 time; f32 fps; f32 frame; bool loop; bool playing; } aether_mdl_animation_t;
aether_result_t aether_mdl_animation_init(aether_mdl_animation_t *a, u32 bone_count);
void aether_mdl_animation_play(aether_mdl_animation_t *a, u32 sequence, f32 fps, bool loop);
void aether_mdl_animation_update(aether_mdl_animation_t *a, f32 dt, u32 frame_count);
void aether_mdl_animation_stop(aether_mdl_animation_t *a);
#endif
