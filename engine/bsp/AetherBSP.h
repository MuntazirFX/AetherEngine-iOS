/* AetherBSP.h — GoldSrc BSP v30 map file parser.
 * Based on publicly documented Half-Life map format.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_BSP_H
#define AETHER_BSP_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_BSP_VERSION      30
#define AETHER_BSP_LUMP_COUNT   15

typedef enum aether_bsp_lump_id {
    AETHER_BSP_LUMP_ENTITIES     = 0,
    AETHER_BSP_LUMP_PLANES       = 1,
    AETHER_BSP_LUMP_TEXTURES     = 2,
    AETHER_BSP_LUMP_VERTICES     = 3,
    AETHER_BSP_LUMP_VISIBILITY   = 4,
    AETHER_BSP_LUMP_NODES        = 5,
    AETHER_BSP_LUMP_TEXINFO      = 6,
    AETHER_BSP_LUMP_FACES        = 7,
    AETHER_BSP_LUMP_LIGHTING     = 8,
    AETHER_BSP_LUMP_CLIPNODES    = 9,
    AETHER_BSP_LUMP_LEAVES       = 10,
    AETHER_BSP_LUMP_MARKSURFACES = 11,
    AETHER_BSP_LUMP_EDGES        = 12,
    AETHER_BSP_LUMP_SURFEDGES    = 13,
    AETHER_BSP_LUMP_MODELS       = 14,
} aether_bsp_lump_id_t;

#pragma pack(push, 1)

typedef struct aether_bsp_vertex {
    f32 x, y, z;
} aether_bsp_vertex_t;

typedef struct aether_bsp_edge {
    u16 v0, v1;
} aether_bsp_edge_t;

typedef struct aether_bsp_plane {
    f32 normal[3];
    f32 dist;
    i32 type;
} aether_bsp_plane_t;

typedef struct aether_bsp_texinfo {
    f32 vecs[2][4];
    i32 miptex;
    i32 flags;
} aether_bsp_texinfo_t;

typedef struct aether_bsp_face {
    u16 plane;
    u16 side;
    i32 first_edge;
    u16 num_edges;
    u16 texinfo;
    u8  styles[4];
    i32 light_offset;
} aether_bsp_face_t;

typedef struct aether_bsp_leaf {
    i32 contents;
    i32 vis_offset;
    i16 mins[3];
    i16 maxs[3];
    u16 first_marksurface;
    u16 num_marksurfaces;
    u8  ambient[4];
} aether_bsp_leaf_t;

typedef struct aether_bsp_node {
    i32 plane;
    i16 children[2];
    i16 mins[3];
    i16 maxs[3];
    u16 first_face;
    u16 num_faces;
} aether_bsp_node_t;

typedef struct aether_bsp_model {
    f32 mins[3];
    f32 maxs[3];
    f32 origin[3];
    i32 headnodes[4];
    i32 num_leafs;
    i32 first_face;
    i32 num_faces;
} aether_bsp_model_t;

typedef struct aether_bsp_miptex {
    char name[16];
    u32  width;
    u32  height;
    u32  offsets[4];
} aether_bsp_miptex_t;

#pragma pack(pop)

typedef struct aether_bsp aether_bsp_t;

aether_bsp_t *aether_bsp_load(const char *filepath);
void          aether_bsp_free(aether_bsp_t *bsp);
aether_bsp_t *aether_bsp_load_from_memory(const u8 *data, u32 size,
                                          const char *source_name);

bool          aether_bsp_is_valid(const aether_bsp_t *bsp);
u32           aether_bsp_version(const aether_bsp_t *bsp);
const char   *aether_bsp_source(const aether_bsp_t *bsp);

u32  aether_bsp_lump_offset(const aether_bsp_t *bsp, aether_bsp_lump_id_t id);
u32  aether_bsp_lump_size  (const aether_bsp_t *bsp, aether_bsp_lump_id_t id);
const u8 *aether_bsp_lump_data(const aether_bsp_t *bsp, aether_bsp_lump_id_t id);
const char *aether_bsp_lump_name(aether_bsp_lump_id_t id);

u32 aether_bsp_vertex_count (const aether_bsp_t *bsp);
u32 aether_bsp_plane_count  (const aether_bsp_t *bsp);
u32 aether_bsp_edge_count   (const aether_bsp_t *bsp);
u32 aether_bsp_face_count   (const aether_bsp_t *bsp);
u32 aether_bsp_node_count   (const aether_bsp_t *bsp);
u32 aether_bsp_leaf_count   (const aether_bsp_t *bsp);
u32 aether_bsp_model_count  (const aether_bsp_t *bsp);
u32 aether_bsp_texinfo_count(const aether_bsp_t *bsp);
u32 aether_bsp_miptex_count (const aether_bsp_t *bsp);

const aether_bsp_vertex_t *aether_bsp_vertex_at(const aether_bsp_t *bsp, u32 idx);
const aether_bsp_plane_t  *aether_bsp_plane_at (const aether_bsp_t *bsp, u32 idx);
const aether_bsp_edge_t   *aether_bsp_edge_at  (const aether_bsp_t *bsp, u32 idx);
const aether_bsp_face_t   *aether_bsp_face_at  (const aether_bsp_t *bsp, u32 idx);
const aether_bsp_node_t   *aether_bsp_node_at  (const aether_bsp_t *bsp, u32 idx);
const aether_bsp_leaf_t   *aether_bsp_leaf_at  (const aether_bsp_t *bsp, u32 idx);
const aether_bsp_model_t  *aether_bsp_model_at (const aether_bsp_t *bsp, u32 idx);
const aether_bsp_texinfo_t *aether_bsp_texinfo_at(const aether_bsp_t *bsp, u32 idx);
const aether_bsp_miptex_t  *aether_bsp_miptex_at (const aether_bsp_t *bsp, u32 idx);

void aether_bsp_dump(const aether_bsp_t *bsp);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_BSP_H */
