#include "AetherRenderFeatures.h"
#include <string.h>

aether_result_t aether_render_features_init(aether_render_features_t *f,u32 width,u32 height){
    if(!f||!width||!height)return AETHER_ERR_INVALID_ARG;
    memset(f,0,sizeof(*f));
    if(aether_lightmap_init(&f->lightmap,width,height,4)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_water_init(&f->water)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_sky_init(&f->sky)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_fog_init(&f->fog)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_decals_init(&f->decals)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_particles_init(&f->particles)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_sprite_init(&f->sprite)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_mdl_animation_init(&f->mdl_animation,0)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_shadow_init(&f->shadow,2048)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_postfx_init(&f->postfx)!=AETHER_OK)return AETHER_ERR_GENERIC;
    if(aether_world_render_init(&f->world)!=AETHER_OK)return AETHER_ERR_GENERIC;
    f->initialized=true;
    return AETHER_OK;
}
void aether_render_features_shutdown(aether_render_features_t *f){
    if(!f)return;
    aether_postfx_shutdown(&f->postfx);aether_shadow_shutdown(&f->shadow);aether_world_render_shutdown(&f->world);
    aether_decals_clear(&f->decals);aether_particles_clear(&f->particles);aether_sprite_init(&f->sprite);
    aether_mdl_animation_stop(&f->mdl_animation);aether_fog_shutdown(&f->fog);aether_sky_shutdown(&f->sky);aether_water_shutdown(&f->water);aether_lightmap_shutdown(&f->lightmap);
    f->initialized=false;
}
void aether_render_features_update(aether_render_features_t *f,f32 dt){if(!f||!f->initialized||dt<0)return;aether_water_update(&f->water,dt);aether_decals_update(&f->decals,dt);aether_particles_update(&f->particles,dt);aether_mdl_animation_update(&f->mdl_animation,dt,256);}
