#include "AetherParticle.h"
#include <string.h>
aether_result_t aether_particles_init(aether_particles_t*p){if(!p)return AETHER_ERR_INVALID_ARG;memset(p,0,sizeof(*p));return AETHER_OK;}
aether_result_t aether_particles_spawn(aether_particles_t*p,const f32 pos[3],const f32 vel[3],f32 size,f32 life,const f32 color[4]){if(!p||!pos||!vel||!color||size<=0||life<=0)return AETHER_ERR_INVALID_ARG;u32 slot=p->count<AETHER_MAX_PARTICLES?p->count:(p->count%AETHER_MAX_PARTICLES);aether_particle_t*x=&p->items[slot];memcpy(x->position,pos,sizeof(x->position));memcpy(x->velocity,vel,sizeof(x->velocity));memcpy(x->color,color,sizeof(x->color));x->size=size;x->life=life;x->age=0;x->active=true;if(p->count<AETHER_MAX_PARTICLES)p->count++;return AETHER_OK;}
void aether_particles_update(aether_particles_t*p,f32 dt){if(!p)return;for(u32 i=0;i<p->count;i++){aether_particle_t*x=&p->items[i];if(!x->active)continue;for(int k=0;k<3;k++)x->position[k]+=x->velocity[k]*dt;x->age+=dt;if(x->age>=x->life)x->active=false;}}
void aether_particles_clear(aether_particles_t*p){if(p)memset(p,0,sizeof(*p));}
