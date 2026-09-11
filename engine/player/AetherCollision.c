/* AetherCollision.c — Clipnode-based player collision.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherCollision.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Same on-disk layout as GoldSrc clipnode. */
#pragma pack(push, 1)
typedef struct aether_clipnode {
    i32 plane;
    i16 children[2];
} aether_clipnode_t;
#pragma pack(pop)

struct aether_collision {
    const aether_bsp_t *bsp;              /* borrowed */
    const u8           *clipnodes_raw;
    u32                 clipnode_count;
    const u8           *planes_raw;
    u32                 plane_count;
    /* Root clipnode index for each hull (1 = standing, 2 = crouching) */
    i32                 hull_root[3];
};

/* ---------- Helpers ---------- */
static const aether_bsp_plane_t *plane_at(const aether_collision_t *c, u32 idx) {
    if (idx >= c->plane_count) return NULL;
    return (const aether_bsp_plane_t*)(c->planes_raw + idx * sizeof(aether_bsp_plane_t));
}

static const aether_clipnode_t *clipnode_at(const aether_collision_t *c, u32 idx) {
    if (idx >= c->clipnode_count) return NULL;
    return (const aether_clipnode_t*)(c->clipnodes_raw + idx * sizeof(aether_clipnode_t));
}

/* Read headnodes[3] from models[0] — that's the world model. */
static void read_world_headnodes(const aether_bsp_t *bsp, i32 out[4]) {
    for (int i = 0; i < 4; ++i) out[i] = -1;
    const aether_bsp_model_t *m0 = aether_bsp_model_at(bsp, 0);
    if (!m0) return;
    /* headnodes is the first 4 ints after the two vec3s (mins/maxs) and origin vec3 */
    /* Offset = mins(12) + maxs(12) + origin(12) = 36. Then 4 * i32. */
    const u8 *p = (const u8*)m0;
    for (int i = 0; i < 4; ++i) {
        i32 v = (i32)((u32)p[36 + i*4 + 0]
                     | ((u32)p[36 + i*4 + 1] << 8)
                     | ((u32)p[36 + i*4 + 2] << 16)
                     | ((u32)p[36 + i*4 + 3] << 24));
        out[i] = v;
    }
}

/* ---------- Build / free ---------- */
aether_collision_t *aether_collision_build(const aether_bsp_t *bsp) {
    if (!bsp || !aether_bsp_is_valid(bsp)) return NULL;

    aether_collision_t *c = (aether_collision_t*)calloc(1, sizeof *c);
    if (!c) return NULL;
    c->bsp = bsp;

    c->clipnodes_raw  = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_CLIPNODES);
    c->clipnode_count = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_CLIPNODES) / sizeof(aether_clipnode_t);
    c->planes_raw     = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_PLANES);
    c->plane_count    = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_PLANES) / sizeof(aether_bsp_plane_t);

    i32 head[4];
    read_world_headnodes(bsp, head);
    c->hull_root[0] = head[0];
    c->hull_root[1] = head[1];
    c->hull_root[2] = head[2];

    aether_log(AETHER_LOG_INFO, "collision",
               "built: %u clipnodes, %u planes, roots [%d,%d,%d,%d]",
               c->clipnode_count, c->plane_count,
               head[0], head[1], head[2], head[3]);
    return c;
}

void aether_collision_free(aether_collision_t *c) {
    if (c) free(c);
}

/* ---------- Point-in-solid test ---------- */
bool aether_collision_point_in_solid(const aether_collision_t *c,
                                     aether_vec3_t point,
                                     i32 hull_index) {
    if (!c || !c->clipnodes_raw) return false;
    if (hull_index < 1 || hull_index > 2) hull_index = 1;

    i32 idx = c->hull_root[hull_index];
    if (idx < 0) return false;  /* no hull */

    int safety = 0;
    while (idx >= 0) {
        if (++safety > 4096) return false; /* protect against loops */
        const aether_clipnode_t *cn = clipnode_at(c, (u32)idx);
        if (!cn) return false;
        const aether_bsp_plane_t *pl = plane_at(c, (u32)cn->plane);
        if (!pl) return false;

        f32 d = pl->normal[0]*point.x
              + pl->normal[1]*point.y
              + pl->normal[2]*point.z
              - pl->dist;

        /* GoldSrc convention: negative d → solid side. */
        idx = (d < 0.0f) ? cn->children[0] : cn->children[1];
    }
    /* Reached a leaf. Leaf content is (idx) as a negative enum. */
    return (idx == AETHER_CONTENTS_SOLID);
}

/* ---------- Axis-separated movement ---------- */
aether_vec3_t aether_collision_move(aether_collision_t *c,
                                     aether_vec3_t from,
                                     aether_vec3_t to,
                                     i32 hull_index,
                                     bool *out_on_ground) {
    if (out_on_ground) *out_on_ground = false;
    if (!c) return to;

    aether_vec3_t result = from;

    /* --- Move X --- */
    if (to.x != from.x) {
        aether_vec3_t t = result; t.x = to.x;
        if (!aether_collision_point_in_solid(c, t, hull_index)) result.x = t.x;
    }
    /* --- Move Y --- */
    if (to.y != from.y) {
        aether_vec3_t t = result; t.y = to.y;
        if (!aether_collision_point_in_solid(c, t, hull_index)) result.y = t.y;
    }
    /* --- Move Z --- */
    if (to.z != from.z) {
        aether_vec3_t t = result; t.z = to.z;
        if (!aether_collision_point_in_solid(c, t, hull_index)) {
            result.z = t.z;
        } else if (to.z < from.z && out_on_ground) {
            /* Downward motion blocked → landed on ground. */
            *out_on_ground = true;
        }
    }

    return result;
}

void aether_collision_dump(const aether_collision_t *c) {
    if (!c) { aether_log(AETHER_LOG_WARN, "collision", "null"); return; }
    aether_log(AETHER_LOG_INFO, "collision", "===== COLLISION =====");
    aether_log(AETHER_LOG_INFO, "collision", "  clipnodes: %u", c->clipnode_count);
    aether_log(AETHER_LOG_INFO, "collision", "  planes   : %u", c->plane_count);
    aether_log(AETHER_LOG_INFO, "collision", "  hull roots [1,2]: %d, %d",
               c->hull_root[1], c->hull_root[2]);
    aether_log(AETHER_LOG_INFO, "collision", "======================");
}
