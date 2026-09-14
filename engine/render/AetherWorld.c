#include "AetherWorld.h"
#include <string.h>
aether_result_t aether_world_render_init(aether_world_render_t*w){if(!w)return AETHER_ERR_INVALID_ARG;memset(w,0,sizeof(*w));w->backface_culling=true;w->depth_test=true;return AETHER_OK;}
void aether_world_render_shutdown(aether_world_render_t*w){if(w)memset(w,0,sizeof(*w));}
void aether_world_render_set_surface_count(aether_world_render_t*w,u32 c){if(w){w->surface_count=c;w->visible_surface_count=c;}}
