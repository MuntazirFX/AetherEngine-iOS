#include "AetherPak.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* On-disk PAK header (little-endian) */
typedef struct {
    u32 magic;         /* "PACK" */
    u32 dir_offset;
    u32 dir_length;
} pak_header_t;

/* On-disk PAK directory entry */
typedef struct {
    char name[AETHER_PAK_MAX_NAME];
    u32  offset;
    u32  size;
} pak_dir_entry_t;

struct aether_pak {
    char              path[512];
    FILE             *fp;
    u32               entry_count;
    aether_pak_entry_t *entries;
    u32               dir_offset;
    u32               dir_length;
};

static u32 read_u32_le(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

aether_pak_t *aether_pak_open(const char *filepath) {
    if (!filepath) return NULL;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        aether_log(AETHER_LOG_ERROR, "pak", "cannot open %s", filepath);
        return NULL;
    }

    u8 hdr[12];
    if (fread(hdr, 1, sizeof hdr, fp) != sizeof hdr) {
        fclose(fp); return NULL;
    }
    if (read_u32_le(hdr) != AETHER_PAK_MAGIC) {
        aether_log(AETHER_LOG_ERROR, "pak", "%s: bad magic", filepath);
        fclose(fp); return NULL;
    }

    aether_pak_t *p = (aether_pak_t*)calloc(1, sizeof *p);
    if (!p) { fclose(fp); return NULL; }

    aether_str_copy(p->path, sizeof p->path, filepath);
    p->fp         = fp;
    p->dir_offset = read_u32_le(hdr + 4);
    p->dir_length = read_u32_le(hdr + 8);
    p->entry_count = p->dir_length / (u32)sizeof(pak_dir_entry_t);
    p->entries    = (aether_pak_entry_t*)calloc(p->entry_count, sizeof *p->entries);
    if (!p->entries) { fclose(fp); free(p); return NULL; }

    if (fseek(fp, (long)p->dir_offset, SEEK_SET) != 0) {
        fclose(fp); free(p->entries); free(p); return NULL;
    }

    for (u32 i = 0; i < p->entry_count; ++i) {
        pak_dir_entry_t de;
        if (fread(&de, 1, sizeof de, fp) != sizeof de) break;
        aether_str_copy(p->entries[i].name, sizeof p->entries[i].name, de.name);
        p->entries[i].offset = de.offset;
        p->entries[i].size   = de.size;
    }

    aether_log(AETHER_LOG_INFO, "pak", "opened %s (%u entries)",
               p->path, p->entry_count);
    return p;
}

void aether_pak_close(aether_pak_t *p) {
    if (!p) return;
    if (p->fp) fclose(p->fp);
    free(p->entries);
    free(p);
}

u32 aether_pak_entry_count(const aether_pak_t *p) {
    return p ? p->entry_count : 0;
}

const aether_pak_entry_t *aether_pak_entry_at(const aether_pak_t *p, u32 index) {
    if (!p || index >= p->entry_count) return NULL;
    return &p->entries[index];
}

static int ci_strcmp(const char *a, const char *b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
        a++; b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

const aether_pak_entry_t *aether_pak_find(const aether_pak_t *p, const char *name) {
    if (!p || !name) return NULL;
    for (u32 i = 0; i < p->entry_count; ++i) {
        if (ci_strcmp(p->entries[i].name, name) == 0) return &p->entries[i];
    }
    return NULL;
}

u32 aether_pak_read_file(const aether_pak_t *p, const aether_pak_entry_t *entry,
                         u8 *out_buffer, u32 out_size) {
    if (!p || !entry || !out_buffer || out_size < entry->size) return 0;
    if (fseek(p->fp, (long)entry->offset, SEEK_SET) != 0) return 0;
    size_t got = fread(out_buffer, 1, entry->size, p->fp);
    return (u32)got;
}
