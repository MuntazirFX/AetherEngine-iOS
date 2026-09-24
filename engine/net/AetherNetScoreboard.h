/* AetherNetScoreboard.h — Client-side scoreboard state.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_SCOREBOARD_H
#define AETHER_NET_SCOREBOARD_H

#include "AetherNetProtocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_scoreboard_entry {
    u32  player_id;
    char name[AETHER_NET_MAX_NAME];
    i32  score;
    i32  deaths;
    i32  ping_ms;
    bool active;
} aether_scoreboard_entry_t;

typedef struct aether_scoreboard {
    aether_scoreboard_entry_t entries[AETHER_NET_MAX_PLAYERS];
    u32 count;
    bool visible;
} aether_scoreboard_t;

void aether_scoreboard_init(aether_scoreboard_t *sb);
void aether_scoreboard_clear(aether_scoreboard_t *sb);
void aether_scoreboard_set_visible(aether_scoreboard_t *sb, bool visible);
void aether_scoreboard_update_from_packet(aether_scoreboard_t *sb,
                                           const u8 *data, u32 size);
void aether_scoreboard_dump(const aether_scoreboard_t *sb);

#define AETHER_SCOREBOARD_MAX_EVENTS 32

typedef enum aether_scoreboard_event_kind {
    AETHER_SB_EVENT_JOIN  = 0,
    AETHER_SB_EVENT_LEAVE = 1,
    AETHER_SB_EVENT_KILL  = 2
} aether_scoreboard_event_kind_t;

typedef struct aether_scoreboard_event {
    u8   kind;       /* aether_scoreboard_event_kind_t */
    u32  player_id;  /* joiner/leaver, or killer for KILL */
    u32  victim_id;  /* kill feed victim (0 otherwise) */
    char name[AETHER_NET_MAX_NAME];       /* primary name (killer/joiner) */
    char victim_name[AETHER_NET_MAX_NAME]; /* kill feed victim name */
    f32  time;
} aether_scoreboard_event_t;

/* Ring of recent join/leave events (HUD ticker). */
typedef struct aether_scoreboard_events {
    aether_scoreboard_event_t items[AETHER_SCOREBOARD_MAX_EVENTS];
    u32 count;   /* total ever pushed (monotonic) */
    u32 head;    /* next write index */
    u32 live;    /* currently buffered (<= MAX) */
} aether_scoreboard_events_t;

void aether_scoreboard_events_init(aether_scoreboard_events_t *ev);
void aether_scoreboard_events_clear(aether_scoreboard_events_t *ev);
void aether_scoreboard_events_push(aether_scoreboard_events_t *ev,
                                   aether_scoreboard_event_kind_t kind,
                                   u32 player_id, const char *name, f32 time);
u32  aether_scoreboard_events_live(const aether_scoreboard_events_t *ev);
/* Index 0 = oldest live, live-1 = newest. Returns 1 on success. */
int  aether_scoreboard_events_get(const aether_scoreboard_events_t *ev, u32 index,
                                  aether_scoreboard_event_t *out);

/* Apply join/leave to scoreboard entries + optional event log. */
void aether_scoreboard_apply_join(aether_scoreboard_t *sb,
                                  aether_scoreboard_events_t *ev,
                                  u32 player_id, const char *name, f32 time);
void aether_scoreboard_apply_leave(aether_scoreboard_t *sb,
                                   aether_scoreboard_events_t *ev,
                                   u32 player_id, f32 time);

/* Encode JOIN / LEAVE packets (protocol header + payload). Returns bytes. */
u32 aether_scoreboard_encode_join(u8 *out, u32 cap,
                                  u32 player_id, const char *name);
u32 aether_scoreboard_encode_leave(u8 *out, u32 cap, u32 player_id);

/* Dispatch SCOREBOARD / PLAYER_JOIN / PLAYER_LEAVE packets. */
void aether_scoreboard_handle_packet(aether_scoreboard_t *sb,
                                     aether_scoreboard_events_t *ev,
                                     const u8 *data, u32 size, f32 time);

/* Kill feed stub: encode / apply / push HUD event. */
u32  aether_scoreboard_encode_kill(u8 *out, u32 cap,
                                   u32 killer_id, const char *killer_name,
                                   u32 victim_id, const char *victim_name);
void aether_scoreboard_apply_kill(aether_scoreboard_t *sb,
                                  aether_scoreboard_events_t *ev,
                                  u32 killer_id, const char *killer_name,
                                  u32 victim_id, const char *victim_name, f32 time);

/* Update frags/deaths on an entry (creates entry if missing). */
void aether_scoreboard_set_score(aether_scoreboard_t *sb, u32 player_id,
                                 const char *name, i32 score, i32 deaths);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_SCOREBOARD_H */
