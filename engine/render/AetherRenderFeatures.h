#ifndef AETHER_RENDER_FEATURES_H
#define AETHER_RENDER_FEATURES_H
#include "../core/AetherCore.h"
#include "AetherLightmap.h"
#include "AetherWater.h"
#include "AetherSky.h"
#include "AetherFog.h"
#include "AetherDecal.h"
#include "AetherParticle.h"
#include "AetherSprite.h"
#include "AetherMDLAnimation.h"
#include "AetherShadow.h"
#include "AetherPostFX.h"
#include "AetherWorld.h"
typedef struct aether_render_features { aether_lightmap_t lightmap; aether_water_t water; aether_sky_t sky; aether_fog_t fog; aether_decals_t decals; aether_particles_t particles; aether_sprite_t sprite; aether_mdl_animation_t mdl_animation; aether_shadow_t shadow; aether_postfx_t postfx; aether_world_render_t world; bool initialized; } aether_render_features_t;
aether_result_t aether_render_features_init(aether_render_features_t *f, u32 width, u32 height);
void aether_render_features_shutdown(aether_render_features_t *f);
void aether_render_features_update(aether_render_features_t *f, f32 dt);
#endif
