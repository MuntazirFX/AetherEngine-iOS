#include "AetherVGUILocalization.h"
#include <string.h>
#include <ctype.h>
static const char*tok(const char*p,char*out,size_t cap){while(*p&&isspace((unsigned char)*p))p++;if(*p=='"')p++;size_t n=0;while(*p&&*p!='"'&&*p!='\n'&&*p!='\r'){if(n+1<cap)out[n++]=*p;p++;}out[n]=0;if(*p=='"')p++;return p;}
void aether_vgui_localization_init(aether_vgui_localization_t*l){if(l)memset(l,0,sizeof*l);}
aether_result_t aether_vgui_localization_parse(aether_vgui_localization_t*l,const char*t){if(!l||!t)return AETHER_ERR_INVALID_ARG;aether_vgui_localization_init(l);const char*p=t;char k[96],v[256];while(*p&&l->count<AETHER_VGUI_MAX_TOKENS){p=tok(p,k,sizeof k);if(!*k){p++;continue;}while(*p&&*p!='"'&&*p!='\n'&&*p!='\r')p++;if(!*p)break;p=tok(p,v,sizeof v);aether_str_copy(l->entries[l->count].token,sizeof l->entries[0].token,k);aether_str_copy(l->entries[l->count].text,sizeof l->entries[0].text,v);l->count++;}return AETHER_OK;}
const char*aether_vgui_localize(const aether_vgui_localization_t*l,const char*t){if(!l||!t)return t;for(u32 i=0;i<l->count;i++)if(aether_str_eq(l->entries[i].token,t))return l->entries[i].text;return t;}
