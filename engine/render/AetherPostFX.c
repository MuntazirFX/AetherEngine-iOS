#include "AetherPostFX.h"
#include <string.h>
aether_result_t aether_postfx_init(aether_postfx_t*p){if(!p)return AETHER_ERR_INVALID_ARG;memset(p,0,sizeof(*p));p->exposure=1.0f;p->contrast=1.0f;p->saturation=1.0f;p->enabled=true;return AETHER_OK;}
void aether_postfx_shutdown(aether_postfx_t*p){if(p)memset(p,0,sizeof(*p));}
void aether_postfx_set_bloom(aether_postfx_t*p,f32 amount){if(p){if(amount<0)amount=0;if(amount>1)amount=1;p->bloom=amount;}}
