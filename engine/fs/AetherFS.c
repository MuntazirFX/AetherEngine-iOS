#include "AetherFS.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct aether_fs {
    char          game_root[512];
    aether_pak_t *paks[AETHER_FS_MAX_PAKS];
    u32           pak_count;
    char          resolve_cache[512];
};

aether_fs_t *aether_fs_create(const char *game_root) {
    if (!game_root) return NULL;
    aether_fs_t *fs = (aether_fs_t*)calloc(1, sizeof *fs);
    if (!fs) return NULL;
    aether_str_copy(fs->game_root, sizeof fs->game_root, game_root);
    aether_log(AETHER_LOG_INFO, "fs", "vfs created: root='%s'", fs->game_root);
    return fs;
}

void aether_fs_destroy(aether_fs_t *fs) {
    if (!fs) return;
    aether_fs_unmount_all(fs);
    free(fs);
}

aether_result_t aether_fs_mount_pak(aether_fs_t *fs, const char *pak_path) {
    if (!fs || !pak_path) return AETHER_ERR_INVALID_ARG;
    if (fs->pak_count >= AETHER_FS_MAX_PAKS) return AETHER_ERR_OUT_OF_MEM;

    aether_pak_t *p = aether_pak_open(pak_path);
    if (!p) return AETHER_ERR_IO;

    fs->paks[fs->pak_count++] = p;
    aether_log(AETHER_LOG_INFO, "fs", "mounted pak: %s", pak_path);
    return AETHER_OK;
}

void aether_fs_unmount_all(aether_fs_t *fs) {
    if (!fs) return;
    for (u32 i = 0; i < fs->pak_count; ++i) {
        aether_pak_close(fs->paks[i]);
        fs->paks[i] = NULL;
    }
    fs->pak_count = 0;
}

aether_result_t aether_fs_auto_mount_paks(aether_fs_t *fs) {
    if (!fs) return AETHER_ERR_INVALID_ARG;

    u32 mounted = 0;
    for (u32 i = 0; i < AETHER_FS_MAX_PAKS; ++i) {
        char p[600];
        snprintf(p, sizeof p, "%s/pak%u.pak", fs->game_root, i);
        if (!aether_file_exists(p)) {
            if (i == 0) {
                aether_log(AETHER_LOG_WARN, "fs",
                           "no pak0.pak under %s — loose files only", fs->game_root);
                return AETHER_ERR_NOT_FOUND;
            }
            break;
        }
        if (aether_fs_mount_pak(fs, p) == AETHER_OK) mounted++;
    }
    aether_log(AETHER_LOG_INFO, "fs", "auto-mounted %u pak(s)", mounted);
    return mounted ? AETHER_OK : AETHER_ERR_NOT_FOUND;
}

static const aether_pak_entry_t *find_in_paks(aether_fs_t *fs,
                                              const char *vpath,
                                              aether_pak_t **out_pak) {
    /* Reverse mount order: latest PAK wins (matches GoldSrc behaviour). */
    for (i32 i = (i32)fs->pak_count - 1; i >= 0; --i) {
        const aether_pak_entry_t *e = aether_pak_find(fs->paks[i], vpath);
        if (e) {
            if (out_pak) *out_pak = fs->paks[i];
            return e;
        }
    }
    return NULL;
}

bool aether_fs_exists(aether_fs_t *fs, const char *vpath) {
    if (!fs || !vpath) return false;
    if (find_in_paks(fs, vpath, NULL)) return true;

    char disk[600];
    snprintf(disk, sizeof disk, "%s/%s", fs->game_root, vpath);
    return aether_file_exists(disk);
}

u32 aether_fs_read_file(aether_fs_t *fs, const char *vpath,
                        u8 *out_buffer, u32 out_capacity) {
    if (!fs || !vpath) return 0;

    aether_pak_t *pak = NULL;
    const aether_pak_entry_t *e = find_in_paks(fs, vpath, &pak);

    if (e) {
        if (!out_buffer) return e->size;   /* query */
        if (out_capacity < e->size) return 0;
        return aether_pak_read_file(pak, e, out_buffer, out_capacity);
    }

    /* Fallback: disk */
    char disk[600];
    snprintf(disk, sizeof disk, "%s/%s", fs->game_root, vpath);
    FILE *fp = fopen(disk, "rb");
    if (!fp) return 0;

    if (!out_buffer) {
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fclose(fp);
        return (u32)(sz < 0 ? 0 : sz);
    }

    size_t got = fread(out_buffer, 1, out_capacity, fp);
    fclose(fp);
    return (u32)got;
}

const char *aether_fs_resolve(const aether_fs_t *fs, const char *vpath) {
    if (!fs || !vpath) return NULL;
    /* Non-const cast only for cache write; caller treats result as read-only. */
    aether_fs_t *m = (aether_fs_t*)fs;

    if (find_in_paks(m, vpath, NULL)) {
        snprintf(m->resolve_cache, sizeof m->resolve_cache, "pak:%s", vpath);
        return m->resolve_cache;
    }
    snprintf(m->resolve_cache, sizeof m->resolve_cache, "%s/%s", fs->game_root, vpath);
    return m->resolve_cache;
}

const char *aether_fs_game_root(const aether_fs_t *fs) {
    return fs ? fs->game_root : NULL;
}
