#include "AetherVGUIConsole.h"
#include <string.h>
void aether_vgui_console_init(aether_vgui_console_t*c){if(c)memset(c,0,sizeof*c);}
void aether_vgui_console_write(aether_vgui_console_t*c,aether_log_level_t l,const char*t){if(!c||!t)return;if(c->count<AETHER_VGUI_CONSOLE_LINES){aether_vgui_console_line_t*x=&c->lines[c->count++];memset(x,0,sizeof*x);x->level=l;aether_str_copy(x->text,sizeof x->text,t);}else{memmove(&c->lines[0],&c->lines[1],(AETHER_VGUI_CONSOLE_LINES-1)*sizeof c->lines[0]);aether_vgui_console_line_t*x=&c->lines[AETHER_VGUI_CONSOLE_LINES-1];memset(x,0,sizeof*x);x->level=l;aether_str_copy(x->text,sizeof x->text,t);}c->scroll=0;}
aether_result_t aether_vgui_console_set_input(aether_vgui_console_t*c,const char*t){if(!c||!t)return AETHER_ERR_INVALID_ARG;aether_str_copy(c->input,sizeof c->input,t);return AETHER_OK;}
const aether_vgui_console_line_t*aether_vgui_console_line(const aether_vgui_console_t*c,u32 i){return(c&&i<c->count)?&c->lines[i]:NULL;}
