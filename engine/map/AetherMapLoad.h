/* AetherMapLoad.h — Load BSP from FS game dir, else synthetic demo room.
 * Clean-room; no copyrighted maps. AetherEngine-iOS.
 */
#ifndef AETHER_MAP_LOAD_H
#define AETHER_MAP_LOAD_H

#include "../core/AetherCore.h"
#include "../bsp/AetherBSP.h"
#include "../fs/AetherFS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aether_map_source {
    AETHER_MAP_SOURCE_NONE = 0,
    AETHER_MAP_SOURCE_FILE,
    AETHER_MAP_SOURCE_SYNTHETIC,
} aether_map_source_t;

typedef struct aether_map_load_result {
    aether_bsp_t       *bsp;
    aether_map_source_t source;
    char                path_tried[256];
    char                resolved[512];
} aether_map_load_result_t;

aether_result_t aether_map_load(aether_fs_t *fs, const char *vpath,
                                aether_map_load_result_t *out);
aether_result_t aether_map_load_named(aether_fs_t *fs, const char *map_name,
                                      aether_map_load_result_t *out);
u32 aether_map_write_minimal_fixture(const char *filepath);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MAP_LOAD_H */
