/* AetherBSPVis.h — Leaf find + PVS / marksurface face culling for BSP→mesh draw.
 * Synthetic maps get a usable stub (nodes/leaves/marksurfaces; vis_offset=-1 → all empty).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_BSP_VIS_H
#define AETHER_BSP_VIS_H

#include "AetherBSP.h"
#include "AetherBSPGeometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* How visible faces are gathered from leaf/PVS state. */
typedef enum aether_bsp_vis_mode {
    /* Decode PVS for view leaf (vis_offset < 0 → every empty leaf). */
    AETHER_BSP_VIS_USE_PVS = 0,
    /* Only marksurfaces of the current leaf (tight stub / debug). */
    AETHER_BSP_VIS_CURRENT_LEAF_ONLY = 1,
    /* Every face in the BSP (debug; ignore leaf/PVS). */
    AETHER_BSP_VIS_FORCE_FULL = 2
} aether_bsp_vis_mode_t;

typedef struct aether_bsp_vis_stats {
    i32  view_leaf;          /* leaf index from last point query (-1 if none) */
    u32  leaf_count;
    u32  empty_leaf_count;
    u32  visible_leaf_count;
    u32  total_faces;
    u32  visible_faces;
    u32  total_indices;      /* full mesh index count */
    u32  visible_indices;    /* culled index count */
    aether_bsp_vis_mode_t mode;
    bool force_full_vis;     /* bridge/debug override (acts as FORCE_FULL) */
} aether_bsp_vis_stats_t;

/* Walk the node tree; returns leaf index, or -1 on failure / empty tree.
 * GoldSrc: child < 0 → leaf = -1 - child. Leaf 0 is typically solid. */
i32 aether_bsp_find_leaf(const aether_bsp_t *bsp, f32 x, f32 y, f32 z);

/* True if leaf has CONTENTS_EMPTY (or water/slime/lava/sky) — not solid. */
bool aether_bsp_leaf_is_drawable(const aether_bsp_t *bsp, i32 leaf_index);

/* Fill out_bits[face] = 1 for faces belonging to visible leaves under `mode`.
 * out_bits length must be >= face_count. Returns number of visible faces. */
u32 aether_bsp_vis_mark_faces(const aether_bsp_t *bsp,
                              i32 view_leaf,
                              aether_bsp_vis_mode_t mode,
                              u8 *out_bits,
                              u32 face_capacity);

/* Build a compact index list from mesh face ranges ∩ visible face bits.
 * Writes up to out_cap indices into out_indices. Returns index count written. */
u32 aether_bsp_vis_build_culled_indices(const aether_mesh_t *mesh,
                                        const u8 *face_bits,
                                        u32 face_bit_count,
                                        u32 *out_indices,
                                        u32 out_cap);

/* One-shot: find leaf at point → mark faces → build culled indices + fill stats. */
u32 aether_bsp_vis_cull_mesh(const aether_bsp_t *bsp,
                             const aether_mesh_t *mesh,
                             f32 x, f32 y, f32 z,
                             aether_bsp_vis_mode_t mode,
                             u32 *out_indices,
                             u32 out_cap,
                             aether_bsp_vis_stats_t *out_stats);

/* Encode a GoldSrc-style RLE PVS row for `leaf_count` leaves.
 * `visible_bits` is a bitset (1 bit per leaf). Returns bytes written into out. */
u32 aether_bsp_vis_encode_pvs_row(const u8 *visible_bits, u32 leaf_count,
                                  u8 *out, u32 out_cap);

/* Frustum AABB cull: clear face_bits for faces whose owning leaf AABB is outside.
 * frustum may be NULL (no-op). Returns remaining visible face count. */
struct aether_frustum;
u32 aether_bsp_vis_apply_frustum(const aether_bsp_t *bsp,
                                 const struct aether_frustum *frustum,
                                 u8 *face_bits,
                                 u32 face_count);

#ifdef __cplusplus
}
#endif

/* ---------- Multi-portal leaf graph (adjacency from portals/leaves) + flood ---------- */
#define AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES  16
#define AETHER_BSP_PORTAL_GRAPH_MAX_EDGES   64
#define AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD   16

typedef struct aether_bsp_portal_edge {
    u16 leaf_a;
    u16 leaf_b;
    f32 center[3];   /* portal mid-point stub */
    f32 normal[3];
    bool valid;
} aether_bsp_portal_edge_t;

typedef struct aether_bsp_portal_graph {
    u32 leaf_count;
    u32 edge_count;
    u8  adj[AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES][AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES];
    aether_bsp_portal_edge_t edges[AETHER_BSP_PORTAL_GRAPH_MAX_EDGES];
    bool from_bsp;
    bool multi_portal; /* >=2 portal edges */
} aether_bsp_portal_graph_t;

typedef struct aether_bsp_portal_flood {
    u32 reached_count;
    u16 reached[AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD];
    u16 parent[AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD];
    u16 depth[AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD];
    u16 start_leaf;
    bool valid;
} aether_bsp_portal_flood_t;

void aether_bsp_portal_graph_init(aether_bsp_portal_graph_t *g);
/* Build adjacency from BSP leaves (PVS neighbors + shared node children). */
u32  aether_bsp_portal_graph_build_from_bsp(aether_bsp_portal_graph_t *g,
                                            const aether_bsp_t *bsp);
/* Synthetic multi-portal leaf graph (4 leaves, ring) for host smoke. */
u32  aether_bsp_portal_graph_build_multi_fixture(aether_bsp_portal_graph_t *g);
/* Add undirected edge; returns 1 if stored. */
int  aether_bsp_portal_graph_add_edge(aether_bsp_portal_graph_t *g,
                                      u16 a, u16 b,
                                      const f32 center[3], const f32 normal[3]);
/* BFS flood from start leaf through portal adjacency (for reflect reachability). */
u32  aether_bsp_portal_graph_flood(const aether_bsp_portal_graph_t *g,
                                   u16 start_leaf, u32 max_depth,
                                   aether_bsp_portal_flood_t *out);

#endif /* AETHER_BSP_VIS_H */
