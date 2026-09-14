#ifndef AETHER_BOT_H
#define AETHER_BOT_H
#include "../../core/AetherCore.h"
#include "../../core/AetherMath.h"
#include "AetherPathfinding.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_BOT_MAX 32
typedef enum { AETHER_BOT_IDLE=0,AETHER_BOT_ROAM,AETHER_BOT_SEEK,AETHER_BOT_ATTACK,AETHER_BOT_DEAD } aether_bot_state_t;
typedef struct { bool active; u32 id; char name[32]; aether_bot_state_t state; aether_vec3_t position; aether_vec3_t target_position; u32 target_node; f32 think_time; f32 attack_cooldown; i32 health; } aether_bot_t;
typedef struct { aether_bot_t bots[AETHER_BOT_MAX]; u32 count; } aether_bot_manager_t;
void aether_bot_manager_init(aether_bot_manager_t*m);
aether_bot_t *aether_bot_spawn(aether_bot_manager_t*m,const char*name,aether_vec3_t pos);
void aether_bot_set_target(aether_bot_t*b,aether_vec3_t pos);
void aether_bot_tick(aether_bot_manager_t*m,const aether_path_graph_t*g,f32 dt);
#ifdef __cplusplus
}
#endif
#endif
