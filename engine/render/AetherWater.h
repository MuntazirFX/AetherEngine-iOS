#ifndef AETHER_WATER_H
#define AETHER_WATER_H
#include "../core/AetherCore.h"
#include "../model/AetherModelFixture.h"
#include "../bsp/AetherBSPVis.h"

/* GPU/bridge-friendly vertex: pos.xyz + uv.xy + color.rgba (9 floats, 36 bytes). */
typedef struct aether_water_vertex {
    f32 x, y, z;
    f32 u, v;
    f32 r, g, b, a;
} aether_water_vertex_t;

typedef struct aether_water {
    f32 wave_time;
    f32 wave_speed;
    f32 wave_amp;
    f32 wave_freq;
    f32 opacity;
    f32 size;       /* half-extent of the plane on X/Y */
    f32 height;     /* Z of the undisturbed surface */
    f32 origin[2];  /* XY center of the plane */
    f32 color[4];   /* base tint RGBA */
    bool enabled;
} aether_water_t;

aether_result_t aether_water_init(aether_water_t *w);
void aether_water_shutdown(aether_water_t *w);
void aether_water_update(aether_water_t *w, f32 dt);
void aether_water_set_enabled(aether_water_t *w, bool enabled);
aether_result_t aether_water_set_color(aether_water_t *w, const f32 rgba[4]);
aether_result_t aether_water_set_size(aether_water_t *w, f32 size);
aether_result_t aether_water_set_height(aether_water_t *w, f32 height);
aether_result_t aether_water_set_origin(aether_water_t *w, f32 x, f32 y);
aether_result_t aether_water_set_wave(aether_water_t *w, f32 speed, f32 amp, f32 freq);

/* Copy a tessellated wavy plane into out[]. Returns vertex count written
 * (always a multiple of 3). max_out is vertex capacity. */
u32 aether_water_copy_render(const aether_water_t *w,
                             aether_water_vertex_t *out,
                             u32 max_out);
/* How many vertices the default water grid needs. */
u32 aether_water_render_vertex_count(void);


/* Planar reflection stub: mirror matrix + clip plane for Metal encode. */
typedef struct aether_water_reflect {
    f32 mirror[16];      /* column-major 4x4: reflect about water plane */
    f32 clip_plane[4];   /* ax + by + cz + d = 0 (world space) */
    f32 plane_origin[3];
    f32 plane_normal[3]; /* typically 0,0,1 (Z-up) */
    f32 eye_reflected[3];
    bool enabled;
} aether_water_reflect_t;

/* Uniforms Metal/host can upload (mirror 16 + clip 4 + flags). */
typedef struct aether_water_reflect_uniforms {
    f32 mirror[16];
    f32 clip_plane[4];
    f32 enabled;     /* 1 when reflection pass should run */
    f32 pad[3];
} aether_water_reflect_uniforms_t;

/* Build reflection about water height plane (normal +Z). eye = camera world pos. */
void aether_water_reflect_compute(const aether_water_t *w, const f32 eye[3],
                                  aether_water_reflect_t *out);

/* Fill GPU/host uniform block. */
void aether_water_reflect_fill_uniforms(const aether_water_reflect_t *r,
                                        aether_water_reflect_uniforms_t *out);

/* Reflect a point across the water plane (CPU helper / smoke). */
void aether_water_reflect_point(const aether_water_t *w, const f32 in[3], f32 out[3]);

/* True when reflection hooks are ready for Metal encode. */
bool aether_water_reflect_encode_needed(const aether_water_reflect_t *r);

/* ---------- Reflection render-target plan (allocates RT + samples it) ---------- */
typedef struct aether_water_reflect_rt {
    u32  width;
    u32  height;
    f32  scale;          /* relative to framebuffer; default 0.5 */
    bool allocated;      /* ensure() succeeded */
    bool sample_enabled; /* fragment samples reflection texture */
    bool enabled;
    u32  tex_stub_id;    /* host/Metal texture handle stub (non-zero when allocated) */
    /* clear / resolve / mip hooks (host + Metal) */
    bool cleared;
    bool resolved;
    u32  mip_levels;     /* 1 = no mips; >1 after gen_mips */
    f32  clear_rgba[4];
} aether_water_reflect_rt_t;

typedef struct aether_water_reflect_rt_plan {
    u32  pass_count;     /* 1 = render mirrored scene into RT */
    u32  width;
    u32  height;
    bool allocate;
    bool sample;
    bool needed;
} aether_water_reflect_rt_plan_t;

aether_result_t aether_water_reflect_rt_init(aether_water_reflect_rt_t *rt);
void aether_water_reflect_rt_shutdown(aether_water_reflect_rt_t *rt);
void aether_water_reflect_rt_set_enabled(aether_water_reflect_rt_t *rt, bool enabled);

/* Allocate (or resize) reflection RT at fb_w*scale × fb_h*scale. Marks allocated. */
aether_result_t aether_water_reflect_rt_ensure(aether_water_reflect_rt_t *rt,
                                               u32 fb_w, u32 fb_h, f32 scale);

/* Encode plan: allocate RT + sample it (not uniforms-only). */
void aether_water_reflect_rt_encode_plan(const aether_water_reflect_rt_t *rt,
                                         const aether_water_reflect_t *reflect,
                                         aether_water_reflect_rt_plan_t *out);

bool aether_water_reflect_rt_sample_needed(const aether_water_reflect_rt_t *rt);
bool aether_water_reflect_rt_encode_needed(const aether_water_reflect_rt_t *rt,
                                           const aether_water_reflect_t *reflect);

/* ---------- Mirrored-camera encode into reflection RT ---------- */
typedef struct aether_water_reflect_rt_draw {
    f32 mirror_mvp[16];   /* column-major: proj * view_mirrored */
    f32 mirror_view[16];
    f32 mirror_proj[16];
    f32 eye_reflected[3];
    f32 clip_plane[4];
    u32 width;
    u32 height;
    bool clear;           /* clear RT before draw */
    bool draw_world;      /* draw world mesh with mirror_mvp */
    bool draw_entities;   /* draw entities into RT (not BSP-only) */
    bool draw_monsters;   /* draw monsters into RT */
    u32  entity_count;    /* entities scheduled this pass */
    u32  monster_count;   /* monsters scheduled this pass */
    bool draw_studio_skins; /* studio/skinned materials (not debug boxes) */
    u32  studio_count;    /* ents with material studio/skinned */
    bool resolve;         /* resolve/mip after draw */
    bool needed;
} aether_water_reflect_rt_draw_t;

/* Build mirrored view from eye + reflection, then MVP = proj * view_m.
 * view/proj are column-major 4x4 (pass identity proj for smoke). */
void aether_water_reflect_rt_build_mirror_mvp(const aether_water_reflect_t *reflect,
                                              const f32 view[16], const f32 proj[16],
                                              f32 out_mvp[16], f32 out_view_m[16]);

/* Full clear → draw-world → resolve plan for Metal encode into RT. */
void aether_water_reflect_rt_draw_plan(const aether_water_reflect_rt_t *rt,
                                       const aether_water_reflect_t *reflect,
                                       const f32 view[16], const f32 proj[16],
                                       aether_water_reflect_rt_draw_t *out);

/* Clear / resolve / mip stubs (host smoke + Metal hooks). */
aether_result_t aether_water_reflect_rt_clear(aether_water_reflect_rt_t *rt,
                                              f32 r, f32 g, f32 b, f32 a);
aether_result_t aether_water_reflect_rt_resolve(aether_water_reflect_rt_t *rt);
aether_result_t aether_water_reflect_rt_gen_mips(aether_water_reflect_rt_t *rt);
bool aether_water_reflect_rt_was_cleared(const aether_water_reflect_rt_t *rt);
bool aether_water_reflect_rt_was_resolved(const aether_water_reflect_rt_t *rt);
u32  aether_water_reflect_rt_mip_levels(const aether_water_reflect_rt_t *rt);

/* ---------- Entities/monsters into water reflection RT (not BSP-only) ---------- */
#define AETHER_WATER_REFLECT_MAX_ENTS 32

typedef struct aether_water_reflect_ent {
    f32 origin[3];
    f32 half_extents[3]; /* AABB half-size for debug box draw */
    u32 ent_id;
    u8  kind;            /* 0=entity, 1=monster */
    u8  above_water;     /* 1 if origin.z > water height (reflectable) */
    /* Studio skin / attachment (less debug-box when material != DEBUG_BOX) */
    u8  material;        /* 0=debug box, 1=studio mesh, 2=skinned */
    u8  skin_group;
    u8  skin_tex;
    i8  attach_index;    /* -1 = none */
    f32 tint[4];         /* RGBA */
    f32 attach_origin[3];
    u8  has_attach;
    /* Fuller studio texture sample (atlas UV + procedural sample) */
    u8  tex_sample_mode;
    f32 tex_uv_scale[2];
    f32 tex_uv_offset[2];
    f32 tex_atlas[4]; /* u0,v0,u1,v1 */
    f32 tex_sample_rgba[4];
    u8  has_studio_tex;
} aether_water_reflect_ent_t;

typedef struct aether_water_reflect_ent_list {
    u32 count;
    aether_water_reflect_ent_t items[AETHER_WATER_REFLECT_MAX_ENTS];
    u32 entity_count;    /* kind==0 */
    u32 monster_count;   /* kind==1 */
    u32 drawn;           /* how many marked for draw this plan */
} aether_water_reflect_ent_list_t;

/* Extend draw plan flags (also mirrored in aether_water_reflect_rt_draw_t fields added below). */
void aether_water_reflect_ent_list_init(aether_water_reflect_ent_list_t *list);
void aether_water_reflect_ent_list_clear(aether_water_reflect_ent_list_t *list);

/* Push entity/monster; marks above_water if origin[2] > water_height. Returns 1 if stored. */
int  aether_water_reflect_ent_list_push(aether_water_reflect_ent_list_t *list,
                                        u32 ent_id, u8 kind,
                                        const f32 origin[3], const f32 half_ext[3],
                                        f32 water_height);

/* Filter list: set above_water flags; return count above water. */
u32  aether_water_reflect_ent_list_mark_above(aether_water_reflect_ent_list_t *list,
                                              f32 water_height);

/* Fill draw plan entity flags from list (draw_entities/monsters + counts). */
void aether_water_reflect_rt_draw_plan_ents(aether_water_reflect_rt_draw_t *plan,
                                            const aether_water_reflect_ent_list_t *list);

/* Convenience: draw_plan + attach entity list in one call. */
void aether_water_reflect_rt_draw_plan_full(const aether_water_reflect_rt_t *rt,
                                            const aether_water_reflect_t *reflect,
                                            const f32 view[16], const f32 proj[16],
                                            const aether_water_reflect_ent_list_t *ents,
                                            aether_water_reflect_rt_draw_t *out);


/* ---------- Studio skins/attachments into water reflection RT ---------- */
typedef enum aether_water_reflect_mat {
    AETHER_WATER_REFLECT_MAT_DEBUG_BOX = 0,
    AETHER_WATER_REFLECT_MAT_STUDIO    = 1,
    AETHER_WATER_REFLECT_MAT_SKINNED   = 2
} aether_water_reflect_mat_t;

typedef struct aether_water_reflect_studio {
    u8  material;
    u8  skin_group;
    u8  skin_tex;
    i8  attach_index;
    f32 tint[4];
    f32 attach_origin[3];
    bool has_attach;
} aether_water_reflect_studio_t;

/* Push with studio skin/attachment material (not debug-box when material>0). */
int  aether_water_reflect_ent_list_push_studio(aether_water_reflect_ent_list_t *list,
                                               u32 ent_id, u8 kind,
                                               const f32 origin[3], const f32 half_ext[3],
                                               f32 water_height,
                                               u8 material, u8 skin_group, u8 skin_tex,
                                               i8 attach_index, const f32 tint[4]);

u32  aether_water_reflect_ent_list_studio_count(const aether_water_reflect_ent_list_t *list);

void aether_water_reflect_rt_draw_plan_studio(aether_water_reflect_rt_draw_t *plan,
                                              const aether_water_reflect_ent_list_t *list);

int  aether_water_reflect_ent_get_studio(const aether_water_reflect_ent_list_t *list,
                                         u32 index, aether_water_reflect_studio_t *out);

/* Tint helper: skin_group/tex → soft color (clean-room procedural). */
void aether_water_reflect_skin_tint(u8 skin_group, u8 skin_tex, f32 out_rgba[4]);

/* ---------- Portal/teleport-aware water reflect camera ---------- */
typedef struct aether_water_reflect_portal {
    bool active;
    bool eye_crossed;      /* eye teleported through portal this frame */
    f32  in_origin[3];     /* portal entry center */
    f32  out_origin[3];    /* portal exit center */
    f32  out_delta[3];     /* out - in (teleport translation) */
    f32  eye_warped[3];    /* eye after portal warp (before mirror) */
} aether_water_reflect_portal_t;

void aether_water_reflect_portal_init(aether_water_reflect_portal_t *p);
void aether_water_reflect_portal_set(aether_water_reflect_portal_t *p,
                                     const f32 in_origin[3], const f32 out_origin[3],
                                     bool eye_crossed);
/* Warp eye by portal translation when crossed; then planar reflect. */
void aether_water_reflect_compute_portal(const aether_water_t *w, const f32 eye[3],
                                         const aether_water_reflect_portal_t *portal,
                                         aether_water_reflect_t *out);
/* Mirror MVP that applies portal warp to view before mirror. */
void aether_water_reflect_rt_build_mirror_mvp_portal(
    const aether_water_reflect_t *reflect,
    const aether_water_reflect_portal_t *portal,
    const f32 view[16], const f32 proj[16],
    f32 out_mvp[16], f32 out_view_m[16]);

/* ---------- Fuller studio texture sample for water RT ---------- */
typedef struct aether_water_reflect_studio_tex {
    f32 uv_scale[2];
    f32 uv_offset[2];
    f32 atlas_u0, atlas_v0, atlas_u1, atlas_v1;
    u8  sample_mode;   /* 0=tint, 1=procedural atlas, 2=sampled */
    f32 sample_rgba[4];
    bool valid;
} aether_water_reflect_studio_tex_t;

void aether_water_reflect_studio_tex_init(aether_water_reflect_studio_tex_t *tex,
                                          u8 skin_group, u8 skin_tex);
/* Procedural atlas UV + filtered sample (clean-room, no HL assets). */
void aether_water_reflect_studio_tex_sample(const aether_water_reflect_studio_tex_t *tex,
                                            f32 u, f32 v, f32 out_rgba[4]);
/* Bind studio tex onto ent slot; returns 1 if stored. */
int  aether_water_reflect_ent_set_studio_tex(aether_water_reflect_ent_list_t *list,
                                             u32 index, u8 skin_group, u8 skin_tex);
int  aether_water_reflect_ent_get_studio_tex(const aether_water_reflect_ent_list_t *list,
                                             u32 index,
                                             aether_water_reflect_studio_tex_t *out);


/* ---------- Portal winding clip + recursive reflect views ---------- */
#define AETHER_PORTAL_WINDING_MAX_VERTS 8
#define AETHER_PORTAL_REFLECT_MAX_DEPTH 3
#define AETHER_PORTAL_REFLECT_MAX_VIEWS 4

typedef struct aether_portal_winding {
    f32 verts[AETHER_PORTAL_WINDING_MAX_VERTS][3];
    u32 count;
    f32 plane[4];   /* ax+by+cz+d = 0 of portal surface */
    bool valid;
} aether_portal_winding_t;

typedef struct aether_portal_reflect_view {
    u32 depth;             /* recursion depth (0 = primary) */
    f32 clip_plane[4];
    f32 mirror[16];
    f32 eye[3];
    f32 eye_reflected[3];
    bool clipped;
    bool active;
    u32 winding_verts;     /* verts kept after clip */
} aether_portal_reflect_view_t;

typedef struct aether_portal_reflect_plan {
    u32 view_count;
    u32 max_depth;
    aether_portal_reflect_view_t views[AETHER_PORTAL_REFLECT_MAX_VIEWS];
    bool needed;
} aether_portal_reflect_plan_t;

void aether_portal_winding_init(aether_portal_winding_t *w);
/* Build a rectangular portal winding from center + right/up extents on a plane. */
int  aether_portal_winding_make_rect(aether_portal_winding_t *w,
                                     const f32 center[3], const f32 normal[3],
                                     f32 half_w, f32 half_h);
/* Clip winding against a plane (keep positive side). Returns remaining vert count. */
u32  aether_portal_winding_clip(const aether_portal_winding_t *in,
                                const f32 clip_plane[4],
                                aether_portal_winding_t *out);

void aether_portal_reflect_plan_init(aether_portal_reflect_plan_t *plan);
/* Build recursive reflect views through a portal winding (limited depth).
 * Each deeper view mirrors about water then re-clips through portal. */
u32  aether_water_reflect_recursive_plan(const aether_water_t *water,
                                         const f32 eye[3],
                                         const aether_portal_winding_t *portal,
                                         u32 max_depth,
                                         aether_portal_reflect_plan_t *out);

/* ---------- Bind fixture MDL skin page into water reflect RT ent ---------- */
int  aether_water_reflect_ent_bind_skin_page(aether_water_reflect_ent_list_t *list,
                                             u32 index,
                                             const aether_mdl_skin_page_t *page);
int  aether_water_reflect_ent_sample_skin_page(const aether_water_reflect_ent_list_t *list,
                                               u32 index, f32 u, f32 v, f32 out_rgba[4]);


/* ---------- Multi-portal leaf-graph flood → water reflect views ---------- */
typedef struct aether_water_reflect_portal_graph_plan {
    u32 view_count;
    u32 flooded_leaves;
    u32 max_depth;
    aether_portal_reflect_view_t views[AETHER_PORTAL_REFLECT_MAX_VIEWS];
    bool needed;
    bool from_graph;
} aether_water_reflect_portal_graph_plan_t;

void aether_water_reflect_portal_graph_plan_init(aether_water_reflect_portal_graph_plan_t *plan);
/* Flood portal graph from eye leaf; build reflect views for reached leaves. */
u32  aether_water_reflect_portal_graph_plan(const aether_water_t *water,
                                            const f32 eye[3],
                                            const aether_bsp_portal_graph_t *graph,
                                            u16 eye_leaf,
                                            u32 max_depth,
                                            aether_water_reflect_portal_graph_plan_t *out);


/* ---------- Portal winding from BSP marksurface/plane windings ---------- */
/* Fill aether_portal_winding from BSP portal winding verts/plane. */
int  aether_portal_winding_from_bsp(aether_portal_winding_t *out,
                                    const f32 verts[][3], u32 vert_count,
                                    const f32 plane[4]);
/* Build reflect plan using first BSP-sourced portal winding (fuller clip). */
u32  aether_water_reflect_portal_winding_plan(const aether_water_t *water,
                                              const f32 eye[3],
                                              const f32 verts[][3], u32 vert_count,
                                              const f32 plane[4],
                                              u32 max_depth,
                                              aether_portal_reflect_plan_t *out);

/* ---------- Portal winding clip against recursive reflect planes ---------- */
/* Clip winding against N planes in order (Sutherland–Hodgman cascade). */
u32  aether_portal_winding_clip_planes(const aether_portal_winding_t *in,
                                       const f32 planes[][4], u32 plane_count,
                                       aether_portal_winding_t *out);
/* Clip portal winding against every clip_plane in a recursive reflect plan. */
u32  aether_portal_winding_clip_reflect_planes(const aether_portal_winding_t *in,
                                               const aether_portal_reflect_plan_t *reflect,
                                               aether_portal_winding_t *out,
                                               u32 *out_planes_applied);
/* Build recursive plan then clip original portal against all view clip planes. */
u32  aether_water_reflect_portal_clip_plan(const aether_water_t *water,
                                           const f32 eye[3],
                                           const aether_portal_winding_t *portal,
                                           u32 max_depth,
                                           aether_portal_reflect_plan_t *out_plan,
                                           aether_portal_winding_t *out_clipped);

/* ---------- Fuller portal clip stack (multi-plane clip buffer) ---------- */
#define AETHER_PORTAL_CLIP_STACK_MAX 8

typedef struct aether_portal_clip_stack {
    f32 planes[AETHER_PORTAL_CLIP_STACK_MAX][4];
    u32 count;
    u32 push_count;   /* lifetime pushes */
    u32 pop_count;
    u32 clip_ops;     /* windings clipped against stack */
    bool valid;
} aether_portal_clip_stack_t;

void aether_portal_clip_stack_init(aether_portal_clip_stack_t *s);
int  aether_portal_clip_stack_push(aether_portal_clip_stack_t *s, const f32 plane[4]);
int  aether_portal_clip_stack_pop(aether_portal_clip_stack_t *s);
/* Push every active clip_plane from a recursive reflect plan (bottom→top). */
u32  aether_portal_clip_stack_push_reflect(aether_portal_clip_stack_t *s,
                                           const aether_portal_reflect_plan_t *reflect);
/* Clip winding against the entire stack (Sutherland–Hodgman cascade). */
u32  aether_portal_clip_stack_clip(const aether_portal_clip_stack_t *s,
                                   const aether_portal_winding_t *in,
                                   aether_portal_winding_t *out);
/* Build recursive reflect plan, fill clip stack, clip portal → out_clipped. */
u32  aether_water_reflect_portal_stack_plan(const aether_water_t *water,
                                            const f32 eye[3],
                                            const aether_portal_winding_t *portal,
                                            u32 max_depth,
                                            aether_portal_reflect_plan_t *out_plan,
                                            aether_portal_clip_stack_t *out_stack,
                                            aether_portal_winding_t *out_clipped);

#endif /* AETHER_WATER_H */
