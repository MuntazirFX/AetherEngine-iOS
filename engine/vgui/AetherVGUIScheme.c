#include "AetherVGUIScheme.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static f32 num(const char*s,f32 d){char*e=NULL;float v=strtof(s,&e);return(e&&e!=s)?v:d;}
void aether_vgui_scheme_init(aether_vgui_scheme_t*s){if(!s)return;memset(s,0,sizeof*s);s->fg=(aether_vgui_rgba_t){1,1,1,1};s->bg=(aether_vgui_rgba_t){0,0,0,1};s->accent=(aether_vgui_rgba_t){0.75f,0.75f,0.75f,1};}
aether_result_t aether_vgui_scheme_parse(aether_vgui_scheme_t*s,const char*t){if(!s||!t)return AETHER_ERR_INVALID_ARG;aether_vgui_scheme_init(s);aether_vgui_resource_t r;aether_vgui_resource_parse(&r,t);const char*v=aether_vgui_resource_get(&r,"FgColor",NULL);if(v){int x=0,y=0,z=0,a=255;sscanf(v,"%d %d %d %d",&x,&y,&z,&a);s->fg=(aether_vgui_rgba_t){x/255.f,y/255.f,z/255.f,a/255.f};}v=aether_vgui_resource_get(&r,"BgColor",NULL);if(v){int x=0,y=0,z=0,a=255;sscanf(v,"%d %d %d %d",&x,&y,&z,&a);s->bg=(aether_vgui_rgba_t){x/255.f,y/255.f,z/255.f,a/255.f};}v=aether_vgui_resource_get(&r,"AccentColor",NULL);if(v){int x=0,y=0,z=0,a=255;sscanf(v,"%d %d %d %d",&x,&y,&z,&a);s->accent=(aether_vgui_rgba_t){x/255.f,y/255.f,z/255.f,a/255.f};}(void)num;return AETHER_OK;}
const aether_vgui_font_t*aether_vgui_scheme_font(const aether_vgui_scheme_t*s,const char*n){if(!s||!n)return NULL;for(u32 i=0;i<s->font_count;i++)if(aether_str_eq(s->fonts[i].name,n))return &s->fonts[i];return NULL;}
