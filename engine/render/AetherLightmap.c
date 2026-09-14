#include "AetherLightmap.h"
#include <string.h>
aether_result_t aether_lightmap_init(aether_lightmap_t *lm,u32 w,u32 h,u32 styles){if(!lm||!w||!h)return AETHER_ERR_INVALID_ARG;memset(lm,0,sizeof(*lm));lm->width=w;lm->height=h;lm->style_count=styles?styles:1;lm->enabled=true;return AETHER_OK;}
void aether_lightmap_shutdown(aether_lightmap_t *lm){if(lm)memset(lm,0,sizeof(*lm));}
void aether_lightmap_enable(aether_lightmap_t *lm,bool e){if(lm)lm->enabled=e;}
bool aether_lightmap_is_enabled(const aether_lightmap_t *lm){return lm?lm->enabled:false;}
