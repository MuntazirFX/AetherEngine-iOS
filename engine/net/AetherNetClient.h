/* AetherNetClient.h — Client-side connection state machine.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_CLIENT_H
#define AETHER_NET_CLIENT_H

#include "AetherNet.h"
#include "AetherNetBuffer.h"
#include "AetherNetSnapshot.h"
#include "AetherNetScoreboard.h"
#include "AetherNetChat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_net_client aether_net_client_t;

/* Stats tracked per connection */
typedef struct aether_net_stats {
    u32   packets_sent;
    u32   packets_received;
    u32   bytes_sent;
    u32   bytes_received;
    u32   packets_lost;
    f32   ping_ms;
    f32   jitter_ms;
} aether_net_stats_t;

aether_net_client_t *aether_net_client_create(void);
void                 aether_net_client_destroy(aether_net_client_t *c);

/* Connect to server (hostname:port). Returns AETHER_OK if request sent. */
aether_result_t aether_net_client_connect(aether_net_client_t *c,
                                           const char *host, u16 port);

/* Disconnect with reason */
void aether_net_client_disconnect(aether_net_client_t *c);

/* Per-frame pump: sends pending, receives incoming, updates stats. */
void aether_net_client_tick(aether_net_client_t *c, f32 dt);

/* Send client input command */
aether_result_t aether_net_client_send_cmd(aether_net_client_t *c,
                                            aether_vec3_t move, f32 yaw, f32 pitch,
                                            u32 buttons);

/* Send chat */
aether_result_t aether_net_client_send_chat(aether_net_client_t *c, const char *text);

/* State */
aether_net_state_t aether_net_client_state(const aether_net_client_t *c);
u32                aether_net_client_player_id(const aether_net_client_t *c);
const aether_net_stats_t *aether_net_client_stats(const aether_net_client_t *c);
const aether_net_addr_t  *aether_net_client_server_addr(const aether_net_client_t *c);

void aether_net_client_dump(const aether_net_client_t *c);

/* Last ingested snapshot from SERVER_SNAPSHOT packets (UDP tick). */
const aether_net_snapshot_t *aether_net_client_last_snapshot(const aether_net_client_t *c);
u32 aether_net_client_snapshot_count(const aether_net_client_t *c);

/* Apply last snapshot → scoreboard/chat. Returns player_count or 0. */
u32 aether_net_client_apply_snapshot_hud(aether_net_client_t *c,
                                         aether_scoreboard_t *sb,
                                         aether_chat_log_t *chat,
                                         f32 now);

/* Host/smoke helper: inject a raw SERVER_SNAPSHOT packet as if received. */
aether_result_t aether_net_client_ingest_snapshot_packet(aether_net_client_t *c,
                                                         const u8 *data, u32 size);


#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_CLIENT_H */
