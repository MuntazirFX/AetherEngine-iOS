#include "AetherWater.h"
#include <string.h>
aether_result_t aether_water_init(aether_water_t *w){if(!w)return AETHER_ERR_INVALID_ARG;memset(w,0,sizeof(*w));w->wave_speed=1.0f;w->opacity=0.85f;w->enabled=true;return AETHER_OK;}
void aether_water_shutdown(aether_water_t *w){if(w)memset(w,0,sizeof(*w));}
void aether_water_update(aether_water_t *w,f32 dt){if(w&&w->enabled)w->wave_time+=dt*w->wave_speed;}
void aether_water_set_enabled(aether_water_t *w,bool e){if(w)w->enabled=e;}
