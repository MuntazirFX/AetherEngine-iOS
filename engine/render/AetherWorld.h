#ifndef AETHER_WORLD_RENDER_H
#define AETHER_WORLD_RENDER_H
#include "../core/AetherCore.h"
typedef struct aether_world_render {
    u32 surface_count;          /* total mesh triangles */
    u32 visible_surface_count;  /* after leaf/PVS cull */
    bool backface_culling;
    bool depth_test;
} aether_world_render_t;
aether_result_t aether_world_render_init(aether_world_render_t *w);
void aether_world_render_shutdown(aether_world_render_t *w);
void aether_world_render_set_surface_count(aether_world_render_t *w, u32 count);
void aether_world_render_set_visible_surface_count(aether_world_render_t *w, u32 count);
#endif
