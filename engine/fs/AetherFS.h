/* AetherFS.h — Virtual filesystem. Unifies loose files + PAK archives.
 * Search order: PAK archives first, then loose files on disk.
 */
#ifndef AETHER_FS_H
#define AETHER_FS_H

#include "../core/AetherCore.h"
#include "AetherPak.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_FS_MAX_PAKS    8
#define AETHER_FS_MAX_GAME    5

typedef struct aether_fs aether_fs_t;

aether_fs_t *aether_fs_create(const char *game_root);
void         aether_fs_destroy(aether_fs_t *fs);

/* Mount a PAK file into the FS. */
aether_result_t aether_fs_mount_pak(aether_fs_t *fs, const char *pak_path);

/* Unmount all PAKs (loose files remain accessible). */
void            aether_fs_unmount_all(aether_fs_t *fs);

/* Read a virtual file:
 *   - searches mounted PAKs first (in reverse-mount order, i.e. latest wins)
 *   - falls back to <game_root>/<vpath> on disk
 * If `out_buffer` is NULL, returns size only (query mode).
 * Returns bytes read or 0 on failure.
 */
u32             aether_fs_read_file(aether_fs_t *fs, const char *vpath,
                                    u8 *out_buffer, u32 out_capacity);

/* Returns true if vpath exists in any mounted source. */
bool            aether_fs_exists(aether_fs_t *fs, const char *vpath);

/* Returns the physical path (PAK name or disk path) as a static string, or NULL. */
const char     *aether_fs_resolve(const aether_fs_t *fs, const char *vpath);

const char     *aether_fs_game_root(const aether_fs_t *fs);

/* Auto-detect and mount pak0.pak, pak1.pak, ... from game_root. */
aether_result_t aether_fs_auto_mount_paks(aether_fs_t *fs);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_FS_H */
