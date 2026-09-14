#include "AetherGameLogic.h"
#include <string.h>
void aether_game_logic_init(aether_game_logic_t*g){if(!g)return;memset(g,0,sizeof*g);aether_dll_registry_init(&g->dll);aether_path_graph_init(&g->nav);aether_bot_manager_init(&g->bots);aether_entity_class_registry_init(&g->entities);aether_entity_class_register_builtin(&g->entities);}
void aether_game_logic_tick(aether_game_logic_t*g,f32 dt){if(!g)return;aether_dll_frame_all(&g->dll,dt);aether_bot_tick(&g->bots,&g->nav,dt);}
