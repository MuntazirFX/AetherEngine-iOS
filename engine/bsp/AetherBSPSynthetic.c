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

    aether_bsp_plane_t planes[6];
    memset(planes, 0, sizeof planes);
    planes[0].normal[2] =  1.f; planes[0].dist =    0.f; planes[0].type = 2; /* floor +Z */
    planes[1].normal[2] = -1.f; planes[1].dist = -128.f; planes[1].type = 2; /* ceiling -Z */
    planes[2].normal[1] =  1.f; planes[2].dist = -256.f; planes[2].type = 1; /* -Y inward */
    planes[3].normal[1] = -1.f; planes[3].dist = -256.f; planes[3].type = 1; /* +Y inward */
    planes[4].normal[0] =  1.f; planes[4].dist = -256.f; planes[4].type = 0; /* -X inward */
    planes[5].normal[0] = -1.f; planes[5].dist = -256.f; planes[5].type = 0; /* +X inward */

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
        faces[i].texinfo = 0;
        faces[i].styles[0] = faces[i].styles[1] = faces[i].styles[2] = faces[i].styles[3] = 255;
        faces[i].light_offset = -1;
    }

    aether_bsp_texinfo_t texinfo;
    memset(&texinfo, 0, sizeof texinfo);
    texinfo.vecs[0][0] = 1.f;
    texinfo.vecs[1][1] = 1.f;

    aether_bsp_miptex_t mip;
    memset(&mip, 0, sizeof mip);
    memcpy(mip.name, "synth", 5);
    mip.width = 64;
    mip.height = 64;

    aether_bsp_model_t model;
    memset(&model, 0, sizeof model);
    model.mins[0] = -256.f; model.mins[1] = -256.f; model.mins[2] = 0.f;
    model.maxs[0] =  256.f; model.maxs[1] =  256.f; model.maxs[2] = 128.f;
    model.num_faces = 6;
    model.first_face = 0;
    model.num_leafs = 1;
    model.headnodes[0] = model.headnodes[1] = model.headnodes[2] = model.headnodes[3] = -1;

    u32 ents_sz = (u32)strlen(k_ents) + 1;
    u32 planes_sz = (u32)sizeof(planes);
    u32 tex_sz = 4u + 4u + (u32)sizeof(mip);
    u32 verts_sz = (u32)sizeof(verts);
    u32 edges_sz = (u32)sizeof(edges);
    u32 surf_sz = (u32)sizeof(surfedges);
    u32 faces_sz = (u32)sizeof(faces);
    u32 texinfo_sz = (u32)sizeof(texinfo);
    u32 models_sz = (u32)sizeof(model);

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
    PLACE(AETHER_BSP_LUMP_TEXINFO, texinfo_sz);
    PLACE(AETHER_BSP_LUMP_FACES, faces_sz);
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
    memcpy(buf + offsets[AETHER_BSP_LUMP_TEXINFO], &texinfo, texinfo_sz);
    memcpy(buf + offsets[AETHER_BSP_LUMP_FACES], faces, faces_sz);
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
               "demo room: %u verts, %u faces, %u planes (no game assets)",
               aether_bsp_vertex_count(bsp),
               aether_bsp_face_count(bsp),
               aether_bsp_plane_count(bsp));
    return bsp;
}
