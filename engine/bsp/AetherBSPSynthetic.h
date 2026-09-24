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

/* Synthetic +Y water pool (CONTENTS_WATER) — matches optional Metal water plane. */
#define AETHER_SYNTH_WATER_SURFACE_Z  48.0f
#define AETHER_SYNTH_WATER_ORIGIN_X    0.0f
#define AETHER_SYNTH_WATER_ORIGIN_Y  170.0f
#define AETHER_SYNTH_WATER_HALF_SIZE 100.0f /* covers physical pool xy */

/* Build an in-memory GoldSrc-style BSP v30 "demo room":
 * axis-aligned box (-256..256, -256..256, 0..128) with floor/ceiling/walls,
 * a tiny node/leaf tree (split at X=0) + marksurfaces + multi-leaf PVS stub,
 * LIGHTING samples + dual texinfo for lightmap UV unpack,
 * CLIPNODES hulls: point + standing (72u Z) + crouch (36u Z, distinct ceiling),
 * a 16u step ledge on +X, low-ceiling alcove on -X (z=48), +Y water pool
 * (CONTENTS_WATER, surface z=48) for swim/buoyancy tests,
 * ENTITIES lump containing worldspawn, info_player_start, and a few monsters.
 * No Half-Life map data — geometry and entity text are authored here. */
aether_bsp_t *aether_bsp_create_synthetic_room(void);

/* True when bsp->source starts with "synthetic:". */
bool aether_bsp_is_synthetic(const aether_bsp_t *bsp);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_BSP_SYNTHETIC_H */
