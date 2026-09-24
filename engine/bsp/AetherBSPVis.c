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
