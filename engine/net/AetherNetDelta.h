/* AetherNetDelta.h — Delta snapshot encode/apply (not full state every tick).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_DELTA_H
#define AETHER_NET_DELTA_H

#include "AetherNetSnapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Wire message for delta snapshots (vs AETHER_MSG_SERVER_SNAPSHOT full). */
#ifndef AETHER_MSG_SERVER_DELTA
#define AETHER_MSG_SERVER_DELTA 0x17
#endif

/* Encode only players that differ from baseline (by player_id).
 * Always writes tick/time; chat if present. Returns bytes or 0. */
u32 aether_net_delta_encode(const aether_net_snapshot_t *baseline,
                            const aether_net_snapshot_t *current,
                            u8 *out, u32 cap);

/* Apply delta packet onto inout snapshot (starts from baseline copy). */
aether_result_t aether_net_delta_apply(const u8 *data, u32 size,
                                       aether_net_snapshot_t *inout);

/* Count how many player slots would be emitted as deltas. */
u32 aether_net_delta_changed_count(const aether_net_snapshot_t *baseline,
                                   const aether_net_snapshot_t *current);

#ifdef __cplusplus
}
#endif
#endif
