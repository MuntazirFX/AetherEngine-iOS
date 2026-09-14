#include "AetherFog.h"
#include <string.h>
aether_result_t aether_fog_init(aether_fog_t *f){if(!f)return AETHER_ERR_INVALID_ARG;memset(f,0,sizeof(*f));f->end=4096.0f;f->color[0]=f->color[1]=f->color[2]=1.0f;f->color[3]=1.0f;return AETHER_OK;}
void aether_fog_shutdown(aether_fog_t *f){if(f)memset(f,0,sizeof(*f));}
void aether_fog_set_range(aether_fog_t *f,f32 s,f32 e){if(!f)return;if(s<0)s=0;if(e<s)e=s;f->start=s;f->end=e;}
void aether_fog_set_density(aether_fog_t *f,f32 d){if(f){if(d<0)d=0;if(d>1)d=1;f->density=d;f->enabled=d>0;}}
