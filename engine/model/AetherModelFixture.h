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


/* ---------- Real multi-mesh LOD buckets (separate meshes, not just tri fans) ---------- */
#define AETHER_MDL_LOD_BUCKET_MAGIC    ((i32)0xAE7E10D1)
#define AETHER_MDL_LOD_BUCKET_MAX_VERTS 48
#define AETHER_MDL_LOD_BUCKET_MAX_IDX   96

typedef struct aether_mdl_lod_mesh_bucket {
    i32  lod;            /* which LOD level this bucket belongs to */
    u32  vert_count;
    u32  index_count;    /* always multiple of 3 */
    f32  positions[AETHER_MDL_LOD_BUCKET_MAX_VERTS * 3];
    u32  indices[AETHER_MDL_LOD_BUCKET_MAX_IDX];
} aether_mdl_lod_mesh_bucket_t;

typedef struct aether_mdl_lod_mesh_set {
    u32 count; /* one bucket per LOD (up to AETHER_MDL_MAX_LODS) */
    aether_mdl_lod_mesh_bucket_t buckets[AETHER_MDL_MAX_LODS];
} aether_mdl_lod_mesh_set_t;

/* Write LOD fixture that also embeds distinct mesh buckets per LOD. */
u32 aether_mdl_write_lod_mesh_fixture(u8 *out, u32 cap);
u32 aether_mdl_write_lod_mesh_fixture_file(const char *filepath);

/* Parse mesh buckets from fixture. Returns bucket count. */
u32 aether_mdl_fixture_lod_meshes(const u8 *data, u32 size,
                                  aether_mdl_lod_mesh_set_t *out);

/* Select bucket by camera distance (uses LOD table distances). Returns lod or -1. */
i32 aether_mdl_lod_mesh_select(const aether_mdl_lod_table_t *table,
                               const aether_mdl_lod_mesh_set_t *meshes,
                               f32 distance,
                               const aether_mdl_lod_mesh_bucket_t **out_bucket);

/* Copy selected bucket verts/indices into caller buffers. Returns tri count. */
u32 aether_mdl_lod_mesh_copy(const aether_mdl_lod_mesh_bucket_t *bucket,
                             f32 *out_pos, u32 max_verts,
                             u32 *out_idx, u32 max_idx,
                             u32 *out_vert_count, u32 *out_tri_count);

/* ---------- GPU studio LOD draw path (select LOD + issue draw) ---------- */
typedef struct aether_mdl_lod_gpu_draw {
    i32  lod;              /* selected LOD index */
    f32  distance;         /* camera distance used */
    u32  vert_count;
    u32  index_count;      /* multiple of 3 */
    u32  tri_count;
    u32  first_vertex;     /* GPU buffer base (0 for stub) */
    u32  first_index;
    bool issue;            /* true when draw should be submitted */
    bool cpu_select;       /* selected on CPU by distance */
} aether_mdl_lod_gpu_draw_t;

/* Select LOD mesh by distance and fill a GPU draw command (no GPU needed on host).
 * Returns lod index or -1. Sets out->issue when a valid bucket exists. */
i32 aether_mdl_lod_gpu_issue_draw(const aether_mdl_lod_table_t *table,
                                  const aether_mdl_lod_mesh_set_t *meshes,
                                  f32 distance,
                                  aether_mdl_lod_gpu_draw_t *out);

/* Same as issue_draw but also copies mesh verts/indices into caller buffers. */
i32 aether_mdl_lod_gpu_issue_draw_copy(const aether_mdl_lod_table_t *table,
                                       const aether_mdl_lod_mesh_set_t *meshes,
                                       f32 distance,
                                       aether_mdl_lod_gpu_draw_t *out,
                                       f32 *out_pos, u32 max_verts,
                                       u32 *out_idx, u32 max_idx);

/* ---------- GPU Hi-Z / LOD distance gate ---------- */
#define AETHER_MDL_HIZ_MAX_SAMPLES 64

typedef struct aether_mdl_hiz_sample {
    f32 depth;       /* stored Hi-Z depth (0..1, nearer = smaller) */
    f32 screen_x;    /* NDC-ish 0..1 */
    f32 screen_y;
} aether_mdl_hiz_sample_t;

typedef struct aether_mdl_hiz {
    aether_mdl_hiz_sample_t samples[AETHER_MDL_HIZ_MAX_SAMPLES];
    u32 count;
    f32 near_z;      /* camera near */
    f32 far_z;
    bool enabled;
} aether_mdl_hiz_t;

typedef struct aether_mdl_hiz_gate {
    bool occluded;       /* rejected by Hi-Z */
    bool distance_culled;/* beyond max draw distance */
    bool issue;          /* may draw */
    i32  lod;            /* selected (possibly bumped) LOD */
    f32  screen_pixels;  /* estimated projected size */
    f32  min_pixels;     /* gate threshold used */
    f32  distance;
} aether_mdl_hiz_gate_t;

void aether_mdl_hiz_init(aether_mdl_hiz_t *hiz);
void aether_mdl_hiz_clear(aether_mdl_hiz_t *hiz);
void aether_mdl_hiz_set_range(aether_mdl_hiz_t *hiz, f32 near_z, f32 far_z);
/* Push a depth sample (mip0 stub). Returns 1 if stored. */
int  aether_mdl_hiz_push(aether_mdl_hiz_t *hiz, f32 depth, f32 sx, f32 sy);
/* Sample nearest Hi-Z depth at (sx,sy). Returns depth or 1.f if empty. */
f32  aether_mdl_hiz_sample(const aether_mdl_hiz_t *hiz, f32 sx, f32 sy);

/* Distance + Hi-Z gate: may force higher LOD or cull.
 * aabb_radius = object radius in world units; fov_y_deg for screen size.
 * min_pixels: cull if projected size below this (default 4).
 * max_distance: hard cull beyond (0 = use table only).
 * Returns lod (-1 if culled). Fills *out gate. */
i32 aether_mdl_lod_hiz_gate(const aether_mdl_lod_table_t *table,
                            const aether_mdl_lod_mesh_set_t *meshes,
                            const aether_mdl_hiz_t *hiz,
                            f32 distance, f32 aabb_radius, f32 fov_y_deg,
                            f32 min_pixels, f32 max_distance,
                            f32 screen_x, f32 screen_y, f32 depth_ndc,
                            aether_mdl_hiz_gate_t *out);

/* Issue draw gated by Hi-Z / distance (wraps gpu_issue_draw). */
i32 aether_mdl_lod_gpu_issue_draw_hiz(const aether_mdl_lod_table_t *table,
                                      const aether_mdl_lod_mesh_set_t *meshes,
                                      const aether_mdl_hiz_t *hiz,
                                      f32 distance, f32 aabb_radius,
                                      aether_mdl_lod_gpu_draw_t *out,
                                      aether_mdl_hiz_gate_t *gate);

/* ---------- Real hierarchical Hi-Z mip pyramid + visibility queries ---------- */
#define AETHER_MDL_HIZ_MIP_LEVELS   6
#define AETHER_MDL_HIZ_MIP0_W      64
#define AETHER_MDL_HIZ_MIP0_H      64
#define AETHER_MDL_HIZ_PYRAMID_TEXELS \
    ((AETHER_MDL_HIZ_MIP0_W * AETHER_MDL_HIZ_MIP0_H) * 2) /* mip0 + smaller sum */

typedef struct aether_mdl_hiz_pyramid {
    f32  depth[AETHER_MDL_HIZ_PYRAMID_TEXELS]; /* hierarchical max-Z (farther = larger) */
    u32  level_offset[AETHER_MDL_HIZ_MIP_LEVELS];
    u32  level_w[AETHER_MDL_HIZ_MIP_LEVELS];
    u32  level_h[AETHER_MDL_HIZ_MIP_LEVELS];
    u32  levels;           /* built mip count */
    u32  mip0_w, mip0_h;
    bool built;
    bool gpu_hooks;        /* Metal encode path armed */
    u32  vis_queries;      /* count of visibility queries this frame */
    u32  vis_occluded;     /* how many returned occluded */
} aether_mdl_hiz_pyramid_t;

typedef struct aether_mdl_hiz_vis_query {
    f32  screen_x0, screen_y0, screen_x1, screen_y1; /* NDC 0..1 rect */
    f32  object_depth;     /* NDC depth of object (0 near .. 1 far) */
    f32  nearest_hiz;      /* sampled pyramid min-Z over rect */
    i32  mip_used;
    bool visible;
    bool occluded;
    bool valid;
} aether_mdl_hiz_vis_query_t;

void aether_mdl_hiz_pyramid_init(aether_mdl_hiz_pyramid_t *pyr);
void aether_mdl_hiz_pyramid_reset(aether_mdl_hiz_pyramid_t *pyr, u32 mip0_w, u32 mip0_h);
/* Write a mip0 depth texel (x,y in mip0 coords). Returns 1 if stored. */
int  aether_mdl_hiz_pyramid_write(aether_mdl_hiz_pyramid_t *pyr, u32 x, u32 y, f32 depth);
/* Fill mip0 from a full-screen depth stub (row-major, size mip0_w*mip0_h). */
u32  aether_mdl_hiz_pyramid_fill_mip0(aether_mdl_hiz_pyramid_t *pyr,
                                      const f32 *depth_mip0, u32 count);
/* Build hierarchical max-Z pyramid (conservative occlusion: farther Z covers). */
u32  aether_mdl_hiz_build_pyramid(aether_mdl_hiz_pyramid_t *pyr);
/* Sample max-Z at mip level covering rect; returns nearest (min) covering Z. */
f32  aether_mdl_hiz_pyramid_sample_rect(const aether_mdl_hiz_pyramid_t *pyr,
                                        f32 x0, f32 y0, f32 x1, f32 y1, i32 *out_mip);
/* Visibility query: object behind nearer Hi-Z → occluded. */
int  aether_mdl_hiz_vis_query(const aether_mdl_hiz_pyramid_t *pyr,
                              f32 x0, f32 y0, f32 x1, f32 y1, f32 object_depth,
                              aether_mdl_hiz_vis_query_t *out);
/* Arm Metal GPU hooks (downsample + query encode). Host sets flag; Metal reads. */
void aether_mdl_hiz_pyramid_set_gpu_hooks(aether_mdl_hiz_pyramid_t *pyr, bool armed);
bool aether_mdl_hiz_pyramid_gpu_hooks(const aether_mdl_hiz_pyramid_t *pyr);

/* Gate LOD using pyramid vis query (preferred over sample-buffer stub). */
i32 aether_mdl_lod_hiz_pyramid_gate(const aether_mdl_lod_table_t *table,
                                    const aether_mdl_lod_mesh_set_t *meshes,
                                    const aether_mdl_hiz_pyramid_t *pyr,
                                    f32 distance, f32 aabb_radius, f32 fov_y_deg,
                                    f32 min_pixels, f32 max_distance,
                                    f32 sx, f32 sy, f32 depth_ndc,
                                    aether_mdl_hiz_gate_t *out);


/* ---------- Depth-prepass → Hi-Z pyramid bind + mip-explicit vis query ---------- */
typedef struct aether_mdl_hiz_bind_result {
    bool filled;
    bool built;
    bool views_ready;
    u32  mip0_filled;
    u32  levels;
    u32  view_count;
} aether_mdl_hiz_bind_result_t;

/* Fill pyramid mip0 from depth-prepass stub samples (row-major depths, size count).
 * Then build pyramid. Returns levels built. */
u32  aether_mdl_hiz_bind_from_depth(aether_mdl_hiz_pyramid_t *pyr,
                                    const f32 *depth_samples, u32 count,
                                    u32 mip0_w, u32 mip0_h,
                                    aether_mdl_hiz_bind_result_t *out);

/* Copy pyramid level layout into caller arrays (for texture views). Returns levels. */
u32  aether_mdl_hiz_pyramid_texture_views(const aether_mdl_hiz_pyramid_t *pyr,
                                          u32 *out_w, u32 *out_h, u32 *out_off,
                                          u32 max_levels);

/* Visibility query forcing a specific mip (for multi-mip / pyramid-mip tests). */
int  aether_mdl_hiz_vis_query_at_mip(const aether_mdl_hiz_pyramid_t *pyr,
                                     f32 x0, f32 y0, f32 x1, f32 y1,
                                     f32 object_depth, i32 mip,
                                     aether_mdl_hiz_vis_query_t *out);

/* Conservative multi-mip query: occluded if ANY covering mip says occluded. */
int  aether_mdl_hiz_vis_query_multi_mip(const aether_mdl_hiz_pyramid_t *pyr,
                                        f32 x0, f32 y0, f32 x1, f32 y1,
                                        f32 object_depth,
                                        aether_mdl_hiz_vis_query_t *out);

/* ---------- Fixture MDL skin pages (clean-room RGBA pages, no HL assets) ---------- */
#define AETHER_MDL_SKIN_PAGE_W     16
#define AETHER_MDL_SKIN_PAGE_H     16
#define AETHER_MDL_SKIN_PAGE_MAX   4
#define AETHER_MDL_SKIN_PAGE_MAGIC ((i32)0xAE7E5C11)

typedef struct aether_mdl_skin_page {
    u8  rgba[AETHER_MDL_SKIN_PAGE_W * AETHER_MDL_SKIN_PAGE_H * 4];
    u32 width;
    u32 height;
    u8  group;
    u8  tex;
    bool valid;
} aether_mdl_skin_page_t;

typedef struct aether_mdl_skin_page_set {
    u32 count;
    aether_mdl_skin_page_t pages[AETHER_MDL_SKIN_PAGE_MAX];
} aether_mdl_skin_page_set_t;

/* Build procedural skin pages (checker / gradient per group·tex). */
void aether_mdl_skin_pages_init(aether_mdl_skin_page_set_t *set);
u32  aether_mdl_skin_pages_build_fixture(aether_mdl_skin_page_set_t *set, u32 page_count);
/* Sample bilinear-ish nearest texel → RGBA 0..1. Returns 1 if page valid. */
int  aether_mdl_skin_page_sample(const aether_mdl_skin_page_t *page,
                                 f32 u, f32 v, f32 out_rgba[4]);
int  aether_mdl_skin_pages_sample(const aether_mdl_skin_page_set_t *set,
                                  u8 group, u8 tex, f32 u, f32 v, f32 out_rgba[4]);
/* Find page index by group/tex; -1 if missing. */
i32  aether_mdl_skin_pages_find(const aether_mdl_skin_page_set_t *set, u8 group, u8 tex);


/* ---------- Device Metal Hi-Z as texture2d_array / mip-chain slices ---------- */
#define AETHER_MDL_HIZ_ARRAY_MAX_SLICES AETHER_MDL_HIZ_MIP_LEVELS

typedef struct aether_mdl_hiz_array_slice {
    u32 slice;          /* texture2d_array slice index (= mip) */
    u32 width;
    u32 height;
    u32 texel_offset;   /* into pyramid depth[] */
    bool valid;
} aether_mdl_hiz_array_slice_t;

typedef struct aether_mdl_hiz_array {
    u32 slice_count;
    u32 mip0_w, mip0_h;
    bool bound;             /* Metal encode marked array bound */
    bool gpu_array;         /* device texture2d_array path armed */
    aether_mdl_hiz_array_slice_t slices[AETHER_MDL_HIZ_ARRAY_MAX_SLICES];
} aether_mdl_hiz_array_t;

void aether_mdl_hiz_array_init(aether_mdl_hiz_array_t *arr);
/* Pack pyramid mips as texture2d_array slices (host descriptors for Metal). */
u32  aether_mdl_hiz_bind_texture2d_array(const aether_mdl_hiz_pyramid_t *pyr,
                                         aether_mdl_hiz_array_t *out);
void aether_mdl_hiz_array_mark_bound(aether_mdl_hiz_array_t *arr);
bool aether_mdl_hiz_array_was_bound(const aether_mdl_hiz_array_t *arr);
void aether_mdl_hiz_array_set_gpu(aether_mdl_hiz_array_t *arr, bool armed);
bool aether_mdl_hiz_array_gpu(const aether_mdl_hiz_array_t *arr);

/* Vis query using a specific array mip/slice on the Metal encode path. */
int  aether_mdl_hiz_vis_query_array_mip(const aether_mdl_hiz_pyramid_t *pyr,
                                        const aether_mdl_hiz_array_t *arr,
                                        f32 x0, f32 y0, f32 x1, f32 y1,
                                        f32 object_depth, i32 array_mip,
                                        aether_mdl_hiz_vis_query_t *out);

/* ---------- Packed MDL skin lumps (user asset when present; fixture fallback) ---------- */
#define AETHER_MDL_SKIN_LUMP_MAX       4
#define AETHER_MDL_SKIN_LUMP_MAX_W     64
#define AETHER_MDL_SKIN_LUMP_MAX_H     64
#define AETHER_MDL_SKIN_LUMP_MAGIC     ((i32)0xAE7E0001)
#define AETHER_MDL_SKIN_LUMP_MAX_RGBA  (AETHER_MDL_SKIN_LUMP_MAX_W * AETHER_MDL_SKIN_LUMP_MAX_H * 4)

typedef struct aether_mdl_skin_lump {
    char name[32];
    u32  width;
    u32  height;
    u32  rgba_bytes;
    u8   rgba[AETHER_MDL_SKIN_LUMP_MAX_RGBA];
    bool from_asset;   /* true when parsed from MDL bytes */
    bool valid;
} aether_mdl_skin_lump_t;

typedef struct aether_mdl_skin_lump_set {
    u32 count;
    bool used_fixture_fallback;
    aether_mdl_skin_lump_t lumps[AETHER_MDL_SKIN_LUMP_MAX];
} aether_mdl_skin_lump_set_t;

void aether_mdl_skin_lumps_init(aether_mdl_skin_lump_set_t *set);
/* Parse packed texture trailer / skin lumps from MDL bytes. Returns lump count. */
u32  aether_mdl_skin_lumps_load(aether_mdl_skin_lump_set_t *set,
                                const u8 *mdl_bytes, u32 size);
/* Load from user asset path when present; else 0. */
u32  aether_mdl_skin_lumps_load_file(aether_mdl_skin_lump_set_t *set, const char *path);
/* Prefer asset lumps; on miss/empty build fixture pages → lumps. */
u32  aether_mdl_skin_lumps_load_or_fixture(aether_mdl_skin_lump_set_t *set,
                                           const u8 *mdl_bytes, u32 size,
                                           u32 fixture_pages);
int  aether_mdl_skin_lump_sample(const aether_mdl_skin_lump_t *lump,
                                 f32 u, f32 v, f32 out_rgba[4]);
int  aether_mdl_skin_lumps_sample(const aether_mdl_skin_lump_set_t *set,
                                  u32 index, f32 u, f32 v, f32 out_rgba[4]);
/* Copy lump 0 into a skin_page for water-RT bind (downscale/clamp to page size). */
int  aether_mdl_skin_lump_to_page(const aether_mdl_skin_lump_t *lump,
                                  aether_mdl_skin_page_t *out_page);


/* ---------- GPU Hi-Z downsample chain into texture2d_array slices ---------- */
typedef struct aether_mdl_hiz_array_downsample {
    u32 slices_written;     /* how many array slices filled by downsample */
    u32 mip0_w, mip0_h;
    u32 compute_passes;     /* host-simulated Metal compute/fragment passes */
    bool from_mip0;         /* true when chain started from mip0 fill */
    bool gpu_chain;         /* Metal compute/fragment downsample armed */
    bool ready;             /* downsample complete; safe for vis query bind */
} aether_mdl_hiz_array_downsample_t;

void aether_mdl_hiz_array_downsample_init(aether_mdl_hiz_array_downsample_t *ds);
/* Host-side 2x2 min-depth downsample into pyramid + array slice descriptors.
 * Mirrors Metal aether_hiz_array_downsample compute/fragment chain. */
u32  aether_mdl_hiz_array_downsample_chain(aether_mdl_hiz_pyramid_t *pyr,
                                           aether_mdl_hiz_array_t *arr,
                                           aether_mdl_hiz_array_downsample_t *out);
void aether_mdl_hiz_array_downsample_set_gpu(aether_mdl_hiz_array_downsample_t *ds, bool armed);
bool aether_mdl_hiz_array_downsample_gpu(const aether_mdl_hiz_array_downsample_t *ds);
bool aether_mdl_hiz_array_downsample_ready(const aether_mdl_hiz_array_downsample_t *ds);

/* Vis query that requires downsample output bound into the array path. */
int  aether_mdl_hiz_vis_query_downsampled(const aether_mdl_hiz_pyramid_t *pyr,
                                          const aether_mdl_hiz_array_t *arr,
                                          const aether_mdl_hiz_array_downsample_t *ds,
                                          f32 x0, f32 y0, f32 x1, f32 y1,
                                          f32 object_depth, i32 preferred_mip,
                                          aether_mdl_hiz_vis_query_t *out);

/* ---------- MDL skinref / family select (studio skin families) ---------- */
#define AETHER_MDL_SKINREF_MAX_FAMILIES   4
#define AETHER_MDL_SKINREF_MAX_REFS       8
#define AETHER_MDL_SKINREF_MAGIC          ((i32)0xAE7E5C20)
#define AETHER_MDL_SKINREF_NAME_LEN       24

typedef struct aether_mdl_skinref_entry {
    u16 family;             /* family index */
    u16 skin_index;         /* index into skin pages / lumps */
    u8  group;              /* texture group */
    u8  tex;                /* texture within group */
    char name[AETHER_MDL_SKINREF_NAME_LEN];
    bool valid;
} aether_mdl_skinref_entry_t;

typedef struct aether_mdl_skinref_family {
    char name[AETHER_MDL_SKINREF_NAME_LEN];
    u16  family_id;
    u16  ref_count;
    u16  ref_first;         /* index into entries[] */
    bool valid;
} aether_mdl_skinref_family_t;

typedef struct aether_mdl_skinref_table {
    u32 family_count;
    u32 entry_count;
    u32 selected_family;
    u32 selected_ref;       /* within selected family (0..ref_count-1) */
    aether_mdl_skinref_family_t families[AETHER_MDL_SKINREF_MAX_FAMILIES];
    aether_mdl_skinref_entry_t  entries[AETHER_MDL_SKINREF_MAX_REFS];
    bool from_fixture;
} aether_mdl_skinref_table_t;

void aether_mdl_skinref_init(aether_mdl_skinref_table_t *t);
/* Build clean-room fixture: 2 families (default/camo), 2 refs each. */
u32  aether_mdl_skinref_build_fixture(aether_mdl_skinref_table_t *t);
/* Select family by id; returns 1 on success. */
int  aether_mdl_skinref_select_family(aether_mdl_skinref_table_t *t, u32 family_id);
/* Select family by name (case-sensitive short name). */
int  aether_mdl_skinref_select_family_name(aether_mdl_skinref_table_t *t, const char *name);
/* Select skinref within current family (0-based). */
int  aether_mdl_skinref_select_ref(aether_mdl_skinref_table_t *t, u32 ref_in_family);
/* Cycle family (+1/-1). Returns new family id or -1. */
i32  aether_mdl_skinref_cycle_family(aether_mdl_skinref_table_t *t, int dir);
/* Resolve current selection into group/tex/skin_index. */
int  aether_mdl_skinref_resolve(const aether_mdl_skinref_table_t *t,
                                u32 *out_family, u32 *out_ref,
                                u8 *out_group, u8 *out_tex, u16 *out_skin_index);
/* Sample active skinref via skin pages (builds pages if needed). */
int  aether_mdl_skinref_sample(const aether_mdl_skinref_table_t *t,
                               const aether_mdl_skin_page_set_t *pages,
                               f32 u, f32 v, f32 out_rgba[4]);
/* Write a tiny skinref trailer fixture into a buffer (for load tests). */
u32  aether_mdl_write_skinref_fixture(u8 *out, u32 cap);

#endif /* AETHER_MODEL_FIXTURE_H */
