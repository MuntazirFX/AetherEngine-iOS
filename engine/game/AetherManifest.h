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

#define AETHER_MANIFEST_MAX_SIZE 4096
#define AETHER_MANIFEST_MAX_SLOTS 8

/* Parse a single manifest file into an owned slot (safe for multiple loads). */
aether_result_t aether_manifest_load(const char *filepath, aether_game_info_t *out_info);

/* Load known 5-game manifests from a directory. Returns count loaded, or -1. */
i32 aether_manifest_load_all(const char *dir_path);

/* Access previously loaded manifests (from load / load_all). */
u32 aether_manifest_count(void);
const aether_game_info_t *aether_manifest_at(u32 index);
void aether_manifest_clear(void);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MANIFEST_H */
