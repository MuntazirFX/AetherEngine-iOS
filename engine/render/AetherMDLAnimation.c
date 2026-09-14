#include "AetherMDLAnimation.h"
#include <string.h>
aether_result_t aether_mdl_animation_init(aether_mdl_animation_t*a,u32 bones){if(!a)return AETHER_ERR_INVALID_ARG;memset(a,0,sizeof(*a));a->bone_count=bones;a->fps=30.0f;return AETHER_OK;}
void aether_mdl_animation_play(aether_mdl_animation_t*a,u32 seq,f32 fps,bool loop){if(!a)return;a->sequence=seq;a->fps=fps>0?fps:30.0f;a->loop=loop;a->time=0;a->frame=0;a->playing=true;}
void aether_mdl_animation_update(aether_mdl_animation_t*a,f32 dt,u32 frames){if(!a||!a->playing||!frames)return;a->time+=dt;a->frame=a->time*a->fps;if(a->frame>=(f32)frames){if(a->loop){a->frame=(f32)((u32)a->frame%frames);a->time=a->frame/a->fps;}else{a->frame=(f32)(frames-1);a->playing=false;}}}
void aether_mdl_animation_stop(aether_mdl_animation_t*a){if(a)a->playing=false;}
