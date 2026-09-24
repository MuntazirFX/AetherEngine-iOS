/* AetherNetPredict.h — Client predicts local player; reconcile on snapshot.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_PREDICT_H
#define AETHER_NET_PREDICT_H

#include "AetherNetSnapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_net_predict_cmd {
    f32 forward;  /* -1..1 */
    f32 side;     /* -1..1 */
    f32 yaw_deg;
    f32 dt;
    u32 seq;
} aether_net_predict_cmd_t;

typedef struct aether_net_predict {
    u32 local_id;
    f32 origin[3];
    f32 velocity[3];
    f32 speed;          /* units/sec for predicted move */
    u32 last_ack_tick;
    u32 cmd_seq;
    f32 error[3];       /* last reconcile delta */
    bool active;
    struct aether_collision *collision; /* optional clipnodes for predict move */
    bool on_ground;
} aether_net_predict_t;

void aether_net_predict_init(aether_net_predict_t *pr, u32 local_id);
void aether_net_predict_set_speed(aether_net_predict_t *pr, f32 speed);

/* Apply one input cmd locally (no collision — stub). */
void aether_net_predict_apply_cmd(aether_net_predict_t *pr, const aether_net_predict_cmd_t *cmd);

/* Soft-correct toward authoritative snapshot origin for local_id. */
void aether_net_predict_reconcile(aether_net_predict_t *pr,
                                  const aether_net_snapshot_t *snap,
                                  f32 blend /* 0=keep predict, 1=snap */);

void aether_net_predict_get_origin(const aether_net_predict_t *pr, f32 out[3]);

/* Optional clipnode collision for prediction (borrowed; may be NULL). */
struct aether_collision;
void aether_net_predict_set_collision(aether_net_predict_t *pr, struct aether_collision *col);
struct aether_collision *aether_net_predict_get_collision(const aether_net_predict_t *pr);

/* Same as apply_cmd but slides through clipnodes when collision is set. */
void aether_net_predict_apply_cmd_clipped(aether_net_predict_t *pr,
                                          const aether_net_predict_cmd_t *cmd,
                                          i32 hull_index);

#ifdef __cplusplus
}
#endif
#endif
