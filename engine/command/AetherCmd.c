#include "AetherCmd.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
struct aether_cmd_registry { aether_cmd_t items[AETHER_CMD_MAX]; u32 count; aether_cvar_registry_t *cvars; };
static aether_cmd_t*find(aether_cmd_registry_t*r,const char*n){if(!r||!n)return NULL;for(u32 i=0;i<r->count;++i)if(r->items[i].registered&&aether_str_eq(r->items[i].name,n))return&r->items[i];return NULL;}
aether_cmd_registry_t*aether_cmd_create(void){return calloc(1,sizeof(aether_cmd_registry_t));}
void aether_cmd_destroy(aether_cmd_registry_t*r){free(r);}
aether_cmd_t*aether_cmd_register(aether_cmd_registry_t*r,const char*n,aether_cmd_fn fn,void*u){if(!r||!n||!fn||r->count>=AETHER_CMD_MAX)return NULL;aether_cmd_t*old=find(r,n);if(old)return old;aether_cmd_t*c=&r->items[r->count++];memset(c,0,sizeof*c);aether_str_copy(c->name,sizeof c->name,n);c->fn=fn;c->user=u;c->registered=true;return c;}
aether_cmd_t*aether_cmd_find(aether_cmd_registry_t*r,const char*n){return find(r,n);}
static int tokenize(char*line,const char**argv){int argc=0;char*p=line;while(*p&&argc<AETHER_CMD_MAX_ARGS){while(isspace((unsigned char)*p))++p;if(!*p)break;char*start=p;bool quoted=false;if(*p=='"'){quoted=true;start=++p;}char*out=start;while(*p){if(quoted){if(*p=='"'){++p;break;}}else if(isspace((unsigned char)*p))break;if(*p=='\\'&&p[1])++p;*out++=*p++;}*out='\0';argv[argc++]=start;while(isspace((unsigned char)*p))++p;}return argc;}
static void cmd_set(const aether_cmd_context_t*c,void*u){aether_cvar_registry_t*r=u;if(c->argc<2){aether_log(AETHER_LOG_INFO,"console","usage: set <name> <value>");return;}aether_result_t z=aether_cvar_set(r,c->argv[1],c->argc>=3?c->argv[2]:"");if(z!=AETHER_OK)aether_log(AETHER_LOG_WARN,"console","cannot set %s (%s)",c->argv[1],aether_result_string(z));}
static void cmd_toggle(const aether_cmd_context_t*c,void*u){aether_cvar_registry_t*r=u;if(c->argc<2)return;bool v=aether_cvar_bool(r,c->argv[1],false);aether_cvar_set(r,c->argv[1],v?"0":"1");}
static void cmd_reset(const aether_cmd_context_t*c,void*u){aether_cvar_registry_t*r=u;if(c->argc<2)aether_cvar_reset_all(r);else aether_cvar_reset(r,c->argv[1]);}
static void cmd_cvarlist(const aether_cmd_context_t*c,void*u){(void)c;aether_cvar_registry_t*r=u;for(u32 i=0;i<aether_cvar_count(r);++i){const aether_cvar_t*v=aether_cvar_at(r,i);aether_log(AETHER_LOG_INFO,"cvar","%s = %s",v->name,v->string);}}
aether_result_t aether_cmd_execute(aether_cmd_registry_t*r,const char*line){if(!r||!line)return AETHER_ERR_INVALID_ARG;char buf[2048];aether_str_copy(buf,sizeof buf,line);const char*argv[AETHER_CMD_MAX_ARGS];int argc=tokenize(buf,argv);if(!argc)return AETHER_OK;aether_cmd_t*c=find(r,argv[0]);if(c){aether_cmd_context_t x={argc,{0}};for(int i=0;i<argc;++i)x.argv[i]=argv[i];c->fn(&x,c->user);return AETHER_OK;}if(r->cvars&&aether_cvar_find(r->cvars,argv[0])){if(argc==1)aether_log(AETHER_LOG_INFO,"console","%s = %s",argv[0],aether_cvar_string(r->cvars,argv[0]));else return aether_cvar_set(r->cvars,argv[0],argv[1]);}else aether_log(AETHER_LOG_WARN,"console","unknown command: %s",argv[0]);return AETHER_ERR_NOT_FOUND;}
void aether_cmd_list(const aether_cmd_registry_t*r){if(!r)return;for(u32 i=0;i<r->count;++i)aether_log(AETHER_LOG_INFO,"cmd","%s",r->items[i].name);}
u32 aether_cmd_count(const aether_cmd_registry_t*r){return r?r->count:0;}
const aether_cmd_t*aether_cmd_at(const aether_cmd_registry_t*r,u32 i){return(r&&i<r->count)?&r->items[i]:NULL;}
void aether_cmd_set_cvars(aether_cmd_registry_t*r,aether_cvar_registry_t*c){if(r)r->cvars=c;if(r&&c){aether_cmd_register(r,"set",cmd_set,c);aether_cmd_register(r,"toggle",cmd_toggle,c);aether_cmd_register(r,"reset",cmd_reset,c);aether_cmd_register(r,"cvarlist",cmd_cvarlist,c);}}
