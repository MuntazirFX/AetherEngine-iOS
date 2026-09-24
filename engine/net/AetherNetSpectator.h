/* AetherNetSpectator.h — Spectator follow / cycle / eye-copy camera.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_NET_SPECTATOR_H
#define AETHER_NET_SPECTATOR_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_SPECTATOR_MAX_PLAYERS 32
#define AETHER_SPECTATOR_HUD_LEN     64

typedef enum aether_spectator_cam_mode {
    AETHER_SPEC_CAM_FOLLOW = 0, /* eye behind target */
    AETHER_SPEC_CAM_COPY_EYE    /* eye = target eye (+ optional height) */
} aether_spectator_cam_mode_t;

typedef struct aether_spectator_player {
    u32  player_id;
    char name[32];
    bool active;
} aether_spectator_player_t;

typedef struct aether_spectator {
    bool enabled;
    u32  target_player_id;
    f32  eye[3];
    f32  forward[3];
    f32  follow_distance; /* behind target (FOLLOW mode) */
    f32  follow_height;
    f32  smooth;          /* 0..1 lerp toward target */
    bool following;
    aether_spectator_cam_mode_t cam_mode;
    /* Cycle roster */
    aether_spectator_player_t roster[AETHER_SPECTATOR_MAX_PLAYERS];
    u32  roster_count;
    i32  roster_index;    /* -1 if none */
    /* HUD indicator */
    char hud_label[AETHER_SPECTATOR_HUD_LEN]; /* e.g. "SPEC: Alice [87]" */
    bool hud_visible;
    /* Target name/HP stub for SPEC HUD */
    char target_name[32];
    i32  target_hp;       /* -1 = unknown; 0..100 typical */
    bool target_hp_valid;
} aether_spectator_t;

void aether_spectator_init(aether_spectator_t *sp);
void aether_spectator_set_enabled(aether_spectator_t *sp, bool enabled);
void aether_spectator_follow(aether_spectator_t *sp, u32 player_id);
void aether_spectator_stop(aether_spectator_t *sp);

/* Update eye/forward to follow target_pos + target_fwd. Returns 1 if active. */
int  aether_spectator_tick(aether_spectator_t *sp, f32 dt,
                           const f32 target_pos[3], const f32 target_fwd[3]);

/* Copy current spectator eye/forward. */
void aether_spectator_get_eye(const aether_spectator_t *sp, f32 out[3]);
void aether_spectator_get_forward(const aether_spectator_t *sp, f32 out[3]);
bool aether_spectator_is_following(const aether_spectator_t *sp);

/* Camera mode: FOLLOW (behind) vs COPY_EYE (match target eye). */
void aether_spectator_set_cam_mode(aether_spectator_t *sp, aether_spectator_cam_mode_t mode);
aether_spectator_cam_mode_t aether_spectator_get_cam_mode(const aether_spectator_t *sp);

/* Roster for next/prev cycle. */
void aether_spectator_roster_clear(aether_spectator_t *sp);
int  aether_spectator_roster_add(aether_spectator_t *sp, u32 player_id, const char *name);
u32  aether_spectator_roster_count(const aether_spectator_t *sp);

/* Cycle to next/prev active player; starts follow. Returns new player_id or 0. */
u32  aether_spectator_cycle_next(aether_spectator_t *sp);
u32  aether_spectator_cycle_prev(aether_spectator_t *sp);
u32  aether_spectator_target_id(const aether_spectator_t *sp);

/* HUD indicator string ("SPEC: name" or empty). Returns bytes written. */
u32  aether_spectator_hud_indicator(const aether_spectator_t *sp, char *out, u32 cap);
bool aether_spectator_hud_visible(const aether_spectator_t *sp);
/* Refresh hud_label from current target/roster. */
void aether_spectator_refresh_hud(aether_spectator_t *sp);

/* Target name/HP stub: drives "SPEC: Name [HP]" HUD line. */
void aether_spectator_set_target_hp(aether_spectator_t *sp, i32 hp);
void aether_spectator_set_target_name(aether_spectator_t *sp, const char *name);
i32  aether_spectator_get_target_hp(const aether_spectator_t *sp);
/* Copy target name (from set or roster). Returns bytes. */
u32  aether_spectator_get_target_name(const aether_spectator_t *sp, char *out, u32 cap);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_NET_SPECTATOR_H */
