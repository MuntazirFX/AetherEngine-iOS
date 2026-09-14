#include "AetherVGUIRuntime.h"
#include "AetherVGUIFont.h"
#include "../core/AetherCore.h"
#include <string.h>
static aether_vgui_t *g_ui;
static aether_vgui_scheme_t g_scheme;
static aether_vgui_localization_t g_loc;
static aether_vgui_server_browser_t g_servers;
static aether_vgui_console_t g_console;
static u32 g_main,g_options,g_load,g_multi,g_active;
static void noop(aether_vgui_panel_t*p,void*u){(void)p;(void)u;}
static void make_panel(aether_vgui_t*v,u32 parent,u32*id,const char*name,const char*title){*id=aether_vgui_add(v,AETHER_VGUI_PANEL,parent,name,title);}
aether_result_t aether_vgui_runtime_init(void){if(g_ui)return AETHER_OK;g_ui=aether_vgui_create();if(!g_ui)return AETHER_ERR_OUT_OF_MEM;aether_vgui_scheme_init(&g_scheme);aether_vgui_font_register(&g_scheme,"Default","Arial",18,false,false);aether_vgui_font_register(&g_scheme,"Title","Arial",28,true,false);aether_vgui_localization_init(&g_loc);aether_vgui_server_browser_init(&g_servers);aether_vgui_console_init(&g_console);make_panel(g_ui,0,&g_main,"MainMenu","Half-Life");make_panel(g_ui,0,&g_options,"Options","Options");make_panel(g_ui,0,&g_load,"LoadGame","Load Game");make_panel(g_ui,0,&g_multi,"Multiplayer","Multiplayer");const char*main_items[]={"NEW GAME","LOAD GAME","OPTIONS","MULTIPLAYER","QUIT"};for(u32 i=0;i<5;i++){u32 id=aether_vgui_add(g_ui,AETHER_VGUI_BUTTON,g_main,"MenuItem",main_items[i]);aether_vgui_set_action(g_ui,id,noop,NULL);aether_vgui_set_bounds(g_ui,id,0.12f,0.28f+(f32)i*0.09f,0.38f,0.07f);}u32 opt=aether_vgui_add(g_ui,AETHER_VGUI_LABEL,g_options,"Title","Options");aether_vgui_set_bounds(g_ui,opt,0.12f,0.18f,0.6f,0.08f);u32 vol=aether_vgui_add(g_ui,AETHER_VGUI_SLIDER,g_options,"Volume","Volume");aether_vgui_panel(g_ui,vol)->slider_min=0; aether_vgui_panel(g_ui,vol)->slider_max=1; aether_vgui_panel(g_ui,vol)->slider_value=1;aether_vgui_set_bounds(g_ui,vol,0.12f,0.34f,0.6f,0.07f);u32 load=aether_vgui_add(g_ui,AETHER_VGUI_LABEL,g_load,"Title","Select a saved game");aether_vgui_set_bounds(g_ui,load,0.12f,0.18f,0.7f,0.08f);u32 mp=aether_vgui_add(g_ui,AETHER_VGUI_LIST,g_multi,"ServerList","Internet Servers");aether_vgui_set_bounds(g_ui,mp,0.08f,0.18f,0.84f,0.58f);aether_vgui_runtime_show_main();aether_log(AETHER_LOG_INFO,"vgui","VGUI2/VGUI1 compatibility runtime initialized");return AETHER_OK;}
void aether_vgui_runtime_shutdown(void){if(g_ui){aether_vgui_destroy(g_ui);g_ui=NULL;}memset(&g_scheme,0,sizeof g_scheme);memset(&g_loc,0,sizeof g_loc);memset(&g_servers,0,sizeof g_servers);memset(&g_console,0,sizeof g_console);}
aether_vgui_t*aether_vgui_runtime_ui(void){return g_ui;} aether_vgui_scheme_t*aether_vgui_runtime_scheme(void){return &g_scheme;} aether_vgui_localization_t*aether_vgui_runtime_localization(void){return &g_loc;} aether_vgui_server_browser_t*aether_vgui_runtime_server_browser(void){return &g_servers;} aether_vgui_console_t*aether_vgui_runtime_console(void){return &g_console;}
static void show(u32 id){if(g_ui)aether_vgui_set_visible(g_ui, id, true);}
void aether_vgui_runtime_show_main(void){if(!g_ui)return;g_active=g_main;aether_vgui_set_visible(g_ui,g_main,true);aether_vgui_set_visible(g_ui,g_options,false);aether_vgui_set_visible(g_ui,g_load,false);aether_vgui_set_visible(g_ui,g_multi,false);}
void aether_vgui_runtime_show_options(void){if(!g_ui)return;g_active=g_options;aether_vgui_runtime_show_main();aether_vgui_set_visible(g_ui,g_main,false);show(g_options);}
void aether_vgui_runtime_show_load_game(void){if(!g_ui)return;g_active=g_load;aether_vgui_runtime_show_main();aether_vgui_set_visible(g_ui,g_main,false);show(g_load);}
void aether_vgui_runtime_show_multiplayer(void){if(!g_ui)return;g_active=g_multi;aether_vgui_runtime_show_main();aether_vgui_set_visible(g_ui,g_main,false);show(g_multi);}
void aether_vgui_runtime_toggle_console(void){g_console.visible=!g_console.visible;}
u32 aether_vgui_runtime_active_panel(void){return g_active;}
