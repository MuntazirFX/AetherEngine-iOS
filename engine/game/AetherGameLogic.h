#ifndef AETHER_GAME_LOGIC_H
#define AETHER_GAME_LOGIC_H
#include "dll/AetherGameDLL.h"
#include "ai/AetherPathfinding.h"
#include "ai/AetherBot.h"
#include "../physics/AetherPhysics.h"
#include "../entity/AetherEntityClassRegistry.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { aether_dll_registry_t dll; aether_path_graph_t nav; aether_bot_manager_t bots; aether_entity_class_registry_t entities; } aether_game_logic_t;
void aether_game_logic_init(aether_game_logic_t*g);
void aether_game_logic_tick(aether_game_logic_t*g,f32 dt);
#ifdef __cplusplus
}
#endif
#endif
