/* AetherTexture.c — WAD3 + palette + atlas implementation (STEP 15B).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherTexture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct wad3_header {
    char ident[4];
    i32  numlumps;
    i32  infotableofs;
} wad3_header_t;

typedef struct wad3_lumpinfo {
    i32  filepos;
    i32  disksize;
    i32  size;
    u8   type;
    u8   compression;
    u8   pad1, pad2;
    char name[16];
} wad3_lumpinfo_t;
#pragma pack(pop)

struct aether_wad {
    u8       *raw;
    u32       raw_size;
    char      source[256];
    bool      valid;
    u32       lump_count;
    aether_wad_lump_t *lumps;
};

static u32 rd_u32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24);
}
static i32 rd_i32(const u8 *p) { return (i32)rd_u32(p); }

/* ============ WAD ============ */
aether_wad_t *aether_wad_load_from_memory(const u8 *data, u32 size, const char *name) {
    if (!data || size < 12) return NULL;
    if (memcmp(data, "WAD3", 4) != 0 && memcmp(data, "WAD2", 4) != 0) return NULL;

    aether_wad_t *w = (aether_wad_t*)calloc(1, sizeof *w);
    if (!w) return NULL;
    w->raw = (u8*)malloc(size);
    if (!w->raw) { free(w); return NULL; }
    memcpy(w->raw, data, size);
    w->raw_size = size;
    aether_str_copy(w->source, sizeof w->source, name ? name : "<memory>");

    const wad3_header_t *h = (const wad3_header_t*)w->raw;
    if (h->numlumps <= 0 || h->numlumps > AETHER_WAD_MAX_LUMPS) { free(w->raw); free(w); return NULL; }
    if ((u32)h->infotableofs + (u32)h->numlumps * 32 > w->raw_size) { free(w->raw); free(w); return NULL; }

    w->lump_count = (u32)h->numlumps;
    w->lumps = (aether_wad_lump_t*)calloc(w->lump_count, sizeof(aether_wad_lump_t));
    if (!w->lumps) { free(w->raw); free(w); return NULL; }

    const u8 *dir = w->raw + h->infotableofs;
    for (u32 i = 0; i < w->lump_count; ++i) {
        const wad3_lumpinfo_t *L = (const wad3_lumpinfo_t*)(dir + i*32);
        aether_wad_lump_t *out = &w->lumps[i];
        memcpy(out->name, L->name, 16); out->name[15] = '\0';
        out->file_pos = L->filepos; out->disk_size = L->disksize;
        out->size = L->size; out->type = L->type; out->compression = L->compression;
    }
    w->valid = true;
    aether_log(AETHER_LOG_INFO, "wad", "loaded %s: %u lumps", w->source, w->lump_count);
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
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
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

void aether_wad_dump(const aether_wad_t *wad) {
    if (!aether_wad_is_valid(wad)) return;
    u32 miptex = 0, palette = 0, other = 0;
    for (u32 i = 0; i < wad->lump_count; ++i) {
        const aether_wad_lump_t *L = &wad->lumps[i];
        if (L->type == AETHER_WAD_TYPE_MIPTEX) miptex++;
        else if (L->type == AETHER_WAD_TYPE_PALETTE) palette++;
        else other++;
    }
    aether_log(AETHER_LOG_INFO, "wad", "%s: %u lumps (miptex=%u palette=%u other=%u)",
               wad->source, wad->lump_count, miptex, palette, other);
}

/* ============ Palette ============ */
bool aether_palette_from_wad(aether_palette_t *out, const aether_wad_t *wad) {
    if (!out || !wad) return false;
    out->loaded = false;
    for (u32 i = 0; i < wad->lump_count; ++i) {
        const aether_wad_lump_t *L = &wad->lumps[i];
        if (L->type != AETHER_WAD_TYPE_PALETTE) continue;
        if (L->file_pos < 0 || (u32)L->file_pos + 768 > wad->raw_size) return false;
        memcpy(out->rgb, wad->raw + L->file_pos, 768);
        out->loaded = true;
        aether_log(AETHER_LOG_INFO, "palette", "loaded palette from %s (%s)",
                   wad->source, L->name);
        return true;
    }
    return false;
}

void aether_palette_default(aether_palette_t *out) {
    if (!out) return;
    for (int i = 0; i < 256; ++i) {
        out->rgb[i][0] = (u8)i;
        out->rgb[i][1] = (u8)i;
        out->rgb[i][2] = (u8)i;
    }
    out->loaded = true;
}

/* ============ BSP miptex ============ */
bool aether_bsp_miptex_info(const aether_bsp_t *bsp, u32 index,
                            aether_miptex_info_t *out) {
    if (!bsp || !out) return false;
    size_t lump_size = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_TEXTURES);
    const u8 *lump = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_TEXTURES);
    if (!lump || lump_size < 4) return false;

    u32 count = rd_u32(lump);
    if (index >= count) return false;

    i32 rel = rd_i32(lump + 4 + index * 4);
    if (rel < 0 || (u32)rel + 40 > lump_size) return false;

    const u8 *base = lump + rel;
    u32 w   = rd_u32(base + 16);
    u32 h   = rd_u32(base + 20);
    u32 off0= rd_u32(base + 24);
    if (w == 0 || h == 0 || w > 1024 || h > 1024) return false;
    u32 mip0 = w * h;
    if ((u32)rel + off0 + mip0 > lump_size) return false;

    memset(out, 0, sizeof *out);
    memcpy(out->name, base, 16); out->name[15] = '\0';
    out->width = w; out->height = h;
    out->pixel_data = base + off0;
    out->pixel_size = mip0;
    return true;
}

void aether_bsp_miptex_dump(const aether_bsp_t *bsp) {
    if (!bsp) return;
    u32 count = aether_bsp_miptex_count(bsp);
    aether_log(AETHER_LOG_INFO, "bsp-tex", "BSP embedded miptex: %u", count);
    u32 shown = count > 10 ? 10 : count;
    for (u32 i = 0; i < shown; ++i) {
        aether_miptex_info_t info;
        if (aether_bsp_miptex_info(bsp, i, &info)) {
            aether_log(AETHER_LOG_INFO, "bsp-tex", "  [%u] %-15s %ux%u",
                       i, info.name, info.width, info.height);
        }
    }
}

/* ============ Atlas ============ */
static void write_rgba(u8 *dst, u32 dst_w, u32 dst_h,
                       u32 x0, u32 y0, u32 w, u32 h,
                       const u8 *indexed, const aether_palette_t *pal) {
    for (u32 y = 0; y < h; ++y) {
        if (y0 + y >= dst_h) break;
        for (u32 x = 0; x < w; ++x) {
            if (x0 + x >= dst_w) break;
            u8 idx = indexed[y * w + x];
            u8 *p = dst + ((y0 + y) * dst_w + (x0 + x)) * 4;
            p[0] = pal->rgb[idx][0];
            p[1] = pal->rgb[idx][1];
            p[2] = pal->rgb[idx][2];
            p[3] = 255;
        }
    }
}

aether_texture_atlas_t *aether_texture_atlas_build(const aether_bsp_t *bsp,
                                                    const aether_palette_t *palette) {
    if (!bsp) return NULL;
    const aether_palette_t *pal = palette;
    aether_palette_t fallback;
    if (!pal || !pal->loaded) { aether_palette_default(&fallback); pal = &fallback; }

    u32 count = aether_bsp_miptex_count(bsp);
    if (count == 0) { aether_log(AETHER_LOG_WARN, "atlas", "no miptex"); return NULL; }

    u32 atlas_w = 2048, atlas_h = 2048;
    u8 *rgba = (u8*)calloc((size_t)atlas_w * atlas_h * 4, 1);
    if (!rgba) return NULL;

    aether_texture_atlas_t *a = (aether_texture_atlas_t*)calloc(1, sizeof *a);
    if (!a) { free(rgba); return NULL; }
    a->rgba = rgba;
    a->width = atlas_w;
    a->height = atlas_h;

    /* Shelf packing */
    u32 cursor_x = 0, cursor_y = 0, row_h = 0;

    for (u32 i = 0; i < count && a->slot_count < AETHER_ATLAS_MAX_SLOTS; ++i) {
        aether_miptex_info_t info;
        if (!aether_bsp_miptex_info(bsp, i, &info)) continue;

        /* Pad to next power of two-ish for UV stability (16-byte min) */
        u32 tw = info.width;
        u32 th = info.height;
        if (tw < 16) tw = 16;
        if (th < 16) th = 16;

        /* Wrap to next row if doesn't fit */
        if (cursor_x + tw > atlas_w) {
            cursor_x = 0;
            cursor_y += row_h;
            row_h = 0;
        }
        if (cursor_y + th > atlas_h) {
            aether_log(AETHER_LOG_WARN, "atlas", "atlas full at slot %u", a->slot_count);
            break;
        }

        write_rgba(rgba, atlas_w, atlas_h,
                   cursor_x, cursor_y, info.width, info.height,
                   info.pixel_data, pal);

        aether_texture_slot_t *s = &a->slots[a->slot_count];
        aether_str_copy(s->name, sizeof s->name, info.name);
        s->x = cursor_x; s->y = cursor_y;
        s->w = info.width; s->h = info.height;
        a->slot_count++;

        cursor_x += tw;
        if (th > row_h) row_h = th;
    }

    aether_log(AETHER_LOG_INFO, "atlas",
               "built %ux%u, %u slots", atlas_w, atlas_h, a->slot_count);
    return a;
}

void aether_texture_atlas_free(aether_texture_atlas_t *a) {
    if (!a) return;
    free(a->rgba);
    free(a);
}

void aether_texture_atlas_dump(const aether_texture_atlas_t *a) {
    if (!a) return;
    aether_log(AETHER_LOG_INFO, "atlas", "===== ATLAS %ux%u, %u slots =====",
               a->width, a->height, a->slot_count);
    u32 shown = a->slot_count > 15 ? 15 : a->slot_count;
    for (u32 i = 0; i < shown; ++i) {
        const aether_texture_slot_t *s = &a->slots[i];
        aether_log(AETHER_LOG_INFO, "atlas", "  [%u] %-15s @ (%u,%u) %ux%u",
                   i, s->name, s->x, s->y, s->w, s->h);
    }
}

i32 aether_texture_atlas_find(const aether_texture_atlas_t *a,
                               const aether_bsp_t *bsp, u32 miptex_idx) {
    if (!a || !bsp) return -1;
    aether_miptex_info_t info;
    if (!aether_bsp_miptex_info(bsp, miptex_idx, &info)) return -1;
    for (u32 i = 0; i < a->slot_count; ++i) {
        if (aether_str_eq(a->slots[i].name, info.name)) return (i32)i;
    }
    return -1;
}
