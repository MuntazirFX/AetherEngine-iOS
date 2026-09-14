#include "AetherSky.h"
#include <string.h>
aether_result_t aether_sky_init(aether_sky_t *s){if(!s)return AETHER_ERR_INVALID_ARG;memset(s,0,sizeof(*s));s->face_count=6;s->enabled=true;return AETHER_OK;}
aether_result_t aether_sky_set_name(aether_sky_t *s,const char *n){if(!s||!n)return AETHER_ERR_INVALID_ARG;aether_str_copy(s->name,sizeof(s->name),n);s->face_count=6;return AETHER_OK;}
void aether_sky_shutdown(aether_sky_t *s){if(s)memset(s,0,sizeof(*s));}
