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

        /* Quake/GoldSrc: children[0]=front (d>=0), children[1]=back (d<0). */
        idx = (d < 0.0f) ? cn->children[1] : cn->children[0];
    }
    /* Reached a leaf. Negative idx is contents (EMPTY=-1, SOLID=-2, …). */
    return (idx == AETHER_CONTENTS_SOLID);
}

/* Binary-search the farthest non-solid point along one axis from `base`. */
static f32 collision_slide_axis(const aether_collision_t *c,
                                aether_vec3_t base,
                                i32 axis,
                                f32 dest,
                                i32 hull_index) {
    aether_vec3_t t = base;
    f32 *comp = (axis == 0) ? &t.x : (axis == 1) ? &t.y : &t.z;
    *comp = dest;
    if (!aether_collision_point_in_solid(c, t, hull_index)) return dest;

    f32 lo = (axis == 0) ? base.x : (axis == 1) ? base.y : base.z;
    f32 hi = dest;
    for (int i = 0; i < 12; ++i) {
        f32 mid = 0.5f * (lo + hi);
        *comp = mid;
        if (aether_collision_point_in_solid(c, t, hull_index))
            hi = mid;
        else
            lo = mid;
    }
    return lo;
}

/* Axis-separated slide without step-up. */
static aether_vec3_t collision_slide_move(const aether_collision_t *c,
                                          aether_vec3_t from,
                                          aether_vec3_t to,
                                          i32 hull_index,
                                          bool *out_on_ground) {
    if (out_on_ground) *out_on_ground = false;
    aether_vec3_t result = from;

    if (to.x != from.x)
        result.x = collision_slide_axis(c, result, 0, to.x, hull_index);
    if (to.y != from.y)
        result.y = collision_slide_axis(c, result, 1, to.y, hull_index);
    if (to.z != from.z) {
        f32 nz = collision_slide_axis(c, result, 2, to.z, hull_index);
        if (to.z < from.z && nz > to.z + 1e-3f && out_on_ground)
            *out_on_ground = true;
        result.z = nz;
    }
    return result;
}

/* ---------- Axis-separated movement + Quake-style step-up ---------- */
aether_vec3_t aether_collision_move(aether_collision_t *c,
                                     aether_vec3_t from,
                                     aether_vec3_t to,
                                     i32 hull_index,
                                     f32 max_step,
                                     bool *out_on_ground) {
    if (out_on_ground) *out_on_ground = false;
    if (!c) return to;

    bool ground = false;
    aether_vec3_t slid = collision_slide_move(c, from, to, hull_index, &ground);

    /* Detect horizontal blockage (wish further than we got). */
    const f32 eps = 1e-3f;
    f32 wish_hx = to.x - from.x;
    f32 wish_hy = to.y - from.y;
    f32 got_hx  = slid.x - from.x;
    f32 got_hy  = slid.y - from.y;
    bool blocked = false;
    if (wish_hx >  eps && got_hx < wish_hx - eps) blocked = true;
    if (wish_hx < -eps && got_hx > wish_hx + eps) blocked = true;
    if (wish_hy >  eps && got_hy < wish_hy - eps) blocked = true;
    if (wish_hy < -eps && got_hy > wish_hy + eps) blocked = true;

    if (!blocked || max_step <= 0.0f) {
        if (out_on_ground) *out_on_ground = ground;
        return slid;
    }

    /* 1) Raise up to max_step (abort if we barely rise). */
    f32 up_z = collision_slide_axis(c, from, 2, from.z + max_step, hull_index);
    if (up_z < from.z + 1.0f) {
        if (out_on_ground) *out_on_ground = ground;
        return slid;
    }
    aether_vec3_t elevated = from;
    elevated.z = up_z;

    /* 2) Horizontal from elevated toward wish XY. */
    aether_vec3_t mid = elevated;
    if (to.x != from.x)
        mid.x = collision_slide_axis(c, mid, 0, to.x, hull_index);
    if (to.y != from.y)
        mid.y = collision_slide_axis(c, mid, 1, to.y, hull_index);

    f32 step_hx = mid.x - from.x;
    f32 step_hy = mid.y - from.y;
    f32 slid_h2 = got_hx * got_hx + got_hy * got_hy;
    f32 step_h2 = step_hx * step_hx + step_hy * step_hy;
    if (step_h2 <= slid_h2 + 1e-4f) {
        /* No extra horizontal progress (wall taller than step). */
        if (out_on_ground) *out_on_ground = ground;
        return slid;
    }

    /* 3) Drop by max_step to find the new floor; then honor remaining vertical wish. */
    bool step_ground = false;
    aether_vec3_t result = mid;
    f32 drop_to = mid.z - max_step;
    f32 nz = collision_slide_axis(c, mid, 2, drop_to, hull_index);
    if (nz > drop_to + eps) step_ground = true;
    result.z = nz;

    if (to.z > result.z + eps) {
        /* Still rising (jump) after the step. */
        result.z = collision_slide_axis(c, result, 2, to.z, hull_index);
        step_ground = false;
    } else if (to.z < result.z - eps) {
        /* Continue falling past the step land height. */
        f32 z2 = collision_slide_axis(c, result, 2, to.z, hull_index);
        if (z2 > to.z + eps) step_ground = true;
        else step_ground = false;
        result.z = z2;
    }

    if (out_on_ground) *out_on_ground = step_ground;
    return result;
}

u32 aether_collision_clipnode_count(const aether_collision_t *c) {
    return c ? c->clipnode_count : 0;
}

i32 aether_collision_hull_root(const aether_collision_t *c, i32 hull_index) {
    if (!c || hull_index < 0 || hull_index > 2) return -1;
    return c->hull_root[hull_index];
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
