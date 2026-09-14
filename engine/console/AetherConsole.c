#include "AetherConsole.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
struct aether_console { aether_cvar_registry_t*cvars; aether_cmd_registry_t*cmds; aether_console_sink_fn sink; void*user; };
static void log_sink(aether_log_level_t l,const char*t,const char*m,void*u){(void)l;(void)t;aether_console_t*c=u;if(c&&c->sink)c->sink(m,c->user);}
aether_console_t*aether_console_create(void){aether_console_t*c=calloc(1,sizeof*c);if(!c)return NULL;c->cvars=aether_cvar_create();c->cmds=aether_cmd_create();if(!c->cvars||!c->cmds){aether_console_destroy(c);return NULL;}aether_cmd_set_cvars(c->cmds,c->cvars);aether_console_register_defaults(c);return c;}
void aether_console_destroy(aether_console_t*c){if(!c)return;aether_cvar_destroy(c->cvars);aether_cmd_destroy(c->cmds);free(c);}
aether_cvar_registry_t*aether_console_cvars(aether_console_t*c){return c?c->cvars:NULL;}
aether_cmd_registry_t*aether_console_commands(aether_console_t*c){return c?c->cmds:NULL;}
void aether_console_set_sink(aether_console_t*c,aether_console_sink_fn fn,void*u){if(!c)return;c->sink=fn;c->user=u;aether_log_set(fn?log_sink:NULL,c);}
aether_result_t aether_console_execute(aether_console_t*c,const char*l){return(c&&l)?aether_cmd_execute(c->cmds,l):AETHER_ERR_INVALID_ARG;}
void aether_console_write(aether_console_t*c,aether_log_level_t level,const char*fmt,...){if(!c||!c->sink)return;char b[AETHER_CONSOLE_LINE_MAX];va_list ap;va_start(ap,fmt);vsnprintf(b,sizeof b,fmt,ap);va_end(ap);c->sink(b,c->user);(void)level;}
void aether_console_register_defaults(aether_console_t*c){if(!c)return;aether_cvar_register(c->cvars,"developer",AETHER_CVAR_INT,"0",AETHER_CVAR_ARCHIVE);aether_cvar_register(c->cvars,"fps_max",AETHER_CVAR_INT,"120",AETHER_CVAR_ARCHIVE);aether_cvar_register(c->cvars,"volume",AETHER_CVAR_FLOAT,"1.0",AETHER_CVAR_ARCHIVE);aether_cvar_register(c->cvars,"sv_cheats",AETHER_CVAR_BOOL,"0",0);aether_cvar_register(c->cvars,"name",AETHER_CVAR_STRING,"AetherPlayer",AETHER_CVAR_ARCHIVE|AETHER_CVAR_USERINFO);}
