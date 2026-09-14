#ifndef AETHER_VGUI_SERVER_BROWSER_H
#define AETHER_VGUI_SERVER_BROWSER_H
#include "../core/AetherCore.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_VGUI_MAX_SERVERS 128
typedef struct { char name[128]; char address[64]; char map[64]; i32 players; i32 max_players; i32 ping_ms; bool password; } aether_vgui_server_t;
typedef struct { aether_vgui_server_t servers[AETHER_VGUI_MAX_SERVERS]; u32 count; i32 selected; bool refreshing; } aether_vgui_server_browser_t;
void aether_vgui_server_browser_init(aether_vgui_server_browser_t*);
aether_result_t aether_vgui_server_browser_add(aether_vgui_server_browser_t*,const char*,const char*,const char*,i32,i32,i32,bool);
void aether_vgui_server_browser_clear(aether_vgui_server_browser_t*);
void aether_vgui_server_browser_sort_ping(aether_vgui_server_browser_t*);
const aether_vgui_server_t*aether_vgui_server_browser_selected(const aether_vgui_server_browser_t*);
#ifdef __cplusplus
}
#endif
#endif
