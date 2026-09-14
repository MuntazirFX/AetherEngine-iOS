#include "AetherVGUIFont.h"
#include <string.h>
aether_result_t aether_vgui_font_register(aether_vgui_scheme_t*s,const char*n,const char*face,i32 tall,bool bold,bool italic){if(!s||!n||!face||tall<=0)return AETHER_ERR_INVALID_ARG;if(s->font_count>=AETHER_VGUI_MAX_FONTS)return AETHER_ERR_OUT_OF_MEM;aether_vgui_font_t*f=&s->fonts[s->font_count++];memset(f,0,sizeof*f);aether_str_copy(f->name,sizeof f->name,n);aether_str_copy(f->face,sizeof f->face,face);f->tall=tall;f->bold=bold;f->italic=italic;return AETHER_OK;}
const aether_vgui_font_t*aether_vgui_font_find(const aether_vgui_scheme_t*s,const char*n){return aether_vgui_scheme_font(s,n);}
