/* AetherFS.h — Multi-root virtual filesystem.
 * Supports multiple search roots (like Xash3D/ GoldSrc).
 * Search order: last-added root has highest priority (reverse-mount).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_FS_H
#define AETHER_FS_H

#include "../core/AetherCore.h"
#include "AetherPak.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_FS_MAX_ROOTS 8
#define AETHER_FS_MAX_PAKS_PER_ROOT 4
#define AETHER_FS_MAX_PAKS (AETHER_FS_MAX_ROOTS * AETHER_FS_MAX_PAKS_PER_ROOT)

typedef struct aether_fs aether_fs_t;

aether_fs_t *aether_fs_create(const char *basedir);
void         aether_fs_destroy(aether_fs_t *fs);

/* Add a search root. Loose files inside are found by vpath.
 * Returns AETHER_OK on success, AETHER_ERR_ALREADY if root exists.
 */
aether_result_t aether_fs_add_root(aether_fs_t *fs, const char *root);

/* Remove all roots. */
void aether_fs_clear_roots(aether_fs_t *fs);

/* Mount a PAK file that belongs to a specific root. */
aether_result_t aether_fs_mount_pak(aether_fs_t *fs, const char *root,
                                    const char *pak_path);

/* Auto-detect and mount pak0.pak, pak1.pak, ... from a root.
 * Called after aether_fs_add_root(). */
aether_result_t aether_fs_auto_mount_paks(aether_fs_t *fs, const char *root);

/* Read a virtual file from any root.
 * Returns bytes read or 0 on failure. */
u32 aether_fs_read_file(aether_fs_t *fs, const char *vpath,
                        u8 *out_buffer, u32 out_capacity);

/* Check if vpath exists in any root. */
bool aether_fs_exists(aether_fs_t *fs, const char *vpath);

/* Return physical path (disk or pak name) — static buffer. */
const char *aether_fs_resolve(aether_fs_t *fs, const char *vpath);

/* Diagnostics */
u32         aether_fs_root_count(const aether_fs_t *fs);
const char *aether_fs_root_at(const aether_fs_t *fs, u32 index);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_FS_H */
