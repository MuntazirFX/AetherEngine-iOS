/* AetherModelFixture.h — Procedural clean-room MDL / SPR fixtures for CI.
 * No Half-Life IP; tiny synthetic files the host parser can load.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MODEL_FIXTURE_H
#define AETHER_MODEL_FIXTURE_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Write a minimal valid MDL v10 (IDST) with 1 bone, 1 bodypart, 1 studio
 * triangle mesh (3 verts) extractable for Metal draw. Returns bytes written. */
u32 aether_mdl_write_fixture(u8 *out, u32 cap);

/* Write fixture to filepath. Returns bytes written. */
u32 aether_mdl_write_fixture_file(const char *filepath);

/* Minimal GoldSrc-ish SPR (IDSP) v2 single-frame stub. Returns bytes written. */
u32 aether_sprite_write_fixture(u8 *out, u32 cap);
u32 aether_sprite_write_fixture_file(const char *filepath);

/* Load SPR fixture header fields (type/width/height/frames). Returns AETHER_OK. */
typedef struct aether_sprite_file_info {
    i32 version;
    i32 type;
    i32 width;
    i32 height;
    i32 numframes;
} aether_sprite_file_info_t;

aether_result_t aether_sprite_parse_header(const u8 *data, u32 size,
                                           aether_sprite_file_info_t *out);


/* Textured + 2-bone fixture (clean-room checker texture + dual-bone hierarchy).
 * Returns bytes written. Geometry extract still yields the triangle mesh. */
u32 aether_mdl_write_textured_fixture(u8 *out, u32 cap);
u32 aether_mdl_write_textured_fixture_file(const char *filepath);

/* Extract embedded texture RGBA8 stub from textured fixture (8x8 checker).
 * Returns bytes written (w*h*4) or 0. */
u32 aether_mdl_fixture_texture_rgba(u8 *out, u32 cap, u32 *out_w, u32 *out_h);

/* Sequence-skinned fixture: 2 bones + sequence frame count metadata (clean-room).
 * Geometry extract still yields the triangle; skinning uses aether_mdl_sequence_init_sway. */
u32 aether_mdl_write_seq_fixture(u8 *out, u32 cap);
u32 aether_mdl_write_seq_fixture_file(const char *filepath);
/* Returns embedded sequence frame count hint (0 if not seq fixture). */
u32 aether_mdl_fixture_seq_frame_count(const u8 *data, u32 size);

#ifdef __cplusplus
}
#endif

/* Studio fixture: real sequence + anim keyframe blocks (clean-room, not sway-only). */
u32 aether_mdl_write_studio_fixture(u8 *out, u32 cap);
u32 aether_mdl_write_studio_fixture_file(const char *filepath);

/* Hitbox stub embedded in studio fixture. */
typedef struct aether_mdl_hitbox {
    i32 bone;
    i32 group;
    f32 mins[3];
    f32 maxs[3];
} aether_mdl_hitbox_t;

#define AETHER_MDL_FIXTURE_MAX_HITBOXES 8

/* Parse hitboxes from studio/seq fixture bytes. Returns count. */
u32 aether_mdl_fixture_hitboxes(const u8 *data, u32 size,
                                aether_mdl_hitbox_t *out, u32 max_out);

/* Ray vs fixture hitboxes (world-space AABB, bone transform ignored for stub). */
bool aether_mdl_hitbox_trace(const aether_mdl_hitbox_t *boxes, u32 count,
                             const f32 origin[3], const f32 dir[3], f32 max_dist,
                             i32 *out_index, f32 *out_t, f32 out_point[3]);

/* Attachment points (muzzle etc.) embedded in studio fixture. */
typedef struct aether_mdl_attachment {
    i32 bone;
    f32 origin[3];
    f32 angles_deg[3];
    char name[32];
} aether_mdl_attachment_t;

#define AETHER_MDL_FIXTURE_MAX_ATTACHMENTS 8

u32 aether_mdl_fixture_attachments(const u8 *data, u32 size,
                                   aether_mdl_attachment_t *out, u32 max_out);

/* Resolve attachment: bone_mats is bone_count * 16 floats (column-major 4x4). */
bool aether_mdl_attachment_transform(const aether_mdl_attachment_t *att,
                                     const f32 *bone_mats, u32 bone_count,
                                     f32 out_pos[3], f32 out_forward[3]);

/* Studio event / sound cue stub keyed by frame. */
typedef struct aether_mdl_studio_event {
    f32 frame;
    i32 event;       /* 5004=sound cue stub, 5001=muzzle, etc. */
    char options[64];
} aether_mdl_studio_event_t;

#define AETHER_MDL_FIXTURE_MAX_EVENTS 8

u32 aether_mdl_fixture_events(const u8 *data, u32 size,
                              aether_mdl_studio_event_t *out, u32 max_out);

/* Events crossed when advancing from prev_frame → frame (half-open). Returns count fired. */
u32 aether_mdl_studio_events_fire(const aether_mdl_studio_event_t *evts, u32 count,
                                  f32 prev_frame, f32 frame,
                                  aether_mdl_studio_event_t *out_fired, u32 max_out);

/* Write studio fixture that also embeds RLE keys + attachments + events. */
u32 aether_mdl_write_studio_fixture_ex(u8 *out, u32 cap);

/* Find attachment by name (e.g. "muzzle"). Returns index or -1. */
i32 aether_mdl_attachment_find(const aether_mdl_attachment_t *atts, u32 count,
                               const char *name);



/* 3rd-person studio attachment matrix chain:
 * weapon muzzle (weapon bones) → player hand attach (player bones) → world. */
bool aether_mdl_attachment_chain_world(const aether_mdl_attachment_t *hand_att,
                                       const f32 *player_bone_mats, u32 player_bones,
                                       const aether_mdl_attachment_t *weapon_att,
                                       const f32 *weapon_bone_mats, u32 weapon_bones,
                                       const f32 player_origin[3],
                                       f32 out_pos[3], f32 out_forward[3]);

/* Fill identity 4x4 into out[16]. */
void aether_mdl_mat4_identity(f32 out[16]);

/* Multiply column-major 4x4: out = A * B. */
void aether_mdl_mat4_mul(const f32 A[16], const f32 B[16], f32 out[16]);

/* Translate matrix from origin. */
void aether_mdl_mat4_translate(const f32 origin[3], f32 out[16]);




/* ---------- Studio LOD / bodygroup select (clean-room) ---------- */
#define AETHER_MDL_LOD_MAGIC       ((i32)0xAE7E10D0)
#define AETHER_MDL_BODYGROUP_MAGIC ((i32)0xAE7E8006)
#define AETHER_MDL_MAX_LODS        4
#define AETHER_MDL_MAX_BODYPARTS   4
#define AETHER_MDL_MAX_SUBMODELS  4

typedef struct aether_mdl_lod_level {
    i32  level;          /* 0 = highest detail */
    u32  tri_count;      /* synthetic triangle budget for this LOD */
    f32  max_distance;   /* select this LOD when dist <= max_distance */
} aether_mdl_lod_level_t;

typedef struct aether_mdl_lod_table {
    u32 count;
    aether_mdl_lod_level_t levels[AETHER_MDL_MAX_LODS];
} aether_mdl_lod_table_t;

typedef struct aether_mdl_bodygroup_part {
    char name[32];
    u32  submodel_count;
    u32  selected;       /* 0 .. submodel_count-1 */
    u32  tri_per_sub[AETHER_MDL_MAX_SUBMODELS];
} aether_mdl_bodygroup_part_t;

typedef struct aether_mdl_bodygroup_state {
    u32 part_count;
    aether_mdl_bodygroup_part_t parts[AETHER_MDL_MAX_BODYPARTS];
    i32 active_lod;      /* currently selected LOD index */
} aether_mdl_bodygroup_state_t;

/* Write studio fixture + LOD table (3 levels) + 2 bodyparts × 2 submodels. */
u32 aether_mdl_write_lod_fixture(u8 *out, u32 cap);
u32 aether_mdl_write_lod_fixture_file(const char *filepath);

/* Parse LOD table from fixture bytes. Returns level count. */
u32 aether_mdl_fixture_lods(const u8 *data, u32 size,
                            aether_mdl_lod_table_t *out);

/* Select LOD index by camera distance (0 = highest). Returns -1 on fail. */
i32 aether_mdl_lod_select(const aether_mdl_lod_table_t *table, f32 distance);

/* Triangle budget for a LOD level (0 if OOB). */
u32 aether_mdl_lod_tri_count(const aether_mdl_lod_table_t *table, i32 lod);

/* Init bodygroup state from fixture (or defaults if no trailer). */
bool aether_mdl_bodygroup_init_from_fixture(aether_mdl_bodygroup_state_t *st,
                                            const u8 *data, u32 size);

/* Set / get / cycle submodel on a bodypart. */
bool aether_mdl_bodygroup_set(aether_mdl_bodygroup_state_t *st, u32 part, u32 sub);
u32  aether_mdl_bodygroup_get(const aether_mdl_bodygroup_state_t *st, u32 part);
u32  aether_mdl_bodygroup_cycle(aether_mdl_bodygroup_state_t *st, u32 part, int dir);

/* Total tris for current bodygroup selection at given LOD (min of both budgets). */
u32 aether_mdl_bodygroup_tri_total(const aether_mdl_bodygroup_state_t *st,
                                   const aether_mdl_lod_table_t *lods, i32 lod);

/* Apply LOD selection into bodygroup state.active_lod. */
i32 aether_mdl_bodygroup_apply_lod(aether_mdl_bodygroup_state_t *st,
                                   const aether_mdl_lod_table_t *lods, f32 distance);

/* ---------- Fuller LOD mesh extract (multi tri-budget → mesh by distance) ---------- */
#define AETHER_MDL_LOD_EXTRACT_MAX_TRIS  64
#define AETHER_MDL_LOD_EXTRACT_MAX_VERTS (AETHER_MDL_LOD_EXTRACT_MAX_TRIS * 3)

/* Fill out_pos[3*verts] + out_idx[3*tris] for a LOD's triangle budget.
 * Procedural clean-room fan mesh (not HL geometry). Returns tri count. */
u32 aether_mdl_lod_extract_mesh(const aether_mdl_lod_table_t *table, i32 lod,
                                f32 *out_pos, u32 max_verts,
                                u32 *out_idx, u32 max_idx,
                                u32 *out_vert_count, u32 *out_tri_count);

/* Select LOD by camera distance then extract that mesh. Returns lod index or -1. */
i32 aether_mdl_lod_extract_by_distance(const aether_mdl_lod_table_t *table, f32 distance,
                                       f32 *out_pos, u32 max_verts,
                                       u32 *out_idx, u32 max_idx,
                                       u32 *out_vert_count, u32 *out_tri_count);

/* ---------- Studio skin / texture group select stub ---------- */
#define AETHER_MDL_TEXGROUP_MAGIC      ((i32)0xAE7E5C10)
#define AETHER_MDL_MAX_TEXGROUPS 4
#define AETHER_MDL_MAX_TEXGROUP_SKINS       4

typedef struct aether_mdl_texgroup {
    char name[32];
    u32  texture_count;
    u32  selected; /* 0 .. texture_count-1 */
} aether_mdl_texgroup_t;

typedef struct aether_mdl_texgroup_state {
    u32 group_count;
    aether_mdl_texgroup_t groups[AETHER_MDL_MAX_TEXGROUPS];
    i32 active_group;
} aether_mdl_texgroup_state_t;

/* Write LOD fixture that also embeds skin/texture groups. */
u32 aether_mdl_write_skin_lod_fixture(u8 *out, u32 cap);
u32 aether_mdl_write_skin_lod_fixture_file(const char *filepath);

bool aether_mdl_texgroup_init_from_fixture(aether_mdl_texgroup_state_t *st,
                                       const u8 *data, u32 size);
bool aether_mdl_texgroup_set(aether_mdl_texgroup_state_t *st, u32 group, u32 tex);
u32  aether_mdl_texgroup_get(const aether_mdl_texgroup_state_t *st, u32 group);
u32  aether_mdl_texgroup_cycle(aether_mdl_texgroup_state_t *st, u32 group, int dir);
i32  aether_mdl_texgroup_select(aether_mdl_texgroup_state_t *st, u32 group);

#endif /* AETHER_MODEL_FIXTURE_H */
