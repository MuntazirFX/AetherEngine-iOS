/* AetherFS.c — Multi-root VFS implementation.
 * Search order: PAKs (reverse-mount) -> loose files (reverse-root).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct aether_fs_root {
    char        path[512];
    bool        used;
    aether_pak_t *paks[AETHER_FS_MAX_PAKS_PER_ROOT];
    u32          pak_count;
} aether_fs_root_t;

struct aether_fs {
    aether_fs_root_t roots[AETHER_FS_MAX_ROOTS];
    u32              root_count;
    char             resolve_cache[512];
};

aether_fs_t *aether_fs_create(const char *basedir) {
    if (!basedir) return NULL;
    aether_fs_t *fs = (aether_fs_t*)calloc(1, sizeof *fs);
    if (!fs) return NULL;
    aether_log(AETHER_LOG_INFO, "fs", "vfs created: basedir='%s'", basedir);
    return fs;
}

void aether_fs_destroy(aether_fs_t *fs) {
    if (!fs) return;
    aether_fs_clear_roots(fs);
    free(fs);
}

aether_result_t aether_fs_add_root(aether_fs_t *fs, const char *root) {
    if (!fs || !root) return AETHER_ERR_INVALID_ARG;
    if (fs->root_count >= AETHER_FS_MAX_ROOTS) return AETHER_ERR_OUT_OF_MEM;

    /* Skip if already present. */
    for (u32 i = 0; i < fs->root_count; ++i) {
        if (aether_str_eq(fs->roots[i].path, root)) {
            aether_log(AETHER_LOG_DEBUG, "fs", "root already present: %s", root);
            return AETHER_ERR_ALREADY;
        }
    }

    aether_fs_root_t *r = &fs->roots[fs->root_count];
    aether_str_copy(r->path, sizeof r->path, root);
    r->used = true;
    r->pak_count = 0;
    fs->root_count++;
    aether_log(AETHER_LOG_INFO, "fs", "added root [%u]: %s", fs->root_count - 1, root);
    return AETHER_OK;
}

void aether_fs_clear_roots(aether_fs_t *fs) {
    if (!fs) return;
    for (u32 i = 0; i < fs->root_count; ++i) {
        for (u32 j = 0; j < fs->roots[i].pak_count; ++j) {
            aether_pak_close(fs->roots[i].paks[j]);
        }
        fs->roots[i].pak_count = 0;
        fs->roots[i].used = false;
    }
    fs->root_count = 0;
}

aether_result_t aether_fs_mount_pak(aether_fs_t *fs, const char *root,
                                    const char *pak_path) {
    if (!fs || !root || !pak_path) return AETHER_ERR_INVALID_ARG;

    /* Find the root */
    aether_fs_root_t *r = NULL;
    for (u32 i = 0; i < fs->root_count; ++i) {
        if (aether_str_eq(fs->roots[i].path, root)) { r = &fs->roots[i]; break; }
    }
    if (!r) return AETHER_ERR_NOT_FOUND;
    if (r->pak_count >= AETHER_FS_MAX_PAKS_PER_ROOT) return AETHER_ERR_OUT_OF_MEM;

    aether_pak_t *p = aether_pak_open(pak_path);
    if (!p) return AETHER_ERR_IO;

    r->paks[r->pak_count++] = p;
    aether_log(AETHER_LOG_INFO, "fs", "mounted pak: %s (root=%s)", pak_path, root);
    return AETHER_OK;
}

aether_result_t aether_fs_auto_mount_paks(aether_fs_t *fs, const char *root) {
    if (!fs || !root) return AETHER_ERR_INVALID_ARG;
    u32 mounted = 0;
    for (u32 i = 0; i < AETHER_FS_MAX_PAKS_PER_ROOT; ++i) {
        char p[600];
        snprintf(p, sizeof p, "%s/pak%u.pak", root, i);
        if (!aether_file_exists(p)) {
            if (i == 0) {
                aether_log(AETHER_LOG_WARN, "fs",
                           "no pak0.pak under %s — loose files only", root);
                return AETHER_ERR_NOT_FOUND;
            }
            break;
        }
        if (aether_fs_mount_pak(fs, root, p) == AETHER_OK) mounted++;
    }
    aether_log(AETHER_LOG_INFO, "fs", "auto-mounted %u pak(s) from %s", mounted, root);
    return mounted ? AETHER_OK : AETHER_ERR_NOT_FOUND;
}

static const aether_pak_entry_t *find_in_all_paks(aether_fs_t *fs,
                                                   const char *vpath,
                                                   aether_pak_t **out_pak) {
    /* Reverse iteration: last root/pak has highest priority (Xash3D behaviour). */
    for (i32 i = (i32)fs->root_count - 1; i >= 0; --i) {
        aether_fs_root_t *r = &fs->roots[i];
        for (i32 j = (i32)r->pak_count - 1; j >= 0; --j) {
            const aether_pak_entry_t *e = aether_pak_find(r->paks[j], vpath);
            if (e) {
                if (out_pak) *out_pak = r->paks[j];
                return e;
            }
        }
    }
    return NULL;
}

bool aether_fs_exists(aether_fs_t *fs, const char *vpath) {
    if (!fs || !vpath) return false;
    if (find_in_all_paks(fs, vpath, NULL)) return true;
    /* Disk fallback */
    for (i32 i = (i32)fs->root_count - 1; i >= 0; --i) {
        char disk[600];
        snprintf(disk, sizeof disk, "%s/%s", fs->roots[i].path, vpath);
        if (aether_file_exists(disk)) return true;
    }
    return false;
}

u32 aether_fs_read_file(aether_fs_t *fs, const char *vpath,
                        u8 *out_buffer, u32 out_capacity) {
    if (!fs || !vpath) return 0;
    aether_pak_t *pak = NULL;
    const aether_pak_entry_t *e = find_in_all_paks(fs, vpath, &pak);
    if (e) {
        if (!out_buffer) return e->size;
        if (out_capacity < e->size) return 0;
        return aether_pak_read_file(pak, e, out_buffer, out_capacity);
    }
    /* Disk fallback */
    for (i32 i = (i32)fs->root_count - 1; i >= 0; --i) {
        char disk[600];
        snprintf(disk, sizeof disk, "%s/%s", fs->roots[i].path, vpath);
        FILE *fp = fopen(disk, "rb");
        if (!fp) continue;
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
    return 0;
}

const char *aether_fs_resolve(aether_fs_t *fs, const char *vpath) {
    if (!fs || !vpath) return NULL;
    if (find_in_all_paks(fs, vpath, NULL)) {
        snprintf(fs->resolve_cache, sizeof fs->resolve_cache, "pak:%s", vpath);
        return fs->resolve_cache;
    }
    for (i32 i = (i32)fs->root_count - 1; i >= 0; --i) {
        char disk[600];
        snprintf(disk, sizeof disk, "%s/%s", fs->roots[i].path, vpath);
        if (aether_file_exists(disk)) {
            aether_str_copy(fs->resolve_cache, sizeof fs->resolve_cache, disk);
            return fs->resolve_cache;
        }
    }
    return NULL;
}

u32 aether_fs_root_count(const aether_fs_t *fs) { return fs ? fs->root_count : 0; }

const char *aether_fs_root_at(const aether_fs_t *fs, u32 index) {
    if (!fs || index >= fs->root_count) return NULL;
    return fs->roots[index].path;
}
