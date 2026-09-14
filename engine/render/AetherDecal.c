#include "AetherDecal.h"
#include <string.h>
aether_result_t aether_decals_init(aether_decals_t*d){if(!d)return AETHER_ERR_INVALID_ARG;memset(d,0,sizeof(*d));return AETHER_OK;}
aether_result_t aether_decals_add(aether_decals_t*d,const f32 p[3],const f32 n[3],f32 size,f32 life){if(!d||!p||!n||size<=0||life<0)return AETHER_ERR_INVALID_ARG;u32 slot=d->count<AETHER_MAX_DECALS?d->count:(d->count%AETHER_MAX_DECALS);aether_decal_t*x=&d->items[slot];memcpy(x->position,p,sizeof(x->position));memcpy(x->normal,n,sizeof(x->normal));x->size=size;x->life=life;x->age=0;x->active=true;if(d->count<AETHER_MAX_DECALS)d->count++;return AETHER_OK;}
void aether_decals_update(aether_decals_t*d,f32 dt){if(!d)return;for(u32 i=0;i<d->count;i++){aether_decal_t*x=&d->items[i];if(!x->active)continue;x->age+=dt;if(x->life>0&&x->age>=x->life)x->active=false;}}
void aether_decals_clear(aether_decals_t*d){if(d)memset(d,0,sizeof(*d));}
