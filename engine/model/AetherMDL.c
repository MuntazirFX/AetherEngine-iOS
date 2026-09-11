/* AetherMDL.c — MDL v10 parser (STEP 16A).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* On-disk layout constants (little-endian, packed). */
#define HDR_SIZE        244
#define BONE_SIZE       112
#define BODYPART_SIZE   76
#define TEXTURE_SIZE    80

/* Header field offsets (bytes from start of file). */
#define OFF_ID          0
#define OFF_VERSION     4
#define OFF_NAME        8
#define OFF_LENGTH      72
#define OFF_EYE         76
#define OFF_MIN         88
#define OFF_MAX         100
#define OFF_BBMIN       112
#define OFF_BBMAX       124
#define OFF_FLAGS       136
#define OFF_NUMBONES    140
#define OFF_BONEIDX     144
#define OFF_NUMBC       148
#define OFF_BCIDX       152
#define OFF_NUMHITBOX   156
#define OFF_HITBOXIDX   160
#define OFF_NUMSEQ      164
#define OFF_SEQIDX      168
#define OFF_NUMSEQGRP   172
#define OFF_SEQGRPIDX   176
#define OFF_NUMTEX      180
#define OFF_TEXIDX      184
#define OFF_TEXDATAIDX  188
#define OFF_NUMSKINREF  192
#define OFF_NUMSKINFAM  196
#define OFF_SKINIDX     200
#define OFF_NUMBODY     204
#define OFF_BODYIDX     208
#define OFF_NUMATTACH   212
#define OFF_ATTACHIDX   216

struct aether_mdl {
    u8                  *raw;
    u32                  raw_size;
    bool                 valid;

    aether_mdl_info_t    info;

    aether_mdl_bone_t   *bones;
    aether_mdl_skin_t   *skins;
    aether_mdl_bodypart_t *bodyparts;
};

/* ---------- Little-endian readers ---------- */
static i32 rd_i32(const u8 *p) {
    return (i32)((u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24));
}
static f32 rd_f32(const u8 *p) {
    u32 v = (u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24);
    f32 f; memcpy(&f, &v, 4); return f;
}
static void rd_name(const u8 *p, char *out, size_t cap) {
    size_t i = 0;
    for (; i < cap - 1; ++i) {
        out[i] = (char)p[i];
        if (out[i] == 0) break;
    }
    out[i] = 0;
}

/* ---------- Loader ---------- */
static bool parse_mdl(aether_mdl_t *m) {
    if (m->raw_size < HDR_SIZE) return false;

    const u8 *h = m->raw;
    i32 id      = rd_i32(h + OFF_ID);
    i32 version = rd_i32(h + OFF_VERSION);

    if (id != AETHER_MDL_ID) {
        aether_log(AETHER_LOG_ERROR, "mdl", "bad magic (0x%08X)", (u32)id);
        return false;
    }
    if (version != AETHER_MDL_VERSION) {
        aether_log(AETHER_LOG_ERROR, "mdl", "unsupported version %d", version);
        return false;
    }

    rd_name(h + OFF_NAME, m->info.name, sizeof m->info.name);
    m->info.length       = rd_i32(h + OFF_LENGTH);
    m->info.eye_position[0] = rd_f32(h + OFF_EYE + 0);
    m->info.eye_position[1] = rd_f32(h + OFF_EYE + 4);
    m->info.eye_position[2] = rd_f32(h + OFF_EYE + 8);
    for (int i = 0; i < 3; ++i) {
        m->info.min[i]   = rd_f32(h + OFF_MIN   + i*4);
        m->info.max[i]   = rd_f32(h + OFF_MAX   + i*4);
        m->info.bbmin[i] = rd_f32(h + OFF_BBMIN + i*4);
        m->info.bbmax[i] = rd_f32(h + OFF_BBMAX + i*4);
    }
    m->info.bone_count        = rd_i32(h + OFF_NUMBONES);
    m->info.bodypart_count    = rd_i32(h + OFF_NUMBODY);
    m->info.texture_count     = rd_i32(h + OFF_NUMTEX);
    m->info.sequence_count    = rd_i32(h + OFF_NUMSEQ);
    m->info.skinref_count     = rd_i32(h + OFF_NUMSKINREF);
    m->info.skinfamily_count  = rd_i32(h + OFF_NUMSKINFAM);
    m->info.hitbox_count      = rd_i32(h + OFF_NUMHITBOX);
    m->info.attachment_count  = rd_i32(h + OFF_NUMATTACH);

    /* ---- Bones ---- */
    i32 bone_off = rd_i32(h + OFF_BONEIDX);
    if (m->info.bone_count > 0 && bone_off > 0) {
        m->bones = (aether_mdl_bone_t*)calloc(m->info.bone_count, sizeof(aether_mdl_bone_t));
        if (m->bones) {
            for (i32 i = 0; i < m->info.bone_count; ++i) {
                u32 off = (u32)bone_off + (u32)i * BONE_SIZE;
                if (off + BONE_SIZE > m->raw_size) break;
                const u8 *bp = m->raw + off;
                rd_name(bp, m->bones[i].name, sizeof m->bones[i].name);
                m->bones[i].parent = rd_i32(bp + 32);
            }
        }
    }

    /* ---- Textures (skins) ---- */
    i32 tex_off = rd_i32(h + OFF_TEXIDX);
    if (m->info.texture_count > 0 && tex_off > 0) {
        m->skins = (aether_mdl_skin_t*)calloc(m->info.texture_count, sizeof(aether_mdl_skin_t));
        if (m->skins) {
            for (i32 i = 0; i < m->info.texture_count; ++i) {
                u32 off = (u32)tex_off + (u32)i * TEXTURE_SIZE;
                if (off + TEXTURE_SIZE > m->raw_size) break;
                const u8 *tp = m->raw + off;
                rd_name(tp, m->skins[i].name, sizeof m->skins[i].name);
                m->skins[i].flags  = rd_i32(tp + 64);
                m->skins[i].width  = rd_i32(tp + 68);
                m->skins[i].height = rd_i32(tp + 72);
            }
        }
    }

    /* ---- Bodyparts ---- */
    i32 body_off = rd_i32(h + OFF_BODYIDX);
    if (m->info.bodypart_count > 0 && body_off > 0) {
        m->bodyparts = (aether_mdl_bodypart_t*)calloc(m->info.bodypart_count, sizeof(aether_mdl_bodypart_t));
        if (m->bodyparts) {
            for (i32 i = 0; i < m->info.bodypart_count; ++i) {
                u32 off = (u32)body_off + (u32)i * BODYPART_SIZE;
                if (off + BODYPART_SIZE > m->raw_size) break;
                const u8 *bp = m->raw + off;
                rd_name(bp, m->bodyparts[i].name, sizeof m->bodyparts[i].name);
                m->bodyparts[i].num_models = rd_i32(bp + 64);
            }
        }
    }

    m->valid = true;
    aether_log(AETHER_LOG_INFO, "mdl", "parsed '%s': bones=%d bodyparts=%d textures=%d",
               m->info.name, m->info.bone_count, m->info.bodypart_count, m->info.texture_count);
    return true;
}

aether_mdl_t *aether_mdl_load_from_memory(const u8 *data, u32 size, const char *name) {
    if (!data || size < HDR_SIZE) return NULL;
    aether_mdl_t *m = (aether_mdl_t*)calloc(1, sizeof *m);
    if (!m) return NULL;
    m->raw = (u8*)malloc(size);
    if (!m->raw) { free(m); return NULL; }
    memcpy(m->raw, data, size);
    m->raw_size = size;
    if (!parse_mdl(m)) {
        free(m->raw);
        free(m->bones);
        free(m->skins);
        free(m->bodyparts);
        free(m);
        return NULL;
    }
    aether_log(AETHER_LOG_INFO, "mdl", "loaded %s (%u bytes)",
               name ? name : "<memory>", size);
    return m;
}

aether_mdl_t *aether_mdl_load(const char *filepath) {
    if (!filepath) return NULL;
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= HDR_SIZE) { fclose(f); return NULL; }
    u8 *buf = (u8*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    fclose(f);
    aether_mdl_t *m = aether_mdl_load_from_memory(buf, (u32)sz, filepath);
    free(buf);
    return m;
}

void aether_mdl_free(aether_mdl_t *m) {
    if (!m) return;
    free(m->raw);
    free(m->bones);
    free(m->skins);
    free(m->bodyparts);
    free(m);
}

bool aether_mdl_is_valid(const aether_mdl_t *m) { return m && m->valid; }
const aether_mdl_info_t *aether_mdl_info(const aether_mdl_t *m) { return m ? &m->info : NULL; }

i32 aether_mdl_bone_count(const aether_mdl_t *m) { return m ? m->info.bone_count : 0; }
const aether_mdl_bone_t *aether_mdl_bone_at(const aether_mdl_t *m, i32 i) {
    if (!m || !m->bones || i < 0 || i >= m->info.bone_count) return NULL;
    return &m->bones[i];
}
i32 aether_mdl_skin_count(const aether_mdl_t *m) { return m ? m->info.texture_count : 0; }
const aether_mdl_skin_t *aether_mdl_skin_at(const aether_mdl_t *m, i32 i) {
    if (!m || !m->skins || i < 0 || i >= m->info.texture_count) return NULL;
    return &m->skins[i];
}
i32 aether_mdl_bodypart_count(const aether_mdl_t *m) { return m ? m->info.bodypart_count : 0; }
const aether_mdl_bodypart_t *aether_mdl_bodypart_at(const aether_mdl_t *m, i32 i) {
    if (!m || !m->bodyparts || i < 0 || i >= m->info.bodypart_count) return NULL;
    return &m->bodyparts[i];
}

void aether_mdl_dump(const aether_mdl_t *m) {
    if (!aether_mdl_is_valid(m)) {
        aether_log(AETHER_LOG_WARN, "mdl", "dump: invalid");
        return;
    }
    const aether_mdl_info_t *info = &m->info;
    aether_log(AETHER_LOG_INFO, "mdl", "=====================================");
    aether_log(AETHER_LOG_INFO, "mdl", "  name        : %s", info->name);
    aether_log(AETHER_LOG_INFO, "mdl", "  length      : %d bytes", info->length);
    aether_log(AETHER_LOG_INFO, "mdl", "  bbox min    : %.1f %.1f %.1f", info->min[0], info->min[1], info->min[2]);
    aether_log(AETHER_LOG_INFO, "mdl", "  bbox max    : %.1f %.1f %.1f", info->max[0], info->max[1], info->max[2]);
    aether_log(AETHER_LOG_INFO, "mdl", "  eye pos     : %.1f %.1f %.1f", info->eye_position[0], info->eye_position[1], info->eye_position[2]);
    aether_log(AETHER_LOG_INFO, "mdl", "  bones       : %d", info->bone_count);
    aether_log(AETHER_LOG_INFO, "mdl", "  bodyparts   : %d", info->bodypart_count);
    aether_log(AETHER_LOG_INFO, "mdl", "  textures    : %d", info->texture_count);
    aether_log(AETHER_LOG_INFO, "mdl", "  sequences   : %d", info->sequence_count);
    aether_log(AETHER_LOG_INFO, "mdl", "  skinrefs    : %d", info->skinref_count);
    aether_log(AETHER_LOG_INFO, "mdl", "  skinfams    : %d", info->skinfamily_count);
    aether_log(AETHER_LOG_INFO, "mdl", "  hitboxes    : %d", info->hitbox_count);
    aether_log(AETHER_LOG_INFO, "mdl", "  attachments : %d", info->attachment_count);

    /* Show first few textures */
    i32 shown = info->texture_count > 5 ? 5 : info->texture_count;
    for (i32 i = 0; i < shown; ++i) {
        const aether_mdl_skin_t *s = &m->skins[i];
        aether_log(AETHER_LOG_INFO, "mdl", "  tex[%d]: %-15s %dx%d",
                   i, s->name, s->width, s->height);
    }
    if (info->texture_count > shown)
        aether_log(AETHER_LOG_INFO, "mdl", "  … +%d more textures",
                   info->texture_count - shown);

    /* Show first few bones */
    i32 bshown = info->bone_count > 5 ? 5 : info->bone_count;
    for (i32 i = 0; i < bshown; ++i) {
        const aether_mdl_bone_t *b = &m->bones[i];
        aether_log(AETHER_LOG_INFO, "mdl", "  bone[%d]: %-20s parent=%d",
                   i, b->name, b->parent);
    }
    if (info->bone_count > bshown)
        aether_log(AETHER_LOG_INFO, "mdl", "  … +%d more bones",
                   info->bone_count - bshown);

    aether_log(AETHER_LOG_INFO, "mdl", "=====================================");
}
