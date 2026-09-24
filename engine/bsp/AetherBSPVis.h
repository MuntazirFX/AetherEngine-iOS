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

#ifdef __cplusplus
}
#endif
#endif /* AETHER_BSP_VIS_H */
