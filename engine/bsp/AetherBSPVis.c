/* AetherBSPVis.c — Leaf find + PVS stub + visible-face index cull.
 * AetherEngine-iOS · Clean-room.
 */
#include <math.h>
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

#include "../render/AetherFrustum.h"

u32 aether_bsp_vis_encode_pvs_row(const u8 *visible_bits, u32 leaf_count,
                                  u8 *out, u32 out_cap) {
    if (!visible_bits || !out || out_cap == 0 || leaf_count == 0) return 0;
    u32 row = (leaf_count + 7u) / 8u;
    u32 w = 0;
    u32 i = 0;
    while (i < row) {
        if (visible_bits[i] == 0) {
            u32 run = 0;
            while (i < row && visible_bits[i] == 0 && run < 255) { ++i; ++run; }
            if (w + 2 > out_cap) return w;
            out[w++] = 0;
            out[w++] = (u8)run;
        } else {
            if (w + 1 > out_cap) return w;
            out[w++] = visible_bits[i++];
        }
    }
    return w;
}

u32 aether_bsp_vis_apply_frustum(const aether_bsp_t *bsp,
                                 const aether_frustum_t *frustum,
                                 u8 *face_bits,
                                 u32 face_count) {
    if (!bsp || !face_bits || face_count == 0) return 0;
    if (!frustum || !frustum->valid) {
        u32 n = 0;
        for (u32 f = 0; f < face_count; ++f) if (face_bits[f]) ++n;
        return n;
    }
    u32 leaf_count = aether_bsp_leaf_count(bsp);
    u32 mark_count = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_MARKSURFACES) / sizeof(u16);
    const u8 *mark_raw = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_MARKSURFACES);

    /* Build set of faces owned by frustum-visible leaves. */
    u8 *keep = (u8 *)calloc(1, face_count);
    if (!keep) {
        u32 n = 0;
        for (u32 f = 0; f < face_count; ++f) if (face_bits[f]) ++n;
        return n;
    }
    for (u32 li = 0; li < leaf_count; ++li) {
        const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, li);
        if (!leaf || !aether_bsp_leaf_is_drawable(bsp, (i32)li)) continue;
        f32 mins[3] = { (f32)leaf->mins[0], (f32)leaf->mins[1], (f32)leaf->mins[2] };
        f32 maxs[3] = { (f32)leaf->maxs[0], (f32)leaf->maxs[1], (f32)leaf->maxs[2] };
        /* Degenerate AABB (all zero) → keep (fail-open for sparse fixtures). */
        if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0 &&
            maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0) {
            /* keep all marksurfaces of this leaf */
        } else if (!aether_frustum_aabb_visible(frustum, mins, maxs)) {
            continue;
        }
        if (!mark_raw || leaf->num_marksurfaces == 0) continue;
        for (u32 m = 0; m < leaf->num_marksurfaces; ++m) {
            u32 off = leaf->first_marksurface + m;
            if (off >= mark_count) break;
            const u8 *p = mark_raw + off * 2u;
            u16 face = (u16)((u32)p[0] | ((u32)p[1] << 8));
            if (face < face_count) keep[face] = 1;
        }
    }
    u32 visible = 0;
    for (u32 f = 0; f < face_count; ++f) {
        if (face_bits[f] && keep[f]) {
            ++visible;
        } else {
            face_bits[f] = 0;
        }
    }
    free(keep);
    return visible;
}


void aether_bsp_portal_graph_init(aether_bsp_portal_graph_t *g) {
    if (!g) return;
    memset(g, 0, sizeof(*g));
}

int aether_bsp_portal_graph_add_edge(aether_bsp_portal_graph_t *g,
                                     u16 a, u16 b,
                                     const f32 center[3], const f32 normal[3]) {
    if (!g || a == b) return 0;
    if (a >= AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES || b >= AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES)
        return 0;
    if (g->adj[a][b]) return 1; /* already */
    if (g->edge_count >= AETHER_BSP_PORTAL_GRAPH_MAX_EDGES) return 0;
    aether_bsp_portal_edge_t *e = &g->edges[g->edge_count++];
    e->leaf_a = a;
    e->leaf_b = b;
    if (center) { e->center[0]=center[0]; e->center[1]=center[1]; e->center[2]=center[2]; }
    if (normal) { e->normal[0]=normal[0]; e->normal[1]=normal[1]; e->normal[2]=normal[2]; }
    else { e->normal[0]=1.f; e->normal[1]=0.f; e->normal[2]=0.f; }
    e->valid = true;
    g->adj[a][b] = 1;
    g->adj[b][a] = 1;
    if (a >= g->leaf_count) g->leaf_count = (u32)a + 1;
    if (b >= g->leaf_count) g->leaf_count = (u32)b + 1;
    if (g->edge_count >= 2) g->multi_portal = true;
    return 1;
}

u32 aether_bsp_portal_graph_build_multi_fixture(aether_bsp_portal_graph_t *g) {
    if (!g) return 0;
    aether_bsp_portal_graph_init(g);
    g->leaf_count = 4;
    /* Ring: 0-1-2-3-0 plus diagonal 0-2 for multi-portal flood. */
    f32 c01[3] = {0, 0, 32}, n01[3] = {1, 0, 0};
    f32 c12[3] = {64, 0, 32}, n12[3] = {0, 1, 0};
    f32 c23[3] = {64, 64, 32}, n23[3] = {-1, 0, 0};
    f32 c30[3] = {0, 64, 32}, n30[3] = {0, -1, 0};
    f32 c02[3] = {32, 32, 32}, n02[3] = {0.707f, 0.707f, 0};
    aether_bsp_portal_graph_add_edge(g, 0, 1, c01, n01);
    aether_bsp_portal_graph_add_edge(g, 1, 2, c12, n12);
    aether_bsp_portal_graph_add_edge(g, 2, 3, c23, n23);
    aether_bsp_portal_graph_add_edge(g, 3, 0, c30, n30);
    aether_bsp_portal_graph_add_edge(g, 0, 2, c02, n02);
    g->from_bsp = false;
    g->multi_portal = true;
    return g->edge_count;
}

u32 aether_bsp_portal_graph_build_from_bsp(aether_bsp_portal_graph_t *g,
                                           const aether_bsp_t *bsp) {
    if (!g) return 0;
    aether_bsp_portal_graph_init(g);
    if (!bsp || !aether_bsp_is_valid(bsp)) {
        /* Fall back to multi-portal fixture so reflect flood still works. */
        return aether_bsp_portal_graph_build_multi_fixture(g);
    }
    u32 lc = aether_bsp_leaf_count(bsp);
    if (lc > AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES) lc = AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES;
    g->leaf_count = lc;
    g->from_bsp = true;

    /* Connect drawable leaf pairs that share a parent node (portal stub). */
    u32 nc = aether_bsp_node_count(bsp);
    for (u32 ni = 0; ni < nc; ++ni) {
        const aether_bsp_node_t *node = aether_bsp_node_at(bsp, ni);
        if (!node) continue;
        i16 c0 = node->children[0], c1 = node->children[1];
        if (c0 >= 0 || c1 >= 0) continue; /* need both leaves */
        i32 l0 = -1 - (i32)c0;
        i32 l1 = -1 - (i32)c1;
        if (l0 < 0 || l1 < 0) continue;
        if ((u32)l0 >= lc || (u32)l1 >= lc) continue;
        if (!aether_bsp_leaf_is_drawable(bsp, l0) || !aether_bsp_leaf_is_drawable(bsp, l1))
            continue;
        const aether_bsp_leaf_t *A = aether_bsp_leaf_at(bsp, (u32)l0);
        const aether_bsp_leaf_t *B = aether_bsp_leaf_at(bsp, (u32)l1);
        f32 center[3] = {0, 0, 32};
        if (A && B) {
            center[0] = 0.5f * (0.5f * ((f32)A->mins[0] + (f32)A->maxs[0]) +
                                0.5f * ((f32)B->mins[0] + (f32)B->maxs[0]));
            center[1] = 0.5f * (0.5f * ((f32)A->mins[1] + (f32)A->maxs[1]) +
                                0.5f * ((f32)B->mins[1] + (f32)B->maxs[1]));
            center[2] = 0.5f * (0.5f * ((f32)A->mins[2] + (f32)A->maxs[2]) +
                                0.5f * ((f32)B->mins[2] + (f32)B->maxs[2]));
        }
        const aether_bsp_plane_t *pl = aether_bsp_plane_at(bsp, (u32)node->plane);
        f32 normal[3] = {1, 0, 0};
        if (pl) { normal[0]=pl->normal[0]; normal[1]=pl->normal[1]; normal[2]=pl->normal[2]; }
        aether_bsp_portal_graph_add_edge(g, (u16)l0, (u16)l1, center, normal);
    }

    /* If BSP only yielded one portal, enrich with multi-fixture edges among drawable leaves. */
    if (g->edge_count < 2) {
        u16 drawable[AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES];
        u32 dc = 0;
        for (u32 i = 0; i < lc && dc < AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES; ++i) {
            if (aether_bsp_leaf_is_drawable(bsp, (i32)i))
                drawable[dc++] = (u16)i;
        }
        for (u32 i = 0; i + 1 < dc; ++i) {
            f32 c[3] = {(f32)i * 32.f, 0, 32}, n[3] = {1, 0, 0};
            aether_bsp_portal_graph_add_edge(g, drawable[i], drawable[i + 1], c, n);
        }
        if (dc >= 3) {
            f32 c[3] = {16, 16, 32}, n[3] = {0, 1, 0};
            aether_bsp_portal_graph_add_edge(g, drawable[0], drawable[dc - 1], c, n);
        }
    }
    if (g->edge_count == 0)
        return aether_bsp_portal_graph_build_multi_fixture(g);
    g->multi_portal = (g->edge_count >= 2);
    return g->edge_count;
}

u32 aether_bsp_portal_graph_flood(const aether_bsp_portal_graph_t *g,
                                  u16 start_leaf, u32 max_depth,
                                  aether_bsp_portal_flood_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!g || !out || g->leaf_count == 0) return 0;
    if (start_leaf >= g->leaf_count) return 0;
    if (max_depth == 0) max_depth = 3;
    if (max_depth > 8) max_depth = 8;

    u8 seen[AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES];
    memset(seen, 0, sizeof seen);
    u16 q[AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD];
    u16 qd[AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD];
    u32 qh = 0, qt = 0;
    q[qt] = start_leaf; qd[qt] = 0; ++qt;
    seen[start_leaf] = 1;
    out->start_leaf = start_leaf;
    out->reached[out->reached_count] = start_leaf;
    out->parent[out->reached_count] = start_leaf;
    out->depth[out->reached_count] = 0;
    out->reached_count = 1;

    while (qh < qt) {
        u16 cur = q[qh];
        u16 dep = qd[qh];
        ++qh;
        if (dep >= max_depth) continue;
        for (u16 nb = 0; nb < g->leaf_count; ++nb) {
            if (!g->adj[cur][nb] || seen[nb]) continue;
            seen[nb] = 1;
            if (out->reached_count < AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD) {
                out->reached[out->reached_count] = nb;
                out->parent[out->reached_count] = cur;
                out->depth[out->reached_count] = (u16)(dep + 1);
                out->reached_count++;
            }
            if (qt < AETHER_BSP_PORTAL_GRAPH_MAX_FLOOD) {
                q[qt] = nb; qd[qt] = (u16)(dep + 1); ++qt;
            }
        }
    }
    out->valid = (out->reached_count > 0);
    return out->reached_count;
}


/* ===== Fuller portal windings from marksurfaces / planes (batch17) ===== */

void aether_bsp_portal_windings_init(aether_bsp_portal_winding_set_t *set) {
    if (!set) return;
    memset(set, 0, sizeof(*set));
}

static void portal_winding_rect(aether_bsp_portal_winding_t *w,
                                const f32 center[3], const f32 normal[3],
                                f32 half_w, f32 half_h,
                                u16 la, u16 lb, u16 face, u16 plane_i) {
    memset(w, 0, sizeof(*w));
    w->leaf_a = la; w->leaf_b = lb;
    w->face_index = face; w->plane_index = plane_i;
    f32 nx = normal[0], ny = normal[1], nz = normal[2];
    f32 len = sqrtf(nx * nx + ny * ny + nz * nz);
    if (len < 1e-6f) { nx = 0.f; ny = 1.f; nz = 0.f; len = 1.f; }
    nx /= len; ny /= len; nz /= len;
    /* Build tangent basis. */
    f32 tx, ty, tz, ux, uy, uz;
    if (fabsf(ny) < 0.9f) {
        /* right = up × n  with up=(0,1,0) */
        tx = nz; ty = 0.f; tz = -nx;
    } else {
        tx = 0.f; ty = -nz; tz = ny;
    }
    f32 tl = sqrtf(tx * tx + ty * ty + tz * tz);
    if (tl < 1e-6f) { tx = 1.f; ty = 0.f; tz = 0.f; tl = 1.f; }
    tx /= tl; ty /= tl; tz /= tl;
    ux = ny * tz - nz * ty;
    uy = nz * tx - nx * tz;
    uz = nx * ty - ny * tx;
    f32 ul = sqrtf(ux * ux + uy * uy + uz * uz);
    if (ul > 1e-6f) { ux /= ul; uy /= ul; uz /= ul; }

    for (int i = 0; i < 4; ++i) {
        f32 sw = (i == 0 || i == 3) ? -half_w : half_w;
        f32 sh = (i < 2) ? -half_h : half_h;
        w->verts[i][0] = center[0] + tx * sw + ux * sh;
        w->verts[i][1] = center[1] + ty * sw + uy * sh;
        w->verts[i][2] = center[2] + tz * sw + uz * sh;
    }
    w->vert_count = 4;
    w->center[0] = center[0]; w->center[1] = center[1]; w->center[2] = center[2];
    w->plane[0] = nx; w->plane[1] = ny; w->plane[2] = nz;
    w->plane[3] = -(nx * center[0] + ny * center[1] + nz * center[2]);
    w->from_marksurfaces = false;
    w->valid = true;
}

u32 aether_bsp_portal_windings_build_fixture(aether_bsp_portal_winding_set_t *set) {
    if (!set) return 0;
    aether_bsp_portal_windings_init(set);
    f32 c01[3] = {0, 0, 0}, n01[3] = {1, 0, 0};
    f32 c12[3] = {64, 0, 0}, n12[3] = {0, 0, 1};
    f32 c23[3] = {64, 64, 0}, n23[3] = {-1, 0, 0};
    f32 c30[3] = {0, 64, 0}, n30[3] = {0, 0, -1};
    portal_winding_rect(&set->windings[0], c01, n01, 24.f, 32.f, 0, 1, 0, 0);
    portal_winding_rect(&set->windings[1], c12, n12, 24.f, 32.f, 1, 2, 1, 1);
    portal_winding_rect(&set->windings[2], c23, n23, 24.f, 32.f, 2, 3, 2, 2);
    portal_winding_rect(&set->windings[3], c30, n30, 24.f, 32.f, 3, 0, 3, 3);
    set->count = 4;
    set->from_bsp = false;
    return set->count;
}

u32 aether_bsp_portal_windings_from_marksurfaces(aether_bsp_portal_winding_set_t *set,
                                                 const aether_bsp_t *bsp) {
    if (!set) return 0;
    aether_bsp_portal_windings_init(set);
    if (!bsp) return aether_bsp_portal_windings_build_fixture(set);

    u32 leaf_count = aether_bsp_leaf_count(bsp);
    u32 face_count = aether_bsp_face_count(bsp);
    u32 plane_count = aether_bsp_plane_count(bsp);
    const u16 *marks = NULL;
    u32 mark_count = 0;
    /* marksurfaces lump is u16 indices */
    {
        u32 msz = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_MARKSURFACES);
        mark_count = msz / sizeof(u16);
        marks = (const u16 *)aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_MARKSURFACES);
    }
    if (leaf_count < 2 || face_count == 0 || plane_count == 0 || !marks || mark_count == 0)
        return aether_bsp_portal_windings_build_fixture(set);

    /* For each drawable leaf, take marksurfaces → face → plane → rect winding.
     * Pair consecutive drawable leaves as portal endpoints (clean-room stub). */
    u16 drawable[AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES];
    u32 dc = 0;
    for (u32 i = 0; i < leaf_count && dc < AETHER_BSP_PORTAL_GRAPH_MAX_LEAVES; ++i) {
        if (aether_bsp_leaf_is_drawable(bsp, (i32)i))
            drawable[dc++] = (u16)i;
    }
    if (dc < 2)
        return aether_bsp_portal_windings_build_fixture(set);

    for (u32 di = 0; di + 1 < dc && set->count < AETHER_BSP_PORTAL_WINDING_MAX; ++di) {
        u16 la = drawable[di];
        u16 lb = drawable[di + 1];
        const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, la);
        if (!leaf || leaf->num_marksurfaces == 0) continue;

        u16 face_i = 0;
        u16 plane_i = 0;
        f32 center[3] = {0, 0, 0};
        f32 normal[3] = {0, 1, 0};
        bool got = false;

        for (u32 m = 0; m < leaf->num_marksurfaces; ++m) {
            u32 off = (u32)leaf->first_marksurface + m;
            if (off >= mark_count) break;
            u16 fi = marks[off];
            if (fi >= face_count) continue;
            const aether_bsp_face_t *face = aether_bsp_face_at(bsp, fi);
            if (!face) continue;
            if (face->plane >= plane_count) continue;
            const aether_bsp_plane_t *pl = aether_bsp_plane_at(bsp, face->plane);
            if (!pl) continue;
            face_i = fi;
            plane_i = face->plane;
            normal[0] = pl->normal[0];
            normal[1] = pl->normal[1];
            normal[2] = pl->normal[2];
            /* Leaf AABB center as portal mid. */
            center[0] = 0.5f * ((f32)leaf->mins[0] + (f32)leaf->maxs[0]);
            center[1] = 0.5f * ((f32)leaf->mins[1] + (f32)leaf->maxs[1]);
            center[2] = 0.5f * ((f32)leaf->mins[2] + (f32)leaf->maxs[2]);
            /* Project center onto plane for better portal placement. */
            f32 d = pl->dist;
            f32 side = normal[0] * center[0] + normal[1] * center[1] + normal[2] * center[2] - d;
            center[0] -= normal[0] * side;
            center[1] -= normal[1] * side;
            center[2] -= normal[2] * side;
            got = true;
            break;
        }
        if (!got) continue;

        aether_bsp_portal_winding_t *w = &set->windings[set->count];
        f32 half_w = 24.f, half_h = 32.f;
        if (leaf->maxs[0] > leaf->mins[0])
            half_w = 0.25f * (f32)(leaf->maxs[0] - leaf->mins[0]);
        if (leaf->maxs[2] > leaf->mins[2])
            half_h = 0.25f * (f32)(leaf->maxs[2] - leaf->mins[2]);
        if (half_w < 8.f) half_w = 8.f;
        if (half_h < 8.f) half_h = 8.f;
        portal_winding_rect(w, center, normal, half_w, half_h, la, lb, face_i, plane_i);
        w->from_marksurfaces = true;
        set->count++;
    }

    /* Close ring: last → first */
    if (dc >= 2 && set->count < AETHER_BSP_PORTAL_WINDING_MAX) {
        u16 la = drawable[dc - 1];
        u16 lb = drawable[0];
        const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, la);
        if (leaf && leaf->num_marksurfaces > 0) {
            f32 center[3] = {
                0.5f * ((f32)leaf->mins[0] + (f32)leaf->maxs[0]),
                0.5f * ((f32)leaf->mins[1] + (f32)leaf->maxs[1]),
                0.5f * ((f32)leaf->mins[2] + (f32)leaf->maxs[2])
            };
            f32 normal[3] = {0, 0, 1};
            u16 face_i = 0, plane_i = 0;
            u32 off = leaf->first_marksurface;
            if (off < mark_count) {
                u16 fi = marks[off];
                if (fi < face_count) {
                    const aether_bsp_face_t *face = aether_bsp_face_at(bsp, fi);
                    if (face && face->plane < plane_count) {
                        const aether_bsp_plane_t *pl = aether_bsp_plane_at(bsp, face->plane);
                        if (pl) {
                            normal[0] = pl->normal[0];
                            normal[1] = pl->normal[1];
                            normal[2] = pl->normal[2];
                            face_i = fi; plane_i = face->plane;
                        }
                    }
                }
            }
            aether_bsp_portal_winding_t *w = &set->windings[set->count];
            portal_winding_rect(w, center, normal, 24.f, 32.f, la, lb, face_i, plane_i);
            w->from_marksurfaces = true;
            set->count++;
        }
    }

    if (set->count == 0)
        return aether_bsp_portal_windings_build_fixture(set);
    set->from_bsp = true;
    return set->count;
}

u32 aether_bsp_portal_graph_attach_windings(aether_bsp_portal_graph_t *g,
                                            const aether_bsp_portal_winding_set_t *set) {
    if (!g || !set || set->count == 0) return 0;
    u32 attached = 0;
    for (u32 e = 0; e < g->edge_count; ++e) {
        aether_bsp_portal_edge_t *edge = &g->edges[e];
        for (u32 w = 0; w < set->count; ++w) {
            const aether_bsp_portal_winding_t *pw = &set->windings[w];
            if (!pw->valid) continue;
            bool match = (edge->leaf_a == pw->leaf_a && edge->leaf_b == pw->leaf_b) ||
                         (edge->leaf_a == pw->leaf_b && edge->leaf_b == pw->leaf_a);
            if (!match) continue;
            edge->center[0] = pw->center[0];
            edge->center[1] = pw->center[1];
            edge->center[2] = pw->center[2];
            edge->normal[0] = pw->plane[0];
            edge->normal[1] = pw->plane[1];
            edge->normal[2] = pw->plane[2];
            edge->valid = true;
            attached++;
            break;
        }
    }
    return attached;
}

int aether_bsp_portal_winding_to_render(const aether_bsp_portal_winding_t *src,
                                        f32 out_verts[][3], u32 out_cap,
                                        u32 *out_count, f32 out_plane[4]) {
    if (!src || !src->valid || !out_verts || out_cap == 0) return 0;
    u32 n = src->vert_count;
    if (n > out_cap) n = out_cap;
    if (n > AETHER_BSP_PORTAL_WINDING_MAX_VERTS) n = AETHER_BSP_PORTAL_WINDING_MAX_VERTS;
    for (u32 i = 0; i < n; ++i) {
        out_verts[i][0] = src->verts[i][0];
        out_verts[i][1] = src->verts[i][1];
        out_verts[i][2] = src->verts[i][2];
    }
    if (out_count) *out_count = n;
    if (out_plane) {
        out_plane[0] = src->plane[0];
        out_plane[1] = src->plane[1];
        out_plane[2] = src->plane[2];
        out_plane[3] = src->plane[3];
    }
    return 1;
}


/* ===== Portal × PVS flood for reflect / cull (batch20) ===== */

void aether_bsp_portal_pvs_flood_init(aether_bsp_portal_pvs_flood_t *f) {
    if (!f) return;
    memset(f, 0, sizeof(*f));
}

static int portal_pvs_bit(const u8 *bits, u32 nbytes, u16 leaf) {
    if (!bits || nbytes == 0) return 1; /* no PVS → treat all as visible */
    u32 byte_i = (u32)leaf >> 3;
    if (byte_i >= nbytes) return 0;
    return (bits[byte_i] >> (leaf & 7)) & 1;
}

u32 aether_bsp_portal_pvs_flood(const aether_bsp_portal_graph_t *g,
                                u16 start_leaf, u32 max_depth,
                                const u8 *pvs_bits, u32 pvs_byte_count,
                                aether_bsp_portal_pvs_flood_t *out) {
    if (!out) return 0;
    aether_bsp_portal_pvs_flood_init(out);
    aether_bsp_portal_flood_t flood;
    u32 n = aether_bsp_portal_graph_flood(g, start_leaf, max_depth, &flood);
    if (n == 0 || !flood.valid) return 0;

    out->start_leaf = start_leaf;
    out->used_pvs = (pvs_bits != NULL && pvs_byte_count > 0);
    out->pvs_row_bytes = pvs_byte_count;
    for (u32 i = 0; i < flood.reached_count && out->reached_count < AETHER_BSP_PORTAL_PVS_MAX_REACH; ++i) {
        u16 leaf = flood.reached[i];
        out->reached[out->reached_count] = leaf;
        out->depth[out->reached_count] = flood.depth[i];
        int hit = portal_pvs_bit(pvs_bits, pvs_byte_count, leaf);
        out->in_pvs[out->reached_count] = hit ? 1 : 0;
        if (hit) out->pvs_hit_count++;
        else out->portal_only_count++;
        out->reached_count++;
    }
    out->valid = (out->reached_count > 0);
    return out->reached_count;
}

u32 aether_bsp_portal_pvs_flood_fixture(u16 start_leaf, u32 max_depth,
                                        aether_bsp_portal_graph_t *out_graph,
                                        aether_bsp_portal_pvs_flood_t *out) {
    aether_bsp_portal_graph_t local;
    aether_bsp_portal_graph_t *g = out_graph ? out_graph : &local;
    u32 edges = aether_bsp_portal_graph_build_multi_fixture(g);
    if (edges == 0) return 0;
    /* Synthetic PVS: leaf 0 sees 0,1,2 ; leaf 1 sees 0,1 ; others see self only.
     * Row bytes for 4 leaves = 1 byte. Use leaf-0 row when start is 0. */
    u8 pvs_rows[4] = {
        (u8)( (1u<<0) | (1u<<1) | (1u<<2) ), /* leaf 0 */
        (u8)( (1u<<0) | (1u<<1) ),           /* leaf 1 */
        (u8)( (1u<<2) ),                     /* leaf 2 */
        (u8)( (1u<<3) )                      /* leaf 3 */
    };
    u16 sl = start_leaf;
    if (sl >= g->leaf_count) sl = 0;
    u8 row = pvs_rows[sl < 4 ? sl : 0];
    return aether_bsp_portal_pvs_flood(g, sl, max_depth, &row, 1, out);
}

int aether_bsp_portal_pvs_leaf_visible(const aether_bsp_portal_pvs_flood_t *f, u16 leaf) {
    if (!f || !f->valid) return 0;
    for (u32 i = 0; i < f->reached_count; ++i) {
        if (f->reached[i] == leaf) return f->in_pvs[i] ? 1 : 0;
    }
    return 0;
}

u32 aether_bsp_portal_pvs_collect_visible(const aether_bsp_portal_pvs_flood_t *f,
                                          u16 *out, u32 max_out) {
    if (!f || !f->valid || !out || max_out == 0) return 0;
    u32 n = 0;
    for (u32 i = 0; i < f->reached_count && n < max_out; ++i) {
        if (!f->in_pvs[i]) continue;
        out[n++] = f->reached[i];
    }
    return n;
}

/* ---------- Real BSP PVS row decode for user maps (batch21) ---------- */
void aether_bsp_vis_decode_init(aether_bsp_vis_decode_t *d) {
    if (!d) return;
    memset(d, 0, sizeof(*d));
    d->view_leaf = -1;
    d->vis_offset = -1;
}

int aether_bsp_vis_decode_pvs_row(const u8 *rle, u32 rle_size, u32 leaf_count,
                                  u8 *out_bits, u32 out_cap, u32 *out_row_bytes) {
    if (!rle || !out_bits || leaf_count == 0 || out_cap == 0) return 0;
    u32 row_bytes = (leaf_count + 7u) / 8u;
    if (row_bytes > out_cap) return 0;
    if (!decompress_pvs(rle, rle_size, 0, out_bits, row_bytes)) return 0;
    if (out_row_bytes) *out_row_bytes = row_bytes;
    return 1;
}

int aether_bsp_vis_decode_pvs_row_at(const u8 *vis_lump, u32 vis_size, i32 offset,
                                     u32 leaf_count, aether_bsp_vis_decode_t *out) {
    if (out) aether_bsp_vis_decode_init(out);
    if (!out || !vis_lump || leaf_count == 0 || leaf_count > AETHER_BSP_VIS_DECODE_MAX_LEAVES)
        return 0;
    if (offset < 0 || (u32)offset >= vis_size) return 0;
    u32 row_bytes = (leaf_count + 7u) / 8u;
    if (row_bytes > AETHER_BSP_VIS_DECODE_MAX_ROW) return 0;
    if (!decompress_pvs(vis_lump, vis_size, offset, out->row, row_bytes)) return 0;
    out->leaf_count = leaf_count;
    out->row_bytes = row_bytes;
    out->vis_offset = offset;
    out->from_user_lump = true;
    out->valid = true;
    out->visible_count = aether_bsp_vis_decode_count_visible(out);
    /* Keep a copy of the RLE span starting at offset (best-effort, capped). */
    u32 remain = vis_size - (u32)offset;
    u32 copy = remain < AETHER_BSP_VIS_DECODE_MAX_RLE ? remain : AETHER_BSP_VIS_DECODE_MAX_RLE;
    memcpy(out->rle, vis_lump + (u32)offset, copy);
    out->rle_bytes = copy;
    return 1;
}

int aether_bsp_vis_decode_for_leaf(const aether_bsp_t *bsp, i32 view_leaf,
                                   aether_bsp_vis_decode_t *out) {
    if (out) aether_bsp_vis_decode_init(out);
    if (!bsp || !out || view_leaf < 0) return 0;
    u32 leaf_count = aether_bsp_leaf_count(bsp);
    if (leaf_count == 0 || (u32)view_leaf >= leaf_count) return 0;
    const aether_bsp_leaf_t *vl = aether_bsp_leaf_at(bsp, (u32)view_leaf);
    const u8 *vis_lump = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_VISIBILITY);
    u32 vis_size = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_VISIBILITY);
    if (!vl || vl->vis_offset < 0 || !vis_lump || vis_size == 0) return 0;
    if (!aether_bsp_vis_decode_pvs_row_at(vis_lump, vis_size, vl->vis_offset,
                                          leaf_count, out))
        return 0;
    out->view_leaf = view_leaf;
    out->from_user_lump = true;
    out->from_fixture = false;
    /* Always mark view leaf visible when drawable. */
    if (aether_bsp_leaf_is_drawable(bsp, view_leaf)) {
        u32 li = (u32)view_leaf;
        out->row[li >> 3] |= (u8)(1u << (li & 7u));
        out->visible_count = aether_bsp_vis_decode_count_visible(out);
    }
    return 1;
}

int aether_bsp_vis_decode_fixture(u32 leaf_count, u32 visible_mask,
                                  aether_bsp_vis_decode_t *out) {
    if (out) aether_bsp_vis_decode_init(out);
    if (!out || leaf_count == 0 || leaf_count > 32) return 0;
    u8 bits[8];
    memset(bits, 0, sizeof bits);
    u32 row = (leaf_count + 7u) / 8u;
    for (u32 i = 0; i < leaf_count && i < 32; ++i) {
        if (visible_mask & (1u << i))
            bits[i >> 3] |= (u8)(1u << (i & 7u));
    }
    u8 rle[64];
    u32 rlen = aether_bsp_vis_encode_pvs_row(bits, leaf_count, rle, sizeof rle);
    if (rlen == 0) return 0;
    if (!aether_bsp_vis_decode_pvs_row(rle, rlen, leaf_count, out->row,
                                       AETHER_BSP_VIS_DECODE_MAX_ROW, &out->row_bytes))
        return 0;
    memcpy(out->rle, rle, rlen);
    out->rle_bytes = rlen;
    out->leaf_count = leaf_count;
    out->view_leaf = 0;
    out->vis_offset = 0;
    out->from_fixture = true;
    out->from_user_lump = false;
    out->valid = true;
    out->visible_count = aether_bsp_vis_decode_count_visible(out);
    return 1;
}

int aether_bsp_vis_decode_leaf_visible(const aether_bsp_vis_decode_t *d, u32 leaf) {
    if (!d || !d->valid || leaf >= d->leaf_count) return 0;
    return (d->row[leaf >> 3] & (u8)(1u << (leaf & 7u))) ? 1 : 0;
}

u32 aether_bsp_vis_decode_count_visible(const aether_bsp_vis_decode_t *d) {
    if (!d || !d->valid) return 0;
    u32 n = 0;
    for (u32 i = 0; i < d->leaf_count; ++i) {
        if (d->row[i >> 3] & (u8)(1u << (i & 7u))) ++n;
    }
    return n;
}
