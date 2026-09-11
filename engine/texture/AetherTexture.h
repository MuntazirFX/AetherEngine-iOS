/* AetherTexture.h — WAD3 parser + BSP embedded miptex access (STEP 15A).
 * Read-only diagnostics. No GPU upload yet. AetherEngine-iOS · Clean-room.
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

/* WAD3 lump types (only the ones we care about). */
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

bool          aether_wad_is_valid(const aether_wad_t *wad);
u32           aether_wad_lump_count(const aether_wad_t *wad);
const aether_wad_lump_t *aether_wad_lump_at(const aether_wad_t *wad, u32 idx);

/* Extract mip 0 pixels (8-bit indexed). out_w / out_h get dimensions. */
bool aether_wad_extract_mip0(const aether_wad_t *wad, u32 idx,
                             u8 *out_pixels, u32 pixel_cap,
                             u32 *out_w, u32 *out_h);

void aether_wad_dump(const aether_wad_t *wad);

/* ---------- BSP embedded miptex API ---------- */
typedef struct aether_miptex_info {
    char      name[AETHER_MIPTEX_NAME_MAX];
    u32       width;
    u32       height;
    const u8 *pixel_data;   /* pointer into BSP buffer (mip 0) */
    u32       pixel_size;   /* = width * height (8-bit indexed) */
} aether_miptex_info_t;

bool aether_bsp_miptex_info(const aether_bsp_t *bsp, u32 index,
                            aether_miptex_info_t *out);
void aether_bsp_miptex_dump(const aether_bsp_t *bsp);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_TEXTURE_H */
