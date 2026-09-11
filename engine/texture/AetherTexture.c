/* AetherTexture.c — WAD3 + BSP miptex parsing (STEP 15A).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherTexture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct wad3_header {
    char ident[4];      /* "WAD3" */
    i32  numlumps;
    i32  infotableofs;
} wad3_header_t;

typedef struct wad3_lumpinfo {
    i32  filepos;
    i32  disksize;
    i32  size;
    u8   type;
    u8   compression;
    u8   pad1;
    u8   pad2;
    char name[16];
} wad3_lumpinfo_t;

typedef struct miptex_disk {
    char name[16];
    u32  width;
    u32  height;
    u32  offsets[4];
} miptex_disk_t;
#pragma pack(pop)

struct aether_wad {
    u8       *raw;
    u32       raw_size;
    char      source[256];
    bool      valid;
    u32       lump_count;
    aether_wad_lump_t *lumps;
};

/* ---------- helpers ---------- */
static u32 rd_u32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24);
}
static i32 rd_i32(const u8 *p) { return (i32)rd_u32(p); }

/* ---------- WAD load ---------- */
aether_wad_t *aether_wad_load_from_memory(const u8 *data, u32 size, const char *name) {
    if (!data || size < 12) return NULL;

    if (memcmp(data, "WAD3", 4) != 0 && memcmp(data, "WAD2", 4) != 0) {
        aether_log(AETHER_LOG_ERROR, "wad", "bad magic in %s", name ? name : "?");
        return NULL;
    }

    aether_wad_t *w = (aether_wad_t*)calloc(1, sizeof *w);
    if (!w) return NULL;

    w->raw = (u8*)malloc(size);
    if (!w->raw) { free(w); return NULL; }
    memcpy(w->raw, data, size);
    w->raw_size = size;
    aether_str_copy(w->source, sizeof w->source, name ? name : "<memory>");

    const wad3_header_t *h = (const wad3_header_t*)w->raw;
    if (h->numlumps <= 0 || h->numlumps > AETHER_WAD_MAX_LUMPS) {
        aether_log(AETHER_LOG_ERROR, "wad", "invalid lump count %d", h->numlumps);
        free(w->raw); free(w); return NULL;
    }
    if ((u32)h->infotableofs + (u32)h->numlumps * 32 > w->raw_size) {
        aether_log(AETHER_LOG_ERROR, "wad", "directory out of bounds");
        free(w->raw); free(w); return NULL;
    }

    w->lump_count = (u32)h->numlumps;
    w->lumps = (aether_wad_lump_t*)calloc(w->lump_count, sizeof(aether_wad_lump_t));
    if (!w->lumps) { free(w->raw); free(w); return NULL; }

    const u8 *dir = w->raw + h->infotableofs;
    for (u32 i = 0; i < w->lump_count; ++i) {
        const wad3_lumpinfo_t *L = (const wad3_lumpinfo_t*)(dir + i*32);
        aether_wad_lump_t *out = &w->lumps[i];
        /* Name: at most 16 bytes, ensure NUL terminated. */
        memcpy(out->name, L->name, 16);
        out->name[15] = '\0';
        out->file_pos    = L->filepos;
        out->disk_size   = L->disksize;
        out->size        = L->size;
        out->type        = L->type;
        out->compression = L->compression;
    }
    w->valid = true;
    aether_log(AETHER_LOG_INFO, "wad", "loaded %s: %u lumps",
               w->source, w->lump_count);
    return w;
}

aether_wad_t *aether_wad_load(const char *filepath) {
    if (!filepath) return NULL;
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 12) { fclose(f); return NULL; }
    u8 *buf = (u8*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    aether_wad_t *w = aether_wad_load_from_memory(buf, (u32)sz, filepath);
    free(buf);
    return w;
}

void aether_wad_free(aether_wad_t *wad) {
    if (!wad) return;
    free(wad->lumps);
    free(wad->raw);
    free(wad);
}

bool aether_wad_is_valid(const aether_wad_t *wad) { return wad && wad->valid; }
u32  aether_wad_lump_count(const aether_wad_t *wad) { return wad ? wad->lump_count : 0; }

const aether_wad_lump_t *aether_wad_lump_at(const aether_wad_t *wad, u32 idx) {
    if (!wad || idx >= wad->lump_count) return NULL;
    return &wad->lumps[idx];
}

/* Extract mip 0 (indexed 8-bit) from a miptex lump. */
bool aether_wad_extract_mip0(const aether_wad_t *wad, u32 idx,
                             u8 *out_pixels, u32 pixel_cap,
                             u32 *out_w, u32 *out_h) {
    if (!wad || idx >= wad->lump_count || !out_pixels) return false;
    const aether_wad_lump_t *L = &wad->lumps[idx];
    if (L->type != AETHER_WAD_TYPE_MIPTEX) return false;
    if (L->file_pos < 0 || (u32)L->file_pos + 40 > wad->raw_size) return false;
    if (L->disk_size < 40) return false;

    const u8 *base = wad->raw + L->file_pos;
    u32 w = rd_u32(base + 16);
    u32 h = rd_u32(base + 20);
    u32 off0 = rd_u32(base + 24);  /* mip 0 offset */
    if (w == 0 || h == 0 || w > 4096 || h > 4096) return false;
    u32 mip0_size = w * h;
    if (mip0_size > pixel_cap) return false;
    if ((u32)L->disk_size < off0 + mip0_size) return false;

    memcpy(out_pixels, base + off0, mip0_size);
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
    return true;
}

void aether_wad_dump(const aether_wad_t *wad) {
    if (!aether_wad_is_valid(wad)) {
        aether_log(AETHER_LOG_WARN, "wad", "dump: invalid");
        return;
    }
    aether_log(AETHER_LOG_INFO, "wad", "===== %s =====", wad->source);
    aether_log(AETHER_LOG_INFO, "wad", "  lumps  : %u", wad->lump_count);
    u32 shown = 0;
    u32 miptex_count = 0, palette_count = 0, other_count = 0;
    for (u32 i = 0; i < wad->lump_count; ++i) {
        const aether_wad_lump_t *L = &wad->lumps[i];
        if (L->type == AETHER_WAD_TYPE_MIPTEX)  miptex_count++;
        else if (L->type == AETHER_WAD_TYPE_PALETTE) palette_count++;
        else other_count++;

        if (shown < 20) {
            aether_log(AETHER_LOG_INFO, "wad",
                       "  [%u] %-15s type=0x%02X size=%d",
                       i, L->name, L->type, L->size);
            shown++;
        }
    }
    if (wad->lump_count > shown)
        aether_log(AETHER_LOG_INFO, "wad", "  ... (+%u more)",
                   wad->lump_count - shown);
    aether_log(AETHER_LOG_INFO, "wad",
               "  summary: miptex=%u palette=%u other=%u",
               miptex_count, palette_count, other_count);
    aether_log(AETHER_LOG_INFO, "wad", "================");
}

/* ---------- BSP embedded miptex ---------- */
bool aether_bsp_miptex_info(const aether_bsp_t *bsp, u32 index,
                            aether_miptex_info_t *out) {
    if (!bsp || !out) return false;
    size_t lump_size = 0;
    const u8 *lump = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_TEXTURES);
    lump_size = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_TEXTURES);
    if (!lump || lump_size < 4) return false;

    u32 count = rd_u32(lump);
    if (index >= count) return false;

    i32 rel = rd_i32(lump + 4 + index * 4);
    if (rel < 0 || (u32)rel + 40 > lump_size) return false;

    const u8 *base = lump + rel;
    u32 w   = rd_u32(base + 16);
    u32 h   = rd_u32(base + 20);
    u32 off0= rd_u32(base + 24);
    if (w == 0 || h == 0) return false;
    u32 mip0 = w * h;
    if ((u32)rel + off0 + mip0 > lump_size) return false;

    memset(out, 0, sizeof *out);
    memcpy(out->name, base, 16);
    out->name[15] = '\0';
    out->width      = w;
    out->height     = h;
    out->pixel_data = base + off0;
    out->pixel_size = mip0;
    return true;
}

void aether_bsp_miptex_dump(const aether_bsp_t *bsp) {
    if (!bsp) return;
    u32 count = aether_bsp_miptex_count(bsp);
    aether_log(AETHER_LOG_INFO, "bsp-tex", "===== BSP embedded miptex =====");
    aether_log(AETHER_LOG_INFO, "bsp-tex", "  count: %u", count);
    u32 shown = count > 15 ? 15 : count;
    for (u32 i = 0; i < shown; ++i) {
        aether_miptex_info_t info;
        if (aether_bsp_miptex_info(bsp, i, &info)) {
            aether_log(AETHER_LOG_INFO, "bsp-tex",
                       "  [%u] %-15s %ux%u (%u bytes)",
                       i, info.name, info.width, info.height, info.pixel_size);
        }
    }
    if (count > shown)
        aether_log(AETHER_LOG_INFO, "bsp-tex", "  ... (+%u more)", count - shown);
    aether_log(AETHER_LOG_INFO, "bsp-tex", "===============================");
}
