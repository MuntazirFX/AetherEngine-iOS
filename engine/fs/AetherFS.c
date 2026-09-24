/* AetherFS.c — Multi-root VFS implementation.
 * Search order: PAKs (reverse-mount) -> loose files (reverse-root).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct aether_fs_root {
    char        path[512];
    bool        used;
    aether_pak_t *paks[AETHER_FS_MAX_PAKS_PER_ROOT];
    u32          pak_count;
} aether_fs_root_t;

struct aether_fs {
    char             basedir[512];
    aether_fs_root_t roots[AETHER_FS_MAX_ROOTS];
    u32              root_count;
    char             resolve_cache[512];
};

aether_fs_t *aether_fs_create(const char *basedir) {
    if (!basedir) return NULL;
    aether_fs_t *fs = (aether_fs_t*)calloc(1, sizeof *fs);
    if (!fs) return NULL;
    aether_str_copy(fs->basedir, sizeof fs->basedir, basedir);
    aether_log(AETHER_LOG_INFO, "fs", "vfs created: basedir='%s'", basedir);
    return fs;
}

const char *aether_fs_basedir(const aether_fs_t *fs) {
    return fs ? fs->basedir : NULL;
}

aether_result_t aether_fs_setup_game(aether_fs_t *fs, const char *basedir,
                                     const char *gamedir) {
    if (!fs || !basedir || !gamedir || !gamedir[0]) return AETHER_ERR_INVALID_ARG;
    aether_str_copy(fs->basedir, sizeof fs->basedir, basedir);
    aether_fs_clear_roots(fs);

    char valve_dir[600];
    snprintf(valve_dir, sizeof valve_dir, "%s/valve", basedir);
    if (aether_fs_add_root(fs, valve_dir) == AETHER_OK)
        (void)aether_fs_auto_mount_paks(fs, valve_dir);

    if (!aether_str_eq(gamedir, "valve")) {
        char gd[600];
        snprintf(gd, sizeof gd, "%s/%s", basedir, gamedir);
        if (aether_fs_add_root(fs, gd) == AETHER_OK)
            (void)aether_fs_auto_mount_paks(fs, gd);
    }

    aether_log(AETHER_LOG_INFO, "fs",
               "setup_game basedir='%s' gamedir='%s' roots=%u",
               basedir, gamedir, fs->root_count);
    return AETHER_OK;
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

/* ---------- Documents/AetherEngine/<gamedir> FS mount (batch21) ---------- */
static int fs_mkdir_p(const char *path) {
    if (!path || !path[0]) return 0;
    if (aether_dir_exists(path)) return 1;
    if (mkdir(path, 0755) == 0) return 1;
    if (errno == EEXIST) return 1;
    return 0;
}

void aether_fs_documents_mount_init(aether_fs_documents_mount_t *m) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
}

int aether_fs_documents_gamedir_path(const char *documents_root, const char *gamedir,
                                     char *out_engine, u32 engine_cap,
                                     char *out_gamedir, u32 gamedir_cap) {
    if (!documents_root || !gamedir || !gamedir[0]) return 0;
    if (out_engine && engine_cap > 0)
        snprintf(out_engine, engine_cap, "%s/%s", documents_root, AETHER_FS_DOCUMENTS_ENGINE_DIR);
    if (out_gamedir && gamedir_cap > 0)
        snprintf(out_gamedir, gamedir_cap, "%s/%s/%s", documents_root,
                 AETHER_FS_DOCUMENTS_ENGINE_DIR, gamedir);
    return 1;
}

int aether_fs_ensure_documents_layout(const char *documents_root, const char *gamedir,
                                      aether_fs_documents_mount_t *out) {
    if (out) aether_fs_documents_mount_init(out);
    if (!documents_root || !gamedir || !gamedir[0]) return 0;
    char engine[512], gd[512], valve[600], maps[600], saves[600], gdmaps[600];
    if (!aether_fs_documents_gamedir_path(documents_root, gamedir,
                                          engine, sizeof engine, gd, sizeof gd))
        return 0;
    if (!fs_mkdir_p(documents_root)) return 0;
    if (!fs_mkdir_p(engine)) return 0;
    snprintf(valve, sizeof valve, "%s/valve", engine);
    if (!fs_mkdir_p(valve)) return 0;
    if (!aether_str_eq(gamedir, "valve")) {
        if (!fs_mkdir_p(gd)) return 0;
    }
    snprintf(maps, sizeof maps, "%s/maps", engine);
    snprintf(saves, sizeof saves, "%s/saves", engine);
    snprintf(gdmaps, sizeof gdmaps, "%s/maps",
             aether_str_eq(gamedir, "valve") ? valve : gd);
    (void)fs_mkdir_p(maps);
    (void)fs_mkdir_p(saves);
    (void)fs_mkdir_p(gdmaps);
    if (out) {
        aether_str_copy(out->documents_root, sizeof out->documents_root, documents_root);
        aether_str_copy(out->engine_root, sizeof out->engine_root, engine);
        aether_str_copy(out->gamedir_root, sizeof out->gamedir_root,
                        aether_str_eq(gamedir, "valve") ? valve : gd);
        aether_str_copy(out->gamedir, sizeof out->gamedir, gamedir);
        out->layout_ensured = true;
        out->valid = true;
    }
    return 1;
}

aether_result_t aether_fs_mount_documents_gamedir(aether_fs_t *fs,
                                                  const char *documents_root,
                                                  const char *gamedir,
                                                  aether_fs_documents_mount_t *out) {
    if (out) aether_fs_documents_mount_init(out);
    if (!fs || !documents_root || !gamedir || !gamedir[0]) return AETHER_ERR_INVALID_ARG;
    aether_fs_documents_mount_t local;
    aether_fs_documents_mount_t *m = out ? out : &local;
    if (!aether_fs_ensure_documents_layout(documents_root, gamedir, m))
        return AETHER_ERR_IO;
    aether_result_t r = aether_fs_setup_game(fs, m->engine_root, gamedir);
    if (r != AETHER_OK) return r;
    m->roots_mounted = aether_fs_root_count(fs);
    m->valve_mounted = false;
    m->gamedir_mounted = false;
    for (u32 i = 0; i < m->roots_mounted; ++i) {
        const char *root = aether_fs_root_at(fs, i);
        if (!root) continue;
        if (strstr(root, "/valve")) m->valve_mounted = true;
        if (!aether_str_eq(gamedir, "valve") && strstr(root, gamedir))
            m->gamedir_mounted = true;
        if (aether_str_eq(gamedir, "valve") && strstr(root, "/valve"))
            m->gamedir_mounted = true;
    }
    m->valid = (m->roots_mounted > 0);
    aether_log(AETHER_LOG_INFO, "fs",
               "documents mount engine='%s' gamedir='%s' roots=%u",
               m->engine_root, gamedir, m->roots_mounted);
    return m->valid ? AETHER_OK : AETHER_ERR_NOT_FOUND;
}

u32 aether_fs_documents_write_marker(const char *documents_root, const char *gamedir,
                                     const char *relpath, const void *data, u32 size) {
    if (!documents_root || !gamedir || !relpath || !data || size == 0) return 0;
    aether_fs_documents_mount_t m;
    if (!aether_fs_ensure_documents_layout(documents_root, gamedir, &m)) return 0;
    char path[700];
    snprintf(path, sizeof path, "%s/%s", m.gamedir_root, relpath);
    /* Ensure parent of relpath (one level) exists when relpath has a slash. */
    const char *slash = strrchr(relpath, '/');
    if (slash && slash != relpath) {
        char parent[700];
        size_t plen = (size_t)(slash - relpath);
        if (plen + 1 < sizeof parent) {
            snprintf(parent, sizeof parent, "%s/%.*s", m.gamedir_root, (int)plen, relpath);
            (void)fs_mkdir_p(parent);
        }
    }
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;
    size_t w = fwrite(data, 1, size, fp);
    fclose(fp);
    return (u32)w;
}
