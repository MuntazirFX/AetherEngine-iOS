/* AetherTexture.h — WAD3 + BSP miptex + palette + atlas (STEP 15B).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_TEXTURE_H
#define AETHER_TEXTURE_H

#include "../core/AetherCore.h"
#include "../bsp/AetherBSP.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MIPTEX_NAME_MAX  16
#define AETHER_WAD_MAX_LUMPS    8192
#define AETHER_ATLAS_MAX_SIZE   4096
#define AETHER_ATLAS_MAX_SLOTS  1024

#define AETHER_WAD_TYPE_PALETTE  0x40
#define AETHER_WAD_TYPE_MIPTEX   0x43

typedef struct aether_wad_lump {
    char name[AETHER_MIPTEX_NAME_MAX];
    i32  file_pos;
    i32  disk_size;
    i32  size;
    u8   type;
    u8   compression;
} aether_wad_lump_t;

typedef struct aether_wad aether_wad_t;

/* ---------- WAD API ---------- */
aether_wad_t *aether_wad_load_from_memory(const u8 *data, u32 size, const char *name);
aether_wad_t *aether_wad_load(const char *filepath);
void          aether_wad_free(aether_wad_t *wad);

bool aether_wad_is_valid(const aether_wad_t *wad);
u32  aether_wad_lump_count(const aether_wad_t *wad);
const aether_wad_lump_t *aether_wad_lump_at(const aether_wad_t *wad, u32 idx);

void aether_wad_dump(const aether_wad_t *wad);

/* ---------- Palette ---------- */
typedef struct aether_palette {
    u8   rgb[256][3];
    bool loaded;
} aether_palette_t;

/* Extract palette from halflife.wad (looks for lump type 0x40). */
bool aether_palette_from_wad(aether_palette_t *out, const aether_wad_t *wad);

/* Fallback: Quake-ish default greyscale-ish palette. */
void aether_palette_default(aether_palette_t *out);

/* ---------- BSP embedded miptex ---------- */
typedef struct aether_miptex_info {
    char      name[AETHER_MIPTEX_NAME_MAX];
    u32       width;
    u32       height;
    const u8 *pixel_data;   /* pointer into BSP buffer (mip 0, 8-bit indexed) */
    u32       pixel_size;
} aether_miptex_info_t;

bool aether_bsp_miptex_info(const aether_bsp_t *bsp, u32 index,
                            aether_miptex_info_t *out);
void aether_bsp_miptex_dump(const aether_bsp_t *bsp);

/* ---------- Texture atlas ---------- */
typedef struct aether_texture_slot {
    char name[AETHER_MIPTEX_NAME_MAX];
    u32  x, y;      /* top-left in atlas pixels */
    u32  w, h;      /* size in pixels */
} aether_texture_slot_t;

typedef struct aether_texture_atlas {
    u8                   *rgba;    /* RGBA8, width*height*4 */
    u32                   width;
    u32                   height;
    u32                   slot_count;
    aether_texture_slot_t slots[AETHER_ATLAS_MAX_SLOTS];
} aether_texture_atlas_t;

/* Build atlas from all BSP-embedded miptex. Simple shelf packing.
 * Returns NULL if no textures were found. */
aether_texture_atlas_t *aether_texture_atlas_build(const aether_bsp_t *bsp,
                                                    const aether_palette_t *palette);
void aether_texture_atlas_free(aether_texture_atlas_t *a);
void aether_texture_atlas_dump(const aether_texture_atlas_t *a);

/* Look up slot index by miptex index (returns -1 if not found). */
i32 aether_texture_atlas_find(const aether_texture_atlas_t *a,
                               const aether_bsp_t *bsp, u32 miptex_idx);

#ifdef __cplusplus
}
#endif
#endif
