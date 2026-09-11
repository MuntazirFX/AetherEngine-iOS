/* AetherBSP.c — GoldSrc BSP v30 parser implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherBSP.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct aether_bsp_lump {
    u32 offset;
    u32 size;
} aether_bsp_lump_t;

struct aether_bsp {
    u8       *raw;
    u32       raw_size;
    char      source[256];
    u32       version;
    bool      valid;
    aether_bsp_lump_t lumps[AETHER_BSP_LUMP_COUNT];
};

static const char *k_lump_names[AETHER_BSP_LUMP_COUNT] = {
    "ENTITIES", "PLANES", "TEXTURES", "VERTICES", "VISIBILITY",
    "NODES", "TEXINFO", "FACES", "LIGHTING", "CLIPNODES",
    "LEAVES", "MARKSURFACES", "EDGES", "SURFEDGES", "MODELS"
};

const char *aether_bsp_lump_name(aether_bsp_lump_id_t id) {
    if ((int)id < 0 || (int)id >= AETHER_BSP_LUMP_COUNT) return "?";
    return k_lump_names[(int)id];
}

static u32 rd_u32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
static i32 rd_i32(const u8 *p) { return (i32)rd_u32(p); }

static bool parse_header(aether_bsp_t *bsp) {
    if (bsp->raw_size < 4 + AETHER_BSP_LUMP_COUNT * 8) {
        aether_log(AETHER_LOG_ERROR, "bsp", "file too small");
        return false;
    }
    u32 version = rd_u32(bsp->raw);
    if (version != AETHER_BSP_VERSION) {
        aether_log(AETHER_LOG_ERROR, "bsp", "unsupported BSP version %u", version);
        return false;
    }
    bsp->version = version;
    const u8 *p = bsp->raw + 4;
    for (u32 i = 0; i < AETHER_BSP_LUMP_COUNT; ++i) {
        u32 off  = rd_u32(p + i * 8 + 0);
        u32 size = rd_u32(p + i * 8 + 4);
        if ((u64)off + (u64)size > (u64)bsp->raw_size) {
            aether_log(AETHER_LOG_WARN, "bsp", "lump %s out of bounds", k_lump_names[i]);
            off = 0; size = 0;
        }
        bsp->lumps[i].offset = off;
        bsp->lumps[i].size   = size;
    }
    return true;
}

aether_bsp_t *aether_bsp_load_from_memory(const u8 *data, u32 size,
                                          const char *source_name) {
    if (!data || size == 0) return NULL;
    aether_bsp_t *bsp = (aether_bsp_t*)calloc(1, sizeof *bsp);
    if (!bsp) return NULL;
    bsp->raw = (u8*)malloc(size);
    if (!bsp->raw) { free(bsp); return NULL; }
    memcpy(bsp->raw, data, size);
    bsp->raw_size = size;
    aether_str_copy(bsp->source, sizeof bsp->source,
                    source_name ? source_name : "<memory>");
    if (!parse_header(bsp)) {
        free(bsp->raw); free(bsp); return NULL;
    }
    bsp->valid = true;
    aether_log(AETHER_LOG_INFO, "bsp", "loaded %s (%u bytes, v%u)",
               bsp->source, bsp->raw_size, bsp->version);
    return bsp;
}

aether_bsp_t *aether_bsp_load(const char *filepath) {
    if (!filepath) return NULL;
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    u8 *buf = (u8*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    aether_bsp_t *bsp = aether_bsp_load_from_memory(buf, (u32)sz, filepath);
    free(buf);
    return bsp;
}

void aether_bsp_free(aether_bsp_t *bsp) {
    if (!bsp) return;
    free(bsp->raw);
    free(bsp);
}

bool        aether_bsp_is_valid(const aether_bsp_t *bsp) { return bsp && bsp->valid; }
u32         aether_bsp_version (const aether_bsp_t *bsp) { return bsp ? bsp->version : 0; }
const char *aether_bsp_source  (const aether_bsp_t *bsp) { return bsp ? bsp->source  : ""; }

u32 aether_bsp_lump_offset(const aether_bsp_t *bsp, aether_bsp_lump_id_t id) {
    if (!bsp || (int)id < 0 || (int)id >= AETHER_BSP_LUMP_COUNT) return 0;
    return bsp->lumps[(int)id].offset;
}
u32 aether_bsp_lump_size(const aether_bsp_t *bsp, aether_bsp_lump_id_t id) {
    if (!bsp || (int)id < 0 || (int)id >= AETHER_BSP_LUMP_COUNT) return 0;
    return bsp->lumps[(int)id].size;
}
const u8 *aether_bsp_lump_data(const aether_bsp_t *bsp, aether_bsp_lump_id_t id) {
    if (!bsp || (int)id < 0 || (int)id >= AETHER_BSP_LUMP_COUNT) return NULL;
    const aether_bsp_lump_t *L = &bsp->lumps[(int)id];
    if (L->offset == 0 || L->size == 0) return NULL;
    return bsp->raw + L->offset;
}

u32 aether_bsp_vertex_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_VERTICES) / sizeof(aether_bsp_vertex_t);
}
u32 aether_bsp_plane_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_PLANES) / sizeof(aether_bsp_plane_t);
}
u32 aether_bsp_edge_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_EDGES) / sizeof(aether_bsp_edge_t);
}
u32 aether_bsp_face_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_FACES) / sizeof(aether_bsp_face_t);
}
u32 aether_bsp_node_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_NODES) / sizeof(aether_bsp_node_t);
}
u32 aether_bsp_leaf_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_LEAVES) / sizeof(aether_bsp_leaf_t);
}
u32 aether_bsp_model_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_MODELS) / sizeof(aether_bsp_model_t);
}
u32 aether_bsp_texinfo_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    return aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_TEXINFO) / sizeof(aether_bsp_texinfo_t);
}

u32 aether_bsp_miptex_count(const aether_bsp_t *bsp) {
    if (!bsp) return 0;
    const u8 *p = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_TEXTURES);
    if (!p || aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_TEXTURES) < 4) return 0;
    return rd_u32(p);
}

#define ACCESSOR(TYPE, FUNC, LUMP, COUNT)                                 \
    const TYPE *FUNC(const aether_bsp_t *bsp, u32 idx) {                  \
        if (!bsp) return NULL;                                            \
        u32 n = aether_bsp_##COUNT##_count(bsp);                          \
        if (idx >= n) return NULL;                                        \
        const u8 *p = aether_bsp_lump_data(bsp, LUMP);                    \
        if (!p) return NULL;                                              \
        return (const TYPE *)(p + (size_t)idx * sizeof(TYPE));            \
    }

ACCESSOR(aether_bsp_vertex_t,  aether_bsp_vertex_at,  AETHER_BSP_LUMP_VERTICES, vertex)
ACCESSOR(aether_bsp_plane_t,   aether_bsp_plane_at,   AETHER_BSP_LUMP_PLANES,   plane)
ACCESSOR(aether_bsp_edge_t,    aether_bsp_edge_at,    AETHER_BSP_LUMP_EDGES,    edge)
ACCESSOR(aether_bsp_face_t,    aether_bsp_face_at,    AETHER_BSP_LUMP_FACES,    face)
ACCESSOR(aether_bsp_node_t,    aether_bsp_node_at,    AETHER_BSP_LUMP_NODES,    node)
ACCESSOR(aether_bsp_leaf_t,    aether_bsp_leaf_at,    AETHER_BSP_LUMP_LEAVES,   leaf)
ACCESSOR(aether_bsp_model_t,   aether_bsp_model_at,   AETHER_BSP_LUMP_MODELS,   model)
ACCESSOR(aether_bsp_texinfo_t, aether_bsp_texinfo_at, AETHER_BSP_LUMP_TEXINFO,  texinfo)

const aether_bsp_miptex_t *aether_bsp_miptex_at(const aether_bsp_t *bsp, u32 idx) {
    if (!bsp) return NULL;
    const u8 *p = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_TEXTURES);
    if (!p) return NULL;
    u32 count = rd_u32(p);
    if (idx >= count) return NULL;
    i32 rel = rd_i32(p + 4 + idx * 4);
    if (rel < 0 || (u32)rel >= aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_TEXTURES)) return NULL;
    return (const aether_bsp_miptex_t *)(p + rel);
}

void aether_bsp_dump(const aether_bsp_t *bsp) {
    if (!bsp || !bsp->valid) {
        aether_log(AETHER_LOG_WARN, "bsp", "dump: invalid bsp");
        return;
    }
    aether_log(AETHER_LOG_INFO, "bsp", "=====================================");
    aether_log(AETHER_LOG_INFO, "bsp", "BSP: %s", bsp->source);
    aether_log(AETHER_LOG_INFO, "bsp", "  version : %u", bsp->version);
    aether_log(AETHER_LOG_INFO, "bsp", "  size    : %u bytes", bsp->raw_size);
    aether_log(AETHER_LOG_INFO, "bsp", "-------------------------------------");
    for (u32 i = 0; i < AETHER_BSP_LUMP_COUNT; ++i) {
        const aether_bsp_lump_t *L = &bsp->lumps[i];
        aether_log(AETHER_LOG_INFO, "bsp",
                   "  lump[%2u] %-12s off=%-8u size=%-8u",
                   i, k_lump_names[i], L->offset, L->size);
    }
    aether_log(AETHER_LOG_INFO, "bsp", "-------------------------------------");
    aether_log(AETHER_LOG_INFO, "bsp", "  vertices  : %u", aether_bsp_vertex_count (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  planes    : %u", aether_bsp_plane_count  (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  edges     : %u", aether_bsp_edge_count   (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  faces     : %u", aether_bsp_face_count   (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  nodes     : %u", aether_bsp_node_count   (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  leaves    : %u", aether_bsp_leaf_count   (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  models    : %u", aether_bsp_model_count  (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  texinfo   : %u", aether_bsp_texinfo_count(bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "  miptex    : %u", aether_bsp_miptex_count (bsp));
    aether_log(AETHER_LOG_INFO, "bsp", "=====================================");
}
