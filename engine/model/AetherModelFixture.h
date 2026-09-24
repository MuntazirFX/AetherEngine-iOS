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

#endif
