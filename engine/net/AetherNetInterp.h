/* AetherNetInterp.h — Client-side interpolation between snapshots.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_INTERP_H
#define AETHER_NET_INTERP_H

#include "AetherNetSnapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_net_interp {
    aether_net_snapshot_t prev;
    aether_net_snapshot_t curr;
    f32 fraction;   /* 0..1 between prev and curr */
    bool has_prev;
    bool has_curr;
} aether_net_interp_t;

void aether_net_interp_init(aether_net_interp_t *it);
void aether_net_interp_push(aether_net_interp_t *it, const aether_net_snapshot_t *snap);
void aether_net_interp_set_fraction(aether_net_interp_t *it, f32 frac);

/* Lerp player origin by id into out[3]. Returns 1 if found. */
int aether_net_interp_origin(const aether_net_interp_t *it, u32 player_id, f32 out[3]);

/* Build a fully interpolated snapshot (origins lerped; scores from curr). */
void aether_net_interp_sample(const aether_net_interp_t *it, aether_net_snapshot_t *out);

#ifdef __cplusplus
}
#endif
#endif
