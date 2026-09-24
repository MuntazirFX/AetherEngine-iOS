/* AetherFS.h — Multi-root virtual filesystem.
 * Supports multiple search roots (like Xash3D/ GoldSrc).
 * Search order: last-added root has highest priority (reverse-mount).
 * AetherEngine-iOS · Clean-room. No game assets bundled.
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

const char *aether_fs_basedir(const aether_fs_t *fs);

/* Add a search root. Loose files inside are found by vpath. */
aether_result_t aether_fs_add_root(aether_fs_t *fs, const char *root);
void aether_fs_clear_roots(aether_fs_t *fs);

aether_result_t aether_fs_mount_pak(aether_fs_t *fs, const char *root,
                                    const char *pak_path);
aether_result_t aether_fs_auto_mount_paks(aether_fs_t *fs, const char *root);

/* Resolve Documents layout without bundling HL assets:
 * mounts basedir/valve then basedir/<gamedir> (if not valve).
 * Empty dirs are fine — callers fall back to synthetic BSP. */
aether_result_t aether_fs_setup_game(aether_fs_t *fs, const char *basedir,
                                     const char *gamedir);

u32 aether_fs_read_file(aether_fs_t *fs, const char *vpath,
                        u8 *out_buffer, u32 out_capacity);
bool aether_fs_exists(aether_fs_t *fs, const char *vpath);
const char *aether_fs_resolve(aether_fs_t *fs, const char *vpath);

u32         aether_fs_root_count(const aether_fs_t *fs);
const char *aether_fs_root_at(const aether_fs_t *fs, u32 index);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_FS_H */
