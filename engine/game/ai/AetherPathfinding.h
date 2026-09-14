#ifndef AETHER_PATHFINDING_H
#define AETHER_PATHFINDING_H
#include "../../core/AetherCore.h"
#include "../../core/AetherMath.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_PATH_MAX_NODES 1024
#define AETHER_PATH_MAX_NEIGHBORS 8
#define AETHER_PATH_MAX_RESULT 256
typedef struct { u32 id; aether_vec3_t pos; u32 neighbors[AETHER_PATH_MAX_NEIGHBORS]; u8 neighbor_count; bool active; } aether_path_node_t;
typedef struct { aether_path_node_t nodes[AETHER_PATH_MAX_NODES]; u32 count; } aether_path_graph_t;
void aether_path_graph_init(aether_path_graph_t *g);
i32 aether_path_add_node(aether_path_graph_t *g,aether_vec3_t pos);
bool aether_path_connect(aether_path_graph_t *g,u32 a,u32 b,bool bidirectional);
i32 aether_path_find(const aether_path_graph_t *g,u32 start,u32 goal,u32 *out_nodes,u32 max_nodes);
i32 aether_path_find_nearest(const aether_path_graph_t *g,aether_vec3_t pos,f32 max_distance);
#ifdef __cplusplus
}
#endif
#endif
