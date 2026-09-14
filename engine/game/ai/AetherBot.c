#include "AetherBot.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
static f32 dist(aether_vec3_t a,aether_vec3_t b){f32 x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return sqrtf(x*x+y*y+z*z);}
void aether_bot_manager_init(aether_bot_manager_t*m){if(m)memset(m,0,sizeof*m);}
aether_bot_t*aether_bot_spawn(aether_bot_manager_t*m,const char*n,aether_vec3_t p){if(!m||m->count>=AETHER_BOT_MAX)return NULL;aether_bot_t*b=&m->bots[m->count];memset(b,0,sizeof*b);b->active=true;b->id=++m->count;snprintf(b->name,sizeof b->name,"%s",n?n:"Bot");b->state=AETHER_BOT_ROAM;b->position=p;b->target_node=(u32)-1;b->health=100;return b;}
void aether_bot_set_target(aether_bot_t*b,aether_vec3_t p){if(!b)return;b->target_position=p;b->state=AETHER_BOT_SEEK;}
void aether_bot_tick(aether_bot_manager_t*m,const aether_path_graph_t*g,f32 dt){if(!m)return;for(u32 i=0;i<m->count;i++){aether_bot_t*b=&m->bots[i];if(!b->active||b->health<=0){b->state=AETHER_BOT_DEAD;continue;}b->think_time-=dt;b->attack_cooldown-=dt;if(b->think_time>0)continue;b->think_time=0.1f;if(b->state==AETHER_BOT_SEEK||b->state==AETHER_BOT_ROAM){aether_vec3_t d={b->target_position.x-b->position.x,b->target_position.y-b->position.y,b->target_position.z-b->position.z};f32 len=sqrtf(d.x*d.x+d.y*d.y+d.z*d.z);if(len<32){b->state=AETHER_BOT_ATTACK;}else if(len>0){f32 speed=160.0f*0.1f;b->position.x+=d.x/len*speed;b->position.y+=d.y/len*speed;b->position.z+=d.z/len*speed;}}else if(b->state==AETHER_BOT_ATTACK&&dist(b->position,b->target_position)>512)b->state=AETHER_BOT_SEEK; (void)g;}}
