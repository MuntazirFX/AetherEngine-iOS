#include "AetherShadow.h"
#include <string.h>
aether_result_t aether_shadow_init(aether_shadow_t*s,u32 size){if(!s||!size)return AETHER_ERR_INVALID_ARG;memset(s,0,sizeof(*s));s->map_size=size;s->light_dir[0]=0.3f;s->light_dir[1]=-1.0f;s->light_dir[2]=0.4f;s->bias=0.001f;s->strength=0.7f;s->enabled=true;return AETHER_OK;}
void aether_shadow_shutdown(aether_shadow_t*s){if(s)memset(s,0,sizeof(*s));}
void aether_shadow_set_light(aether_shadow_t*s,const f32 d[3]){if(s&&d)memcpy(s->light_dir,d,sizeof(s->light_dir));}
void aether_shadow_set_enabled(aether_shadow_t*s,bool e){if(s)s->enabled=e;}
