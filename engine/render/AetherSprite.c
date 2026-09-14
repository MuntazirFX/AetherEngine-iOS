#include "AetherSprite.h"
#include <string.h>
aether_result_t aether_sprite_init(aether_sprite_t*s){if(!s)return AETHER_ERR_INVALID_ARG;memset(s,0,sizeof(*s));s->size[0]=s->size[1]=1.0f;s->uv[2]=s->uv[3]=1.0f;s->color[0]=s->color[1]=s->color[2]=s->color[3]=1.0f;s->visible=true;return AETHER_OK;}
void aether_sprite_set_uv(aether_sprite_t*s,f32 u0,f32 v0,f32 u1,f32 v1){if(!s)return;s->uv[0]=u0;s->uv[1]=v0;s->uv[2]=u1;s->uv[3]=v1;}
void aether_sprite_set_color(aether_sprite_t*s,const f32 c[4]){if(s&&c)memcpy(s->color,c,sizeof(s->color));}
