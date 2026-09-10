/* AetherManifest.h — Lightweight JSON manifest parser for game configs.
 * Clean-room. No external JSON library used.
 */
#ifndef AETHER_MANIFEST_H
#define AETHER_MANIFEST_H

#include "../core/AetherCore.h"
#include "AetherGameManager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum buffer size for reading manifest files */
#define AETHER_MANIFEST_MAX_SIZE 4096

/* Parse a single manifest file and fill the provided game info struct.
 * Returns AETHER_OK on success, AETHER_ERR_IO if file cannot be read.
 */
aether_result_t aether_manifest_load(const char *filepath, aether_game_info_t *out_info);

/* Find and load all manifests from a directory (e.g. "engine/game/manifests").
 * Returns number of manifests loaded, or -1 on error.
 */
i32 aether_manifest_load_all(const char *dir_path);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MANIFEST_H */
