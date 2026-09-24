/* AetherLagComp.h — World rewind stub: history of player/monster AABBs + query.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_LAGCOMP_H
#define AETHER_LAGCOMP_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_LAGCOMP_MAX_ENTS   32
#define AETHER_LAGCOMP_MAX_FRAMES 16

typedef enum aether_lagcomp_kind {
    AETHER_LAGCOMP_PLAYER = 1,
    AETHER_LAGCOMP_MONSTER = 2,
    AETHER_LAGCOMP_OTHER = 3
} aether_lagcomp_kind_t;

typedef struct aether_lagcomp_aabb {
    i32 id;
    u8  kind;
    u8  pad[3];
    f32 mins[3];
    f32 maxs[3];
} aether_lagcomp_aabb_t;

typedef struct aether_lagcomp_frame {
    f32 time;
    u32 count;
    aether_lagcomp_aabb_t items[AETHER_LAGCOMP_MAX_ENTS];
} aether_lagcomp_frame_t;

typedef struct aether_lagcomp_history {
    aether_lagcomp_frame_t frames[AETHER_LAGCOMP_MAX_FRAMES];
    u32 head;  /* next write */
    u32 count;
} aether_lagcomp_history_t;

void aether_lagcomp_init(aether_lagcomp_history_t *h);
void aether_lagcomp_clear(aether_lagcomp_history_t *h);

/* Begin a new frame snapshot at `time` (seconds). */
void aether_lagcomp_begin_frame(aether_lagcomp_history_t *h, f32 time);

/* Add an AABB to the current (newest) frame. Returns false if full. */
bool aether_lagcomp_push_aabb(aether_lagcomp_history_t *h, i32 id, u8 kind,
                              const f32 mins[3], const f32 maxs[3]);

/* Find AABB for id nearest to `time` (rewound). Returns false if missing. */
bool aether_lagcomp_query(const aether_lagcomp_history_t *h, f32 time, i32 id,
                          aether_lagcomp_aabb_t *out);

/* Ray vs rewound AABB set. Picks nearest hit among ents at that time. */
bool aether_lagcomp_trace(const aether_lagcomp_history_t *h, f32 time,
                          const f32 origin[3], const f32 dir[3], f32 max_dist,
                          i32 *out_id, f32 *out_t, f32 out_point[3]);

u32 aether_lagcomp_frame_count(const aether_lagcomp_history_t *h);

#ifdef __cplusplus
}
#endif

/* Hit validation vs cmd history: rebuild look dir from rewound cmd, trace AABBs. */
struct aether_net_cmd_history;

typedef struct aether_lagcomp_hit {
    i32 id;
    f32 t;
    f32 point[3];
    f32 rewind_time;
    u32 cmd_seq;
    bool valid;
} aether_lagcomp_hit_t;

/* Build forward from yaw/pitch (degrees, Z-up GoldSrc-ish). */
void aether_lagcomp_look_dir(f32 yaw_deg, f32 pitch_deg, f32 out_dir[3]);

/* Validate attack hit: uses cmd at (now - lag_ms) for look + traces history at that time.
 * eye_origin is shooter eye in world. Returns true on hit. */
bool aether_lagcomp_validate_hit(const aether_lagcomp_history_t *h,
                                 const struct aether_net_cmd_history *cmds,
                                 f32 now, f32 lag_ms,
                                 const f32 eye_origin[3], f32 max_dist,
                                 aether_lagcomp_hit_t *out);

/* ---------- Bone-hitbox lag rewind (studio hitboxes at rewound bones) ---------- */
#define AETHER_LAGCOMP_MAX_BONES    8
#define AETHER_LAGCOMP_MAX_HITBOXES 8

typedef struct aether_lagcomp_hitbox {
    i32 bone;
    i32 group;
    f32 mins[3];
    f32 maxs[3];
} aether_lagcomp_hitbox_t;

typedef struct aether_lagcomp_studio {
    i32 id;
    u8  kind;
    u8  bone_count;
    u8  hitbox_count;
    u8  pad;
    f32 bone_mats[AETHER_LAGCOMP_MAX_BONES * 16]; /* column-major 4x4 */
    aether_lagcomp_hitbox_t boxes[AETHER_LAGCOMP_MAX_HITBOXES];
} aether_lagcomp_studio_t;

typedef struct aether_lagcomp_studio_frame {
    f32 time;
    u32 count;
    aether_lagcomp_studio_t items[AETHER_LAGCOMP_MAX_ENTS];
} aether_lagcomp_studio_frame_t;

typedef struct aether_lagcomp_studio_history {
    aether_lagcomp_studio_frame_t frames[AETHER_LAGCOMP_MAX_FRAMES];
    u32 head;
    u32 count;
} aether_lagcomp_studio_history_t;

void aether_lagcomp_studio_init(aether_lagcomp_studio_history_t *h);
void aether_lagcomp_studio_clear(aether_lagcomp_studio_history_t *h);
void aether_lagcomp_studio_begin_frame(aether_lagcomp_studio_history_t *h, f32 time);

/* Push entity studio pose (bone mats + hitboxes) into current frame. */
bool aether_lagcomp_studio_push(aether_lagcomp_studio_history_t *h, i32 id, u8 kind,
                                const f32 *bone_mats, u32 bone_count,
                                const aether_lagcomp_hitbox_t *boxes, u32 hitbox_count);

/* Query rewound studio pose for id. */
bool aether_lagcomp_studio_query(const aether_lagcomp_studio_history_t *h, f32 time, i32 id,
                                 aether_lagcomp_studio_t *out);

/* Transform local hitbox AABB by bone matrix → world mins/maxs. */
void aether_lagcomp_hitbox_to_world(const aether_lagcomp_hitbox_t *box,
                                    const f32 *bone_mat16,
                                    f32 out_mins[3], f32 out_maxs[3]);

/* Ray vs rewound studio hitboxes (bones at rewind time). */
bool aether_lagcomp_studio_trace(const aether_lagcomp_studio_history_t *h, f32 time,
                                 const f32 origin[3], const f32 dir[3], f32 max_dist,
                                 i32 *out_id, i32 *out_hitbox, f32 *out_t, f32 out_point[3]);

u32 aether_lagcomp_studio_frame_count(const aether_lagcomp_studio_history_t *h);

#endif /* AETHER_LAGCOMP_H */
