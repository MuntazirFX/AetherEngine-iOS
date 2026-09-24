/* AetherBSPSynthetic.h — Minimal clean-room BSP v30 room (no game assets).
 * Used when real .bsp files are unavailable (copyright). Feeds mesh + entity paths.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_BSP_SYNTHETIC_H
#define AETHER_BSP_SYNTHETIC_H

#include "AetherBSP.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Build an in-memory GoldSrc-style BSP v30 "demo room":
 * axis-aligned box (-256..256, -256..256, 0..128) with floor/ceiling/walls,
 * a tiny node/leaf tree (split at X=0) + marksurfaces for VIS/leaf culling,
 * CLIPNODES hulls (point + standing/crouch) for floor/wall/ledge traces,
 * a 16u step ledge on +X for step-up tests,
 * ENTITIES lump containing worldspawn, info_player_start, and a few monsters.
 * No Half-Life map data — geometry and entity text are authored here. */
aether_bsp_t *aether_bsp_create_synthetic_room(void);

/* True when bsp->source starts with "synthetic:". */
bool aether_bsp_is_synthetic(const aether_bsp_t *bsp);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_BSP_SYNTHETIC_H */
