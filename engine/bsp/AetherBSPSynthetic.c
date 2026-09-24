/* AetherBSPSynthetic.c — Synthesize a tiny BSP v30 demo room.
 * Clean-room only. No Valve/Half-Life map bytes.
 * AetherEngine-iOS.
 */
#include "AetherBSPSynthetic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)((v >> 8) & 0xff);
    p[2] = (u8)((v >> 16) & 0xff);
    p[3] = (u8)((v >> 24) & 0xff);
}

static void wr_i32(u8 *p, i32 v) { wr_u32(p, (u32)v); }

bool aether_bsp_is_synthetic(const aether_bsp_t *bsp) {
    if (!bsp) return false;
    const char *s = aether_bsp_source(bsp);
    return s && strncmp(s, "synthetic:", 10) == 0;
}

aether_bsp_t *aether_bsp_create_synthetic_room(void) {
    static const char k_ents[] =
        "{\n"
        "\"classname\" \"worldspawn\"\n"
        "\"message\" \"Aether synthetic demo room\"\n"
        "\"skyname\" \"desert\"\n"
        "}\n"
        "{\n"
        "\"classname\" \"info_player_start\"\n"
        "\"origin\" \"0 0 40\"\n"
        "\"angles\" \"0 90 0\"\n"
        "}\n"
        "{\n"
        "\"classname\" \"monster_headcrab\"\n"
        "\"origin\" \"120 40 0\"\n"
        "}\n"
        "{\n"
        "\"classname\" \"monster_zombie\"\n"
        "\"origin\" \"-100 100 0\"\n"
        "}\n"
        "{\n"
        "\"classname\" \"monster_barney\"\n"
        "\"origin\" \"80 -120 0\"\n"
        "}\n"
        "{\n"
        "\"classname\" \"light\"\n"
        "\"origin\" \"0 0 100\"\n"
        "}\n";

    /* 8 box corners: floor z=0, ceiling z=128, xy ±256 */
    aether_bsp_vertex_t verts[8] = {
        {-256.f, -256.f,   0.f},
        { 256.f, -256.f,   0.f},
        { 256.f,  256.f,   0.f},
        {-256.f,  256.f,   0.f},
        {-256.f, -256.f, 128.f},
        { 256.f, -256.f, 128.f},
        { 256.f,  256.f, 128.f},
        {-256.f,  256.f, 128.f},
    };

    /* Render planes 0..6; clip planes 7..37 (Quake n*p - dist, front = +).
     * Ledge: world box x[80..256] y[+/-256] z[0..16] (16u step <= STEPSIZE 18).
     * Low alcove (-X): physical ceiling z=48 so standing (72u) blocked, crouch (36u) fits.
     * +Y water pool: physical x[-80..80] y[120..220] z[0..48] → CONTENTS_WATER.
     * Standing/crouch hull Z are distinct (room_ceil - hull height), not shared. */
    aether_bsp_plane_t planes[38];
    memset(planes, 0, sizeof planes);
    planes[0].normal[2] =  1.f; planes[0].dist =    0.f; planes[0].type = 2; /* floor +Z */
    planes[1].normal[2] = -1.f; planes[1].dist = -128.f; planes[1].type = 2; /* ceiling -Z */
    planes[2].normal[1] =  1.f; planes[2].dist = -256.f; planes[2].type = 1; /* -Y inward */
    planes[3].normal[1] = -1.f; planes[3].dist = -256.f; planes[3].type = 1; /* +Y inward */
    planes[4].normal[0] =  1.f; planes[4].dist = -256.f; planes[4].type = 0; /* -X inward */
    planes[5].normal[0] = -1.f; planes[5].dist = -256.f; planes[5].type = 0; /* +X inward */
    planes[6].normal[0] =  1.f; planes[6].dist =    0.f; planes[6].type = 0; /* room split X=0 */
    /* Point hull: exact room AABB (feet can sit on z=0). */
    planes[7].normal[0]  = 1.f; planes[7].dist  = -256.f; planes[7].type  = 0;
    planes[8].normal[0]  = 1.f; planes[8].dist  =  256.f; planes[8].type  = 0;
    planes[9].normal[1]  = 1.f; planes[9].dist  = -256.f; planes[9].type  = 1;
    planes[10].normal[1] = 1.f; planes[10].dist =  256.f; planes[10].type = 1;
    planes[11].normal[2] = 1.f; planes[11].dist =    0.f; planes[11].type = 2;
    planes[12].normal[2] = 1.f; planes[12].dist =  128.f; planes[12].type = 2;
    /* Standing/crouch hull: XY inset 16u; Z ceilings differ by hull height. */
    planes[13].normal[0] = 1.f; planes[13].dist = -240.f; planes[13].type = 0;
    planes[14].normal[0] = 1.f; planes[14].dist =  240.f; planes[14].type = 0;
    planes[15].normal[1] = 1.f; planes[15].dist = -240.f; planes[15].type = 1;
    planes[16].normal[1] = 1.f; planes[16].dist =  240.f; planes[16].type = 1;
    planes[17].normal[2] = 1.f; planes[17].dist =    0.f; planes[17].type = 2; /* shared floor */
    planes[18].normal[2] = 1.f; planes[18].dist =   56.f; planes[18].type = 2; /* stand feet max = 128-72 */
    /* Ledge extras: top z=16; standing face x=64 (=80-16); point face x=80. */
    planes[19].normal[2] = 1.f; planes[19].dist =   16.f; planes[19].type = 2;
    planes[20].normal[0] = 1.f; planes[20].dist =   64.f; planes[20].type = 0;
    planes[21].normal[0] = 1.f; planes[21].dist =   80.f; planes[21].type = 0;
    /* Crouch distinct Z ceiling (128-36=92) -- not standing AABB reused. */
    planes[22].normal[2] = 1.f; planes[22].dist =   92.f; planes[22].type = 2;
    /* Low alcove (-X): physical x[-256,-96] y[+/-128] ceil z=48 -> hull-space solids. */
    planes[23].normal[0] = 1.f; planes[23].dist = -112.f; planes[23].type = 0; /* alcove +X (hull) */
    planes[24].normal[1] = 1.f; planes[24].dist = -112.f; planes[24].type = 1;
    planes[25].normal[1] = 1.f; planes[25].dist =  112.f; planes[25].type = 1;
    planes[26].normal[2] = 1.f; planes[26].dist =   48.f; planes[26].type = 2; /* point solid bottom */
    planes[27].normal[2] = 1.f; planes[27].dist =    0.f; planes[27].type = 2; /* standing solid bottom */
    planes[28].normal[2] = 1.f; planes[28].dist =   12.f; planes[28].type = 2; /* crouch solid bottom = 48-36 */
    planes[29].normal[0] = 1.f; planes[29].dist =  -96.f; planes[29].type = 0; /* point alcove +X */
    /* +Y water pool planes (surface z=48 reuses plane 26). */
    planes[30].normal[0] = 1.f; planes[30].dist =  -80.f; planes[30].type = 0; /* point water -X */
    planes[31].normal[0] = 1.f; planes[31].dist =   80.f; planes[31].type = 0; /* point water +X */
    planes[32].normal[1] = 1.f; planes[32].dist =  120.f; planes[32].type = 1; /* point water -Y */
    planes[33].normal[1] = 1.f; planes[33].dist =  220.f; planes[33].type = 1; /* point water +Y */
    planes[34].normal[0] = 1.f; planes[34].dist =  -64.f; planes[34].type = 0; /* hull water -X */
    planes[35].normal[0] = 1.f; planes[35].dist =   64.f; planes[35].type = 0; /* hull water +X */
    planes[36].normal[1] = 1.f; planes[36].dist =  136.f; planes[36].type = 1; /* hull water -Y */
    planes[37].normal[1] = 1.f; planes[37].dist =  204.f; planes[37].type = 1; /* hull water +Y */

    aether_bsp_edge_t edges[12] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, /* 0..3 floor ring */
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, /* 4..7 ceiling ring */
        {0, 4}, {1, 5}, {2, 6}, {3, 7}, /* 8..11 verticals */
    };

    /* Surfedge sign: +e → edge.v0, -e → edge.v1 (travel direction along the face). */
    i32 surfedges[24] = {
        /* floor  0→1→2→3 */
         0,  1,  2,  3,
        /* ceiling 4→7→6→5 */
        -7, -6, -5, -4,
        /* -Y wall 0→1→5→4 */
         0,  9, -4, -8,
        /* +Y wall 3→2→6→7 */
        -2, 10,  6,-11,
        /* -X wall 0→3→7→4 */
        -3, 11,  7, -8,
        /* +X wall 1→5→6→2 */
         9,  5,-10, -1,
    };

    aether_bsp_face_t faces[6];
    memset(faces, 0, sizeof faces);
    for (int i = 0; i < 6; ++i) {
        faces[i].plane = (u16)i;
        faces[i].side = 0;
        faces[i].first_edge = i * 4;
        faces[i].num_edges = 4;
        faces[i].texinfo = (u16)(i % 2); /* alternate two texinfos */
        /* Alternate primary style 0 (full) and 2 (flicker) for per-face GPU weights. */
        faces[i].styles[0] = (u8)((i % 2) == 0 ? 0 : 2);
        faces[i].styles[1] = faces[i].styles[2] = faces[i].styles[3] = 255;
        faces[i].light_offset = (i32)(i * 3); /* 1 RGB sample per face */
    }

    aether_bsp_texinfo_t texinfos[2];
    memset(texinfos, 0, sizeof texinfos);
    /* Floor/ceil style: XY axes */
    texinfos[0].vecs[0][0] = 1.f;
    texinfos[0].vecs[1][1] = 1.f;
    /* Wall style: XZ axes */
    texinfos[1].vecs[0][0] = 1.f;
    texinfos[1].vecs[1][2] = 1.f;

    /* Per-face RGB lighting samples (6 faces × 3 bytes). */
    u8 lighting[18];
    for (int i = 0; i < 6; ++i) {
        lighting[i*3+0] = (u8)(80 + i * 25);
        lighting[i*3+1] = (u8)(70 + i * 20);
        lighting[i*3+2] = (u8)(60 + i * 15);
    }

    aether_bsp_miptex_t mip;
    memset(&mip, 0, sizeof mip);
    memcpy(mip.name, "synth", 5);
    mip.width = 64;
    mip.height = 64;

    /* Node tree: one split at X=0 → west leaf 1 / east leaf 2 (leaf 0 = solid). */
    aether_bsp_node_t nodes[1];
    memset(nodes, 0, sizeof nodes);
    nodes[0].plane = 6;
    nodes[0].children[0] = -3; /* front (x >= 0) → leaf 2 */
    nodes[0].children[1] = -2; /* back  (x <  0) → leaf 1 */
    nodes[0].mins[0] = -256; nodes[0].mins[1] = -256; nodes[0].mins[2] = 0;
    nodes[0].maxs[0] =  256; nodes[0].maxs[1] =  256; nodes[0].maxs[2] = 128;
    nodes[0].first_face = 0;
    nodes[0].num_faces = 6;

    /* Marksurfaces: west leaf faces + east leaf faces (shared floor/ceil listed twice). */
    u16 marksurfaces[8] = {
        /* leaf 1 west */ 4, 0, 1, 2,  /* -X, floor, ceil, -Y */
        /* leaf 2 east */ 5, 0, 1, 3   /* +X, floor, ceil, +Y */
    };

    aether_bsp_leaf_t leaves[3];
    memset(leaves, 0, sizeof leaves);
    /* leaf 0 — solid (GoldSrc convention) */
    leaves[0].contents = -2; /* AETHER_CONTENTS_SOLID */
    leaves[0].vis_offset = -1;
    /* leaf 1 — west half (x < 0): PVS sees leaf1+leaf2 (bits 0x06) */
    leaves[1].contents = -1; /* AETHER_CONTENTS_EMPTY */
    leaves[1].vis_offset = 0;
    leaves[1].mins[0] = -256; leaves[1].mins[1] = -256; leaves[1].mins[2] = 0;
    leaves[1].maxs[0] =    0; leaves[1].maxs[1] =  256; leaves[1].maxs[2] = 128;
    leaves[1].first_marksurface = 0;
    leaves[1].num_marksurfaces = 4;
    /* leaf 2 — east half (x >= 0): PVS sees only leaf2 (bits 0x04) — asymmetric stub */
    leaves[2].contents = -1;
    leaves[2].vis_offset = 1;
    leaves[2].mins[0] =    0; leaves[2].mins[1] = -256; leaves[2].mins[2] = 0;
    leaves[2].maxs[0] =  256; leaves[2].maxs[1] =  256; leaves[2].maxs[2] = 128;
    leaves[2].first_marksurface = 4;
    leaves[2].num_marksurfaces = 4;

    /* Multi-leaf PVS stub: 3 leaves → 1 byte/row. Non-zero byte = raw (no RLE). */
    u8 visbits[2] = { 0x06, 0x04 };

    /* Clipnodes: Quake/GoldSrc layout -- children[0]=front(+), children[1]=back(-);
     * negative child = contents (EMPTY=-1, SOLID=-2, WATER=-3).
     * 3 room + 3 water + 3 ledge + 3 alcove = 12 boxes × 6 = 72 nodes.
     * Room -> water pool -> ledge -> low-ceiling alcove -> EMPTY. */
    #pragma pack(push, 1)
    typedef struct { i32 plane; i16 children[2]; } synth_clipnode_t;
    #pragma pack(pop)
    enum { SYNTH_CN_EMPTY = -1, SYNTH_CN_SOLID = -2, SYNTH_CN_WATER = -3 };
    synth_clipnode_t clipnodes[72];
    memset(clipnodes, 0, sizeof clipnodes);
    /* Room AABB: interior leaf = `interior` (next obstacle root or EMPTY). */
    #define SYNTH_BOX_HULL(base, pL, pR, pY0, pY1, pZ0, pZ1, interior) do { \
        clipnodes[(base)+0].plane = (pL);  clipnodes[(base)+0].children[0] = (i16)((base)+1); clipnodes[(base)+0].children[1] = SYNTH_CN_SOLID; \
        clipnodes[(base)+1].plane = (pR);  clipnodes[(base)+1].children[0] = SYNTH_CN_SOLID;   clipnodes[(base)+1].children[1] = (i16)((base)+2); \
        clipnodes[(base)+2].plane = (pY0); clipnodes[(base)+2].children[0] = (i16)((base)+3); clipnodes[(base)+2].children[1] = SYNTH_CN_SOLID; \
        clipnodes[(base)+3].plane = (pY1); clipnodes[(base)+3].children[0] = SYNTH_CN_SOLID;   clipnodes[(base)+3].children[1] = (i16)((base)+4); \
        clipnodes[(base)+4].plane = (pZ0); clipnodes[(base)+4].children[0] = (i16)((base)+5); clipnodes[(base)+4].children[1] = SYNTH_CN_SOLID; \
        clipnodes[(base)+5].plane = (pZ1); clipnodes[(base)+5].children[0] = SYNTH_CN_SOLID;   clipnodes[(base)+5].children[1] = (i16)(interior); \
    } while (0)
    /* Solid obstacle AABB; `outside` chains to next obstacle or EMPTY. */
    #define SYNTH_SOLID_BOX_NEXT(base, pL, pR, pY0, pY1, pZ0, pZ1, outside) do { \
        clipnodes[(base)+0].plane = (pL);  clipnodes[(base)+0].children[0] = (i16)((base)+1); clipnodes[(base)+0].children[1] = (i16)(outside); \
        clipnodes[(base)+1].plane = (pR);  clipnodes[(base)+1].children[0] = (i16)(outside);   clipnodes[(base)+1].children[1] = (i16)((base)+2); \
        clipnodes[(base)+2].plane = (pY0); clipnodes[(base)+2].children[0] = (i16)((base)+3); clipnodes[(base)+2].children[1] = (i16)(outside); \
        clipnodes[(base)+3].plane = (pY1); clipnodes[(base)+3].children[0] = (i16)(outside);   clipnodes[(base)+3].children[1] = (i16)((base)+4); \
        clipnodes[(base)+4].plane = (pZ0); clipnodes[(base)+4].children[0] = (i16)((base)+5); clipnodes[(base)+4].children[1] = (i16)(outside); \
        clipnodes[(base)+5].plane = (pZ1); clipnodes[(base)+5].children[0] = (i16)(outside);   clipnodes[(base)+5].children[1] = SYNTH_CN_SOLID; \
    } while (0)
    /* Water volume AABB; inside leaf = WATER, outside chains onward. */
    #define SYNTH_WATER_BOX_NEXT(base, pL, pR, pY0, pY1, pZ0, pZ1, outside) do { \
        clipnodes[(base)+0].plane = (pL);  clipnodes[(base)+0].children[0] = (i16)((base)+1); clipnodes[(base)+0].children[1] = (i16)(outside); \
        clipnodes[(base)+1].plane = (pR);  clipnodes[(base)+1].children[0] = (i16)(outside);   clipnodes[(base)+1].children[1] = (i16)((base)+2); \
        clipnodes[(base)+2].plane = (pY0); clipnodes[(base)+2].children[0] = (i16)((base)+3); clipnodes[(base)+2].children[1] = (i16)(outside); \
        clipnodes[(base)+3].plane = (pY1); clipnodes[(base)+3].children[0] = (i16)(outside);   clipnodes[(base)+3].children[1] = (i16)((base)+4); \
        clipnodes[(base)+4].plane = (pZ0); clipnodes[(base)+4].children[0] = (i16)((base)+5); clipnodes[(base)+4].children[1] = (i16)(outside); \
        clipnodes[(base)+5].plane = (pZ1); clipnodes[(base)+5].children[0] = (i16)(outside);   clipnodes[(base)+5].children[1] = SYNTH_CN_WATER; \
    } while (0)
    /* Room -> water (54/60/66) -> ledge (18/24/30) -> alcove (36/42/48) -> EMPTY.
     * Standing Z ceiling plane 18 (56); crouch uses distinct plane 22 (92).
     * Water surface z=48 (plane 26); floor z=0 (planes 11/17). */
    SYNTH_BOX_HULL(0,  7,  8,  9, 10, 11, 12, 54); /* hull 0 / point */
    SYNTH_BOX_HULL(6, 13, 14, 15, 16, 17, 18, 60); /* hull 1 standing */
    SYNTH_BOX_HULL(12,13, 14, 15, 16, 17, 22, 66); /* hull 2 crouch (shorter Z) */
    /* Water pools (not solid) -> matching ledge */
    SYNTH_WATER_BOX_NEXT(54, 30, 31, 32, 33, 11, 26, 18); /* point water */
    SYNTH_WATER_BOX_NEXT(60, 34, 35, 36, 37, 17, 26, 24); /* standing water */
    SYNTH_WATER_BOX_NEXT(66, 34, 35, 36, 37, 17, 26, 30); /* crouch water */
    /* Point ledge -> point alcove */
    SYNTH_SOLID_BOX_NEXT(18, 21,  8,  9, 10, 11, 19, 36);
    /* Standing/crouch ledge -> matching alcove */
    SYNTH_SOLID_BOX_NEXT(24, 20, 14, 15, 16, 17, 19, 42);
    SYNTH_SOLID_BOX_NEXT(30, 20, 14, 15, 16, 17, 19, 48);
    /* Point alcove slab: physical z[48..128] x[-256,-96] y[+/-128] */
    SYNTH_SOLID_BOX_NEXT(36,  7, 29,  9, 10, 26, 12, SYNTH_CN_EMPTY);
    /* Standing alcove: expanded so 72u player cannot fit under z=48 (solid z[0..56]) */
    SYNTH_SOLID_BOX_NEXT(42, 13, 23, 24, 25, 27, 18, SYNTH_CN_EMPTY);
    /* Crouch alcove: 36u fits -- solid only above feet z=12 (48-36) */
    SYNTH_SOLID_BOX_NEXT(48, 13, 23, 24, 25, 28, 22, SYNTH_CN_EMPTY);
    #undef SYNTH_BOX_HULL
    #undef SYNTH_SOLID_BOX_NEXT
    #undef SYNTH_WATER_BOX_NEXT

    aether_bsp_model_t model;
    memset(&model, 0, sizeof model);
    model.mins[0] = -256.f; model.mins[1] = -256.f; model.mins[2] = 0.f;
    model.maxs[0] =  256.f; model.maxs[1] =  256.f; model.maxs[2] = 128.f;
    model.num_faces = 6;
    model.first_face = 0;
    model.num_leafs = 2; /* empty leaves (exclude solid leaf 0) */
    model.headnodes[0] = 0;  /* rendering/VIS node tree root (NODES lump) */
    model.headnodes[1] = 6;  /* standing clip hull root (CLIPNODES) */
    model.headnodes[2] = 12; /* crouching clip hull root */
    model.headnodes[3] = 0;  /* point clip hull root */

    u32 ents_sz = (u32)strlen(k_ents) + 1;
    u32 planes_sz = (u32)sizeof(planes);
    u32 tex_sz = 4u + 4u + (u32)sizeof(mip);
    u32 verts_sz = (u32)sizeof(verts);
    u32 edges_sz = (u32)sizeof(edges);
    u32 surf_sz = (u32)sizeof(surfedges);
    u32 faces_sz = (u32)sizeof(faces);
    u32 texinfo_sz = (u32)sizeof(texinfos);
    u32 lighting_sz = (u32)sizeof(lighting);
    u32 vis_sz = (u32)sizeof(visbits);
    u32 models_sz = (u32)sizeof(model);
    u32 nodes_sz = (u32)sizeof(nodes);
    u32 leaves_sz = (u32)sizeof(leaves);
    u32 mark_sz = (u32)sizeof(marksurfaces);
    u32 clip_sz = (u32)sizeof(clipnodes);
    /* VIS + LIGHTING lumps filled for PVS decompress + lightmap UV bake paths. */

    const u32 header = 4u + (u32)AETHER_BSP_LUMP_COUNT * 8u;
    u32 offsets[AETHER_BSP_LUMP_COUNT];
    u32 sizes[AETHER_BSP_LUMP_COUNT];
    memset(offsets, 0, sizeof offsets);
    memset(sizes, 0, sizeof sizes);

    u32 cursor = header;
    #define PLACE(id, sz) do { offsets[id] = cursor; sizes[id] = (sz); cursor += (sz); } while (0)
    PLACE(AETHER_BSP_LUMP_ENTITIES, ents_sz);
    PLACE(AETHER_BSP_LUMP_PLANES, planes_sz);
    PLACE(AETHER_BSP_LUMP_TEXTURES, tex_sz);
    PLACE(AETHER_BSP_LUMP_VERTICES, verts_sz);
    PLACE(AETHER_BSP_LUMP_VISIBILITY, vis_sz);
    PLACE(AETHER_BSP_LUMP_NODES, nodes_sz);
    PLACE(AETHER_BSP_LUMP_TEXINFO, texinfo_sz);
    PLACE(AETHER_BSP_LUMP_FACES, faces_sz);
    PLACE(AETHER_BSP_LUMP_LIGHTING, lighting_sz);
    PLACE(AETHER_BSP_LUMP_CLIPNODES, clip_sz);
    PLACE(AETHER_BSP_LUMP_LEAVES, leaves_sz);
    PLACE(AETHER_BSP_LUMP_MARKSURFACES, mark_sz);
    PLACE(AETHER_BSP_LUMP_EDGES, edges_sz);
    PLACE(AETHER_BSP_LUMP_SURFEDGES, surf_sz);
    PLACE(AETHER_BSP_LUMP_MODELS, models_sz);
    #undef PLACE

    u8 *buf = (u8 *)calloc(1, cursor);
    if (!buf) return NULL;

    wr_u32(buf, AETHER_BSP_VERSION);
    for (int i = 0; i < AETHER_BSP_LUMP_COUNT; ++i) {
        wr_u32(buf + 4 + i * 8 + 0, offsets[i]);
        wr_u32(buf + 4 + i * 8 + 4, sizes[i]);
    }

    memcpy(buf + offsets[AETHER_BSP_LUMP_ENTITIES], k_ents, ents_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_PLANES], planes, planes_sz);
    {
        u8 *t = buf + offsets[AETHER_BSP_LUMP_TEXTURES];
        wr_u32(t, 1);
        wr_i32(t + 4, 8);
        memcpy(t + 8, &mip, sizeof mip);
    }
    memcpy(buf + offsets[AETHER_BSP_LUMP_VERTICES], verts, verts_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_NODES], nodes, nodes_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_VISIBILITY], visbits, vis_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_TEXINFO], texinfos, texinfo_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_FACES], faces, faces_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_LIGHTING], lighting, lighting_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_CLIPNODES], clipnodes, clip_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_LEAVES], leaves, leaves_sz);
    {
        u8 *m = buf + offsets[AETHER_BSP_LUMP_MARKSURFACES];
        for (int i = 0; i < 8; ++i) {
            m[i * 2 + 0] = (u8)(marksurfaces[i] & 0xff);
            m[i * 2 + 1] = (u8)((marksurfaces[i] >> 8) & 0xff);
        }
    }
    memcpy(buf + offsets[AETHER_BSP_LUMP_EDGES], edges, edges_sz);
    {
        u8 *s = buf + offsets[AETHER_BSP_LUMP_SURFEDGES];
        for (int i = 0; i < 24; ++i) wr_i32(s + i * 4, surfedges[i]);
    }
    memcpy(buf + offsets[AETHER_BSP_LUMP_MODELS], &model, models_sz);

    aether_bsp_t *bsp = aether_bsp_load_from_memory(buf, cursor, "synthetic:demo_room");
    free(buf);
    if (!bsp) return NULL;

    aether_log(AETHER_LOG_INFO, "bsp-synth",
               "demo room: %u verts, %u faces, %u planes, %u nodes, %u leaves, %u clipnodes (VIS+LIGHTING)",
               aether_bsp_vertex_count(bsp),
               aether_bsp_face_count(bsp),
               aether_bsp_plane_count(bsp),
               aether_bsp_node_count(bsp),
               aether_bsp_leaf_count(bsp),
               (u32)(sizeof(clipnodes) / sizeof(clipnodes[0])));
    return bsp;
}
