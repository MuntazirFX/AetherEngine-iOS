#include "AetherVGUIServerBrowser.h"
#include <string.h>
static void sw(aether_vgui_server_t*a,aether_vgui_server_t*b){aether_vgui_server_t t=*a;*a=*b;*b=t;}
void aether_vgui_server_browser_init(aether_vgui_server_browser_t*b){if(b){memset(b,0,sizeof*b);b->selected=-1;}}
aether_result_t aether_vgui_server_browser_add(aether_vgui_server_browser_t*b,const char*n,const char*a,const char*m,i32 p,i32 mp,i32 ping,bool pw){if(!b||!n||!a||b->count>=AETHER_VGUI_MAX_SERVERS)return AETHER_ERR_INVALID_ARG;aether_vgui_server_t*s=&b->servers[b->count++];memset(s,0,sizeof*s);aether_str_copy(s->name,sizeof s->name,n);aether_str_copy(s->address,sizeof s->address,a);aether_str_copy(s->map,sizeof s->map,m?m:"");s->players=p;s->max_players=mp;s->ping_ms=ping;s->password=pw;if(b->selected<0)b->selected=0;return AETHER_OK;}
void aether_vgui_server_browser_clear(aether_vgui_server_browser_t*b){if(b){u32 keep=0;memset(b,0,sizeof*b);b->selected=-1;(void)keep;}}
void aether_vgui_server_browser_sort_ping(aether_vgui_server_browser_t*b){if(!b)return;for(u32 i=0;i<b->count;i++)for(u32 j=i+1;j<b->count;j++)if(b->servers[j].ping_ms<b->servers[i].ping_ms)sw(&b->servers[i],&b->servers[j]);}
const aether_vgui_server_t*aether_vgui_server_browser_selected(const aether_vgui_server_browser_t*b){if(!b||b->selected<0||(u32)b->selected>=b->count)return NULL;return &b->servers[b->selected];}
