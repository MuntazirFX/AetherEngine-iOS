/* AetherBSPVis.c — Leaf find + PVS stub + visible-face index cull.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherBSPVis.h"
#include <stdlib.h>
#include <string.h>

/* Match AetherCollision / GoldSrc leaf contents (avoid bsp→player include). */
#ifndef AETHER_CONTENTS_SOLID
#define AETHER_CONTENTS_SOLID (-2)
#endif

i32 aether_bsp_find_leaf(const aether_bsp_t *bsp, f32 x, f32 y, f32 z) {
    if (!bsp || !aether_bsp_is_valid(bsp)) return -1;
    u32 node_count = aether_bsp_node_count(bsp);
    u32 leaf_count = aether_bsp_leaf_count(bsp);
    if (node_count == 0 || leaf_count == 0) return -1;

    i32 node_idx = 0;
    /* Safety: avoid infinite loops on corrupt trees. */
    for (u32 step = 0; step < node_count + 2u; ++step) {
        if (node_idx < 0) {
            i32 leaf = -1 - node_idx;
            if (leaf < 0 || (u32)leaf >= leaf_count) return -1;
            return leaf;
        }
        if ((u32)node_idx >= node_count) return -1;
        const aether_bsp_node_t *node = aether_bsp_node_at(bsp, (u32)node_idx);
        if (!node) return -1;
        const aether_bsp_plane_t *plane = aether_bsp_plane_at(bsp, (u32)node->plane);
        f32 d = 0.f;
        if (plane) {
            d = plane->normal[0] * x + plane->normal[1] * y + plane->normal[2] * z - plane->dist;
        }
        node_idx = (d >= 0.f) ? (i32)node->children[0] : (i32)node->children[1];
    }
    return -1;
}

bool aether_bsp_leaf_is_drawable(const aether_bsp_t *bsp, i32 leaf_index) {
    if (!bsp || leaf_index < 0) return false;
    const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, (u32)leaf_index);
    if (!leaf) return false;
    /* Solid = not drawable. Everything else (empty/water/…) can contribute faces. */
    return leaf->contents != AETHER_CONTENTS_SOLID;
}

/* Mark leaf as visible in a bitset (1 bit per leaf). */
static void set_leaf_bit(u8 *bits, u32 leaf, u32 leaf_count) {
    if (leaf >= leaf_count) return;
    bits[leaf >> 3] |= (u8)(1u << (leaf & 7u));
}

static bool leaf_bit(const u8 *bits, u32 leaf) {
    return (bits[leaf >> 3] & (u8)(1u << (leaf & 7u))) != 0;
}

/* Decompress GoldSrc RLE PVS into leaf bitset. Returns false if truncated/invalid. */
static bool decompress_pvs(const u8 *vis, u32 vis_size, i32 offset,
                           u8 *out_bits, u32 row_bytes) {
    if (!vis || !out_bits || row_bytes == 0 || offset < 0 || (u32)offset >= vis_size) {
        return false;
    }
    memset(out_bits, 0, row_bytes);
    u32 in = (u32)offset;
    u32 out = 0;
    while (out < row_bytes && in < vis_size) {
        if (vis[in] == 0) {
            ++in;
            if (in >= vis_size) return false;
            u32 run = vis[in++];
            while (run-- > 0 && out < row_bytes) out_bits[out++] = 0;
        } else {
            out_bits[out++] = vis[in++];
        }
    }
    return out == row_bytes || out > 0;
}

/* Fill leaf visibility bits for the view leaf under the chosen mode. */
static void gather_visible_leaves(const aether_bsp_t *bsp,
                                  i32 view_leaf,
                                  aether_bsp_vis_mode_t mode,
                                  u8 *leaf_bits,
                                  u32 leaf_count,
                                  u32 *out_visible_leaves) {
    u32 row_bytes = (leaf_count + 7u) / 8u;
    memset(leaf_bits, 0, row_bytes);
    u32 visible = 0;

    if (mode == AETHER_BSP_VIS_FORCE_FULL || view_leaf < 0) {
        for (u32 i = 0; i < leaf_count; ++i) {
            if (aether_bsp_leaf_is_drawable(bsp, (i32)i)) {
                set_leaf_bit(leaf_bits, i, leaf_count);
                ++visible;
            }
        }
        /* Also allow solid leaf 0 faces if any (rare); FORCE_FULL uses faces directly. */
        *out_visible_leaves = visible;
        return;
    }

    if (mode == AETHER_BSP_VIS_CURRENT_LEAF_ONLY) {
        if (aether_bsp_leaf_is_drawable(bsp, view_leaf)) {
            set_leaf_bit(leaf_bits, (u32)view_leaf, leaf_count);
            visible = 1;
        }
        *out_visible_leaves = visible;
        return;
    }

    /* USE_PVS */
    const aether_bsp_leaf_t *vl = aether_bsp_leaf_at(bsp, (u32)view_leaf);
    const u8 *vis_lump = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_VISIBILITY);
    u32 vis_size = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_VISIBILITY);

    if (!vl || vl->vis_offset < 0 || !vis_lump || vis_size == 0) {
        /* Stub / no VIS: every drawable leaf is visible (tiny-room friendly). */
        for (u32 i = 0; i < leaf_count; ++i) {
            if (aether_bsp_leaf_is_drawable(bsp, (i32)i)) {
                set_leaf_bit(leaf_bits, i, leaf_count);
                ++visible;
            }
        }
        *out_visible_leaves = visible;
        return;
    }

    if (!decompress_pvs(vis_lump, vis_size, vl->vis_offset, leaf_bits, row_bytes)) {
        /* Corrupt → fall back to current leaf only (still a real skip path). */
        memset(leaf_bits, 0, row_bytes);
        if (aether_bsp_leaf_is_drawable(bsp, view_leaf)) {
            set_leaf_bit(leaf_bits, (u32)view_leaf, leaf_count);
            visible = 1;
        }
        *out_visible_leaves = visible;
        return;
    }

    /* Always include the view leaf itself. */
    if (aether_bsp_leaf_is_drawable(bsp, view_leaf)) {
        set_leaf_bit(leaf_bits, (u32)view_leaf, leaf_count);
    }
    for (u32 i = 0; i < leaf_count; ++i) {
        if (leaf_bit(leaf_bits, i) && aether_bsp_leaf_is_drawable(bsp, (i32)i)) {
            ++visible;
        } else if (leaf_bit(leaf_bits, i) && !aether_bsp_leaf_is_drawable(bsp, (i32)i)) {
            /* Clear solid bits if present. */
            leaf_bits[i >> 3] &= (u8)~(1u << (i & 7u));
        }
    }
    /* Re-count after solid clear. */
    visible = 0;
    for (u32 i = 0; i < leaf_count; ++i) {
        if (leaf_bit(leaf_bits, i)) ++visible;
    }
    *out_visible_leaves = visible;
}

u32 aether_bsp_vis_mark_faces(const aether_bsp_t *bsp,
                              i32 view_leaf,
                              aether_bsp_vis_mode_t mode,
                              u8 *out_bits,
                              u32 face_capacity) {
    if (!bsp || !out_bits || face_capacity == 0) return 0;
    u32 face_count = aether_bsp_face_count(bsp);
    if (face_count == 0) return 0;
    if (face_count > face_capacity) face_count = face_capacity;
    memset(out_bits, 0, face_count);

    if (mode == AETHER_BSP_VIS_FORCE_FULL) {
        for (u32 f = 0; f < face_count; ++f) out_bits[f] = 1;
        return face_count;
    }

    u32 leaf_count = aether_bsp_leaf_count(bsp);
    u32 mark_count = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_MARKSURFACES) / sizeof(u16);
    const u8 *mark_raw = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_MARKSURFACES);

    /* No leaf/marksurface data → cannot cull; keep full draw (API still exercised). */
    if (leaf_count == 0 || !mark_raw || mark_count == 0) {
        for (u32 f = 0; f < face_count; ++f) out_bits[f] = 1;
        return face_count;
    }

    u32 row_bytes = (leaf_count + 7u) / 8u;
    u8 *leaf_bits = (u8 *)calloc(1, row_bytes ? row_bytes : 1);
    if (!leaf_bits) {
        for (u32 f = 0; f < face_count; ++f) out_bits[f] = 1;
        return face_count;
    }

    u32 vis_leaves = 0;
    gather_visible_leaves(bsp, view_leaf, mode, leaf_bits, leaf_count, &vis_leaves);

    u32 visible_faces = 0;
    for (u32 li = 0; li < leaf_count; ++li) {
        if (!leaf_bit(leaf_bits, li)) continue;
        const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, li);
        if (!leaf || leaf->num_marksurfaces == 0) continue;
        u32 first = leaf->first_marksurface;
        for (u32 m = 0; m < leaf->num_marksurfaces; ++m) {
            u32 off = first + m;
            if (off >= mark_count) break;
            const u8 *p = mark_raw + off * 2u;
            u16 face = (u16)((u32)p[0] | ((u32)p[1] << 8));
            if (face < face_count && out_bits[face] == 0) {
                out_bits[face] = 1;
                ++visible_faces;
            }
        }
    }

    free(leaf_bits);
    return visible_faces;
}

u32 aether_bsp_vis_build_culled_indices(const aether_mesh_t *mesh,
                                        const u8 *face_bits,
                                        u32 face_bit_count,
                                        u32 *out_indices,
                                        u32 out_cap) {
    if (!mesh || !mesh->indices || !mesh->face_ranges || !face_bits || !out_indices || out_cap == 0) {
        return 0;
    }
    u32 written = 0;
    u32 nfaces = mesh->face_count;
    if (nfaces > face_bit_count) nfaces = face_bit_count;
    for (u32 f = 0; f < nfaces; ++f) {
        if (!face_bits[f]) continue;
        u32 first = mesh->face_ranges[f].first_index;
        u32 count = mesh->face_ranges[f].index_count;
        if (first + count > mesh->index_count) continue;
        for (u32 i = 0; i < count; ++i) {
            if (written >= out_cap) return written;
            out_indices[written++] = mesh->indices[first + i];
        }
    }
    return written;
}

u32 aether_bsp_vis_cull_mesh(const aether_bsp_t *bsp,
                             const aether_mesh_t *mesh,
                             f32 x, f32 y, f32 z,
                             aether_bsp_vis_mode_t mode,
                             u32 *out_indices,
                             u32 out_cap,
                             aether_bsp_vis_stats_t *out_stats) {
    aether_bsp_vis_stats_t stats;
    memset(&stats, 0, sizeof stats);
    stats.mode = mode;
    stats.force_full_vis = (mode == AETHER_BSP_VIS_FORCE_FULL);
    stats.view_leaf = -1;
    stats.total_faces = bsp ? aether_bsp_face_count(bsp) : 0;
    stats.total_indices = mesh ? mesh->index_count : 0;
    stats.leaf_count = bsp ? aether_bsp_leaf_count(bsp) : 0;

    if (bsp) {
        for (u32 i = 0; i < stats.leaf_count; ++i) {
            if (aether_bsp_leaf_is_drawable(bsp, (i32)i)) ++stats.empty_leaf_count;
        }
        stats.view_leaf = aether_bsp_find_leaf(bsp, x, y, z);
    }

    if (!bsp || !mesh || !out_indices || out_cap == 0 || stats.total_faces == 0) {
        if (out_stats) *out_stats = stats;
        return 0;
    }

    u8 *face_bits = (u8 *)calloc(1, stats.total_faces);
    if (!face_bits) {
        if (out_stats) *out_stats = stats;
        return 0;
    }

    stats.visible_faces = aether_bsp_vis_mark_faces(bsp, stats.view_leaf, mode,
                                                    face_bits, stats.total_faces);

    /* Count visible leaves for stats (re-run gather lightly). */
    if (mode == AETHER_BSP_VIS_FORCE_FULL) {
        stats.visible_leaf_count = stats.empty_leaf_count;
    } else if (mode == AETHER_BSP_VIS_CURRENT_LEAF_ONLY) {
        stats.visible_leaf_count =
            (stats.view_leaf >= 0 && aether_bsp_leaf_is_drawable(bsp, stats.view_leaf)) ? 1u : 0u;
    } else {
        /* USE_PVS with vis_offset < 0 → all empty (synthetic stub). */
        const aether_bsp_leaf_t *vl =
            (stats.view_leaf >= 0) ? aether_bsp_leaf_at(bsp, (u32)stats.view_leaf) : NULL;
        if (!vl || vl->vis_offset < 0) {
            stats.visible_leaf_count = stats.empty_leaf_count;
        } else {
            stats.visible_leaf_count = stats.empty_leaf_count; /* exact count optional */
        }
    }

    stats.visible_indices = aether_bsp_vis_build_culled_indices(
        mesh, face_bits, stats.total_faces, out_indices, out_cap);

    free(face_bits);
    if (out_stats) *out_stats = stats;
    return stats.visible_indices;
}
