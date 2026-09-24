#include "AetherWater.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Water plane tessellation: segs x segs quads. */
#define AETHER_WATER_SEGS 16

static void set4(f32 dst[4], f32 r, f32 g, f32 b, f32 a) {
    dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
}

aether_result_t aether_water_init(aether_water_t *w) {
    if (!w) return AETHER_ERR_INVALID_ARG;
    memset(w, 0, sizeof(*w));
    w->wave_speed = 1.0f;
    w->wave_amp = 6.0f;
    w->wave_freq = 0.035f;
    w->opacity = 0.72f;
    w->size = 512.0f;
    w->height = 0.0f;
    w->origin[0] = 0.0f;
    w->origin[1] = 0.0f;
    w->enabled = true;
    /* Soft teal — clean-room placeholder, no game assets. */
    set4(w->color, 0.15f, 0.45f, 0.65f, 0.72f);
    return AETHER_OK;
}

void aether_water_shutdown(aether_water_t *w) {
    if (w) memset(w, 0, sizeof(*w));
}

void aether_water_update(aether_water_t *w, f32 dt) {
    if (!w || !w->enabled || dt <= 0.0f) return;
    w->wave_time += dt * w->wave_speed;
}

void aether_water_set_enabled(aether_water_t *w, bool enabled) {
    if (w) w->enabled = enabled;
}

aether_result_t aether_water_set_color(aether_water_t *w, const f32 rgba[4]) {
    if (!w || !rgba) return AETHER_ERR_INVALID_ARG;
    set4(w->color, rgba[0], rgba[1], rgba[2], rgba[3]);
    w->opacity = rgba[3];
    return AETHER_OK;
}

aether_result_t aether_water_set_size(aether_water_t *w, f32 size) {
    if (!w || size <= 0.0f) return AETHER_ERR_INVALID_ARG;
    w->size = size;
    return AETHER_OK;
}

aether_result_t aether_water_set_height(aether_water_t *w, f32 height) {
    if (!w) return AETHER_ERR_INVALID_ARG;
    w->height = height;
    return AETHER_OK;
}

aether_result_t aether_water_set_origin(aether_water_t *w, f32 x, f32 y) {
    if (!w) return AETHER_ERR_INVALID_ARG;
    w->origin[0] = x;
    w->origin[1] = y;
    return AETHER_OK;
}

aether_result_t aether_water_set_wave(aether_water_t *w, f32 speed, f32 amp, f32 freq) {
    if (!w || speed < 0.0f || amp < 0.0f || freq < 0.0f)
        return AETHER_ERR_INVALID_ARG;
    w->wave_speed = speed;
    w->wave_amp = amp;
    w->wave_freq = freq;
    return AETHER_OK;
}

u32 aether_water_render_vertex_count(void) {
    /* Each seg×seg quad → 2 triangles → 6 verts. */
    return (u32)(AETHER_WATER_SEGS * AETHER_WATER_SEGS * 6);
}

static f32 sample_height(const aether_water_t *w, f32 x, f32 y) {
    f32 t = w->wave_time;
    f32 f = w->wave_freq;
    f32 a = w->wave_amp;
    /* Two phase-offset sines — cheap procedurals, clean-room. */
    f32 h = sinf((x + y) * f + t) * a;
    h += sinf((x - y) * f * 1.7f + t * 1.3f) * (a * 0.45f);
    return w->height + h;
}

static void emit_vert(aether_water_vertex_t *v,
                      f32 x, f32 y, f32 z,
                      f32 u, f32 vv,
                      const f32 rgba[4], f32 height_n) {
    /* Tint brighter on wave peaks. */
    f32 boost = 0.08f * height_n;
    v->x = x; v->y = y; v->z = z;
    v->u = u; v->v = vv;
    v->r = rgba[0] + boost;
    v->g = rgba[1] + boost * 0.6f;
    v->b = rgba[2];
    v->a = rgba[3];
    if (v->r > 1.0f) v->r = 1.0f;
    if (v->g > 1.0f) v->g = 1.0f;
}

u32 aether_water_copy_render(const aether_water_t *w,
                             aether_water_vertex_t *out,
                             u32 max_out) {
    if (!w || !out || !w->enabled || max_out == 0) return 0;
    const u32 need = aether_water_render_vertex_count();
    if (max_out < need) return 0;

    const f32 size = (w->size > 0.0f) ? w->size : 512.0f;
    const f32 ox = w->origin[0];
    const f32 oy = w->origin[1];
    f32 rgba[4] = { w->color[0], w->color[1], w->color[2], w->opacity };
    if (rgba[3] < 0.0f) rgba[3] = 0.0f;
    if (rgba[3] > 1.0f) rgba[3] = 1.0f;
    const f32 amp = (w->wave_amp > 1e-4f) ? w->wave_amp : 1.0f;

    u32 written = 0;
    for (int j = 0; j < AETHER_WATER_SEGS; ++j) {
        f32 v0 = (f32)j / (f32)AETHER_WATER_SEGS;
        f32 v1 = (f32)(j + 1) / (f32)AETHER_WATER_SEGS;
        f32 y0 = oy + (v0 * 2.0f - 1.0f) * size;
        f32 y1 = oy + (v1 * 2.0f - 1.0f) * size;
        for (int i = 0; i < AETHER_WATER_SEGS; ++i) {
            f32 u0 = (f32)i / (f32)AETHER_WATER_SEGS;
            f32 u1 = (f32)(i + 1) / (f32)AETHER_WATER_SEGS;
            f32 x0 = ox + (u0 * 2.0f - 1.0f) * size;
            f32 x1 = ox + (u1 * 2.0f - 1.0f) * size;

            f32 z00 = sample_height(w, x0, y0);
            f32 z10 = sample_height(w, x1, y0);
            f32 z01 = sample_height(w, x0, y1);
            f32 z11 = sample_height(w, x1, y1);

            f32 n00 = (z00 - w->height) / amp;
            f32 n10 = (z10 - w->height) / amp;
            f32 n01 = (z01 - w->height) / amp;
            f32 n11 = (z11 - w->height) / amp;

            /* Two triangles (CCW looking down +Z). */
            emit_vert(&out[written++], x0, y0, z00, u0, v0, rgba, n00);
            emit_vert(&out[written++], x1, y0, z10, u1, v0, rgba, n10);
            emit_vert(&out[written++], x0, y1, z01, u0, v1, rgba, n01);

            emit_vert(&out[written++], x1, y0, z10, u1, v0, rgba, n10);
            emit_vert(&out[written++], x1, y1, z11, u1, v1, rgba, n11);
            emit_vert(&out[written++], x0, y1, z01, u0, v1, rgba, n01);
        }
    }
    return written;
}

void aether_water_reflect_point(const aether_water_t *w, const f32 in[3], f32 out[3]) {
    if (!out) return;
    if (!in) { out[0]=out[1]=out[2]=0; return; }
    f32 h = w ? w->height : 0.f;
    /* Reflect across z = h (normal = +Z). */
    out[0] = in[0];
    out[1] = in[1];
    out[2] = 2.f * h - in[2];
}

void aether_water_reflect_compute(const aether_water_t *w, const f32 eye[3],
                                  aether_water_reflect_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->plane_normal[0] = 0.f;
    out->plane_normal[1] = 0.f;
    out->plane_normal[2] = 1.f;
    f32 h = w ? w->height : 0.f;
    out->plane_origin[0] = w ? w->origin[0] : 0.f;
    out->plane_origin[1] = w ? w->origin[1] : 0.f;
    out->plane_origin[2] = h;
    /* Clip plane: 0*x + 0*y + 1*z - h = 0 → keep z >= h for above-water view */
    out->clip_plane[0] = 0.f;
    out->clip_plane[1] = 0.f;
    out->clip_plane[2] = 1.f;
    out->clip_plane[3] = -h;
    /* Mirror matrix: I - 2 n n^T with translation so plane stays fixed.
     * For n=(0,0,1), M = diag(1,1,-1) and translation z' = 2h - z:
     *   [1 0 0 0]
     *   [0 1 0 0]
     *   [0 0 -1 2h]
     *   [0 0 0 1]  (column-major) */
    out->mirror[0] = 1.f; out->mirror[5] = 1.f;
    out->mirror[10] = -1.f; out->mirror[15] = 1.f;
    out->mirror[14] = 2.f * h; /* column 3, row 2 */
    if (eye) {
        aether_water_reflect_point(w, eye, out->eye_reflected);
    }
    out->enabled = w ? w->enabled : false;
}

void aether_water_reflect_fill_uniforms(const aether_water_reflect_t *r,
                                        aether_water_reflect_uniforms_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!r) return;
    memcpy(out->mirror, r->mirror, sizeof out->mirror);
    memcpy(out->clip_plane, r->clip_plane, sizeof out->clip_plane);
    out->enabled = (r->enabled && aether_water_reflect_encode_needed(r)) ? 1.f : 0.f;
}

bool aether_water_reflect_encode_needed(const aether_water_reflect_t *r) {
    if (!r || !r->enabled) return false;
    /* Mirror scale on Z should be -1. */
    return fabsf(r->mirror[10] + 1.f) < 1e-4f;
}

aether_result_t aether_water_reflect_rt_init(aether_water_reflect_rt_t *rt) {
    if (!rt) return AETHER_ERR_INVALID_ARG;
    memset(rt, 0, sizeof(*rt));
    rt->scale = 0.5f;
    rt->sample_enabled = true;
    rt->enabled = true;
    return AETHER_OK;
}

void aether_water_reflect_rt_shutdown(aether_water_reflect_rt_t *rt) {
    if (rt) memset(rt, 0, sizeof(*rt));
}

void aether_water_reflect_rt_set_enabled(aether_water_reflect_rt_t *rt, bool enabled) {
    if (rt) rt->enabled = enabled;
}

aether_result_t aether_water_reflect_rt_ensure(aether_water_reflect_rt_t *rt,
                                               u32 fb_w, u32 fb_h, f32 scale) {
    if (!rt) return AETHER_ERR_INVALID_ARG;
    if (fb_w == 0 || fb_h == 0) return AETHER_ERR_INVALID_ARG;
    if (scale <= 0.05f) scale = 0.5f;
    if (scale > 1.f) scale = 1.f;
    u32 w = (u32)((f32)fb_w * scale);
    u32 h = (u32)((f32)fb_h * scale);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    rt->width = w;
    rt->height = h;
    rt->scale = scale;
    rt->allocated = true;
    /* Non-zero stub id so Metal/host can tell RT is planned (texture made on GPU). */
    rt->tex_stub_id = 0xAE7E0001u ^ (w * 65537u + h);
    if (rt->tex_stub_id == 0) rt->tex_stub_id = 1;
    return AETHER_OK;
}

bool aether_water_reflect_rt_sample_needed(const aether_water_reflect_rt_t *rt) {
    return rt && rt->enabled && rt->allocated && rt->sample_enabled
        && rt->width > 0 && rt->height > 0 && rt->tex_stub_id != 0;
}

bool aether_water_reflect_rt_encode_needed(const aether_water_reflect_rt_t *rt,
                                           const aether_water_reflect_t *reflect) {
    if (!aether_water_reflect_rt_sample_needed(rt)) return false;
    return reflect ? aether_water_reflect_encode_needed(reflect) : true;
}

void aether_water_reflect_rt_encode_plan(const aether_water_reflect_rt_t *rt,
                                         const aether_water_reflect_t *reflect,
                                         aether_water_reflect_rt_plan_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!rt) return;
    out->needed = aether_water_reflect_rt_encode_needed(rt, reflect);
    out->width = rt->width;
    out->height = rt->height;
    out->allocate = rt->allocated && rt->tex_stub_id != 0;
    out->sample = aether_water_reflect_rt_sample_needed(rt);
    out->pass_count = out->needed ? 1u : 0u;
}

static void mat4_mul(const f32 A[16], const f32 B[16], f32 out[16]) {
    f32 t[16];
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            t[c * 4 + r] =
                A[0 * 4 + r] * B[c * 4 + 0] +
                A[1 * 4 + r] * B[c * 4 + 1] +
                A[2 * 4 + r] * B[c * 4 + 2] +
                A[3 * 4 + r] * B[c * 4 + 3];
        }
    }
    memcpy(out, t, sizeof t);
}

static void mat4_identity(f32 m[16]) {
    memset(m, 0, 16 * sizeof(f32));
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

void aether_water_reflect_rt_build_mirror_mvp(const aether_water_reflect_t *reflect,
                                              const f32 view[16], const f32 proj[16],
                                              f32 out_mvp[16], f32 out_view_m[16]) {
    f32 vm[16], p[16], v[16];
    mat4_identity(vm);
    mat4_identity(p);
    mat4_identity(v);
    if (view) memcpy(v, view, sizeof v);
    if (proj) memcpy(p, proj, sizeof p);
    /* view_mirrored = view * mirror  (world → mirrored → view) */
    if (reflect) {
        mat4_mul(v, reflect->mirror, vm);
    } else {
        memcpy(vm, v, sizeof vm);
    }
    if (out_view_m) memcpy(out_view_m, vm, sizeof vm);
    if (out_mvp) mat4_mul(p, vm, out_mvp);
}

void aether_water_reflect_rt_draw_plan(const aether_water_reflect_rt_t *rt,
                                       const aether_water_reflect_t *reflect,
                                       const f32 view[16], const f32 proj[16],
                                       aether_water_reflect_rt_draw_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!rt || !rt->enabled || !rt->allocated) return;
    out->needed = aether_water_reflect_rt_encode_needed(rt, reflect);
    out->width = rt->width;
    out->height = rt->height;
    out->clear = out->needed;
    out->draw_world = out->needed;
    out->resolve = out->needed;
    if (reflect) {
        memcpy(out->clip_plane, reflect->clip_plane, sizeof out->clip_plane);
        memcpy(out->eye_reflected, reflect->eye_reflected, sizeof out->eye_reflected);
    }
    aether_water_reflect_rt_build_mirror_mvp(reflect, view, proj,
                                             out->mirror_mvp, out->mirror_view);
    if (proj) memcpy(out->mirror_proj, proj, sizeof out->mirror_proj);
    else mat4_identity(out->mirror_proj);
}

aether_result_t aether_water_reflect_rt_clear(aether_water_reflect_rt_t *rt,
                                              f32 r, f32 g, f32 b, f32 a) {
    if (!rt || !rt->allocated) return AETHER_ERR_INVALID_ARG;
    rt->clear_rgba[0] = r; rt->clear_rgba[1] = g;
    rt->clear_rgba[2] = b; rt->clear_rgba[3] = a;
    rt->cleared = true;
    rt->resolved = false;
    return AETHER_OK;
}

aether_result_t aether_water_reflect_rt_resolve(aether_water_reflect_rt_t *rt) {
    if (!rt || !rt->allocated) return AETHER_ERR_INVALID_ARG;
    rt->resolved = true;
    if (rt->mip_levels < 1) rt->mip_levels = 1;
    return AETHER_OK;
}

aether_result_t aether_water_reflect_rt_gen_mips(aether_water_reflect_rt_t *rt) {
    if (!rt || !rt->allocated) return AETHER_ERR_INVALID_ARG;
    /* log2 stub levels from max dimension */
    u32 m = rt->width > rt->height ? rt->width : rt->height;
    u32 levels = 1;
    while (m > 1) { m >>= 1; levels++; }
    if (levels < 1) levels = 1;
    if (levels > 12) levels = 12;
    rt->mip_levels = levels;
    rt->resolved = true;
    return AETHER_OK;
}

bool aether_water_reflect_rt_was_cleared(const aether_water_reflect_rt_t *rt) {
    return rt && rt->cleared;
}
bool aether_water_reflect_rt_was_resolved(const aether_water_reflect_rt_t *rt) {
    return rt && rt->resolved;
}
u32 aether_water_reflect_rt_mip_levels(const aether_water_reflect_rt_t *rt) {
    return rt ? rt->mip_levels : 0;
}

void aether_water_reflect_ent_list_init(aether_water_reflect_ent_list_t *list) {
    aether_water_reflect_ent_list_clear(list);
}
void aether_water_reflect_ent_list_clear(aether_water_reflect_ent_list_t *list) {
    if (!list) return;
    memset(list, 0, sizeof(*list));
}

int aether_water_reflect_ent_list_push(aether_water_reflect_ent_list_t *list,
                                       u32 ent_id, u8 kind,
                                       const f32 origin[3], const f32 half_ext[3],
                                       f32 water_height) {
    if (!list || !origin || list->count >= AETHER_WATER_REFLECT_MAX_ENTS) return 0;
    aether_water_reflect_ent_t *e = &list->items[list->count++];
    e->ent_id = ent_id;
    e->kind = kind;
    e->origin[0] = origin[0]; e->origin[1] = origin[1]; e->origin[2] = origin[2];
    if (half_ext) {
        e->half_extents[0] = half_ext[0];
        e->half_extents[1] = half_ext[1];
        e->half_extents[2] = half_ext[2];
    } else {
        e->half_extents[0] = e->half_extents[1] = e->half_extents[2] = 8.f;
    }
    e->above_water = (origin[2] > water_height) ? 1 : 0;
    e->material = AETHER_WATER_REFLECT_MAT_DEBUG_BOX;
    e->skin_group = 0;
    e->skin_tex = 0;
    e->attach_index = -1;
    e->tint[0] = e->tint[1] = e->tint[2] = e->tint[3] = 1.f;
    e->attach_origin[0] = e->attach_origin[1] = e->attach_origin[2] = 0.f;
    e->has_attach = 0;
    if (kind == 1) list->monster_count++;
    else list->entity_count++;
    if (e->above_water) list->drawn++;
    return 1;
}

u32 aether_water_reflect_ent_list_mark_above(aether_water_reflect_ent_list_t *list,
                                             f32 water_height) {
    if (!list) return 0;
    u32 n = 0;
    list->drawn = 0;
    for (u32 i = 0; i < list->count; ++i) {
        list->items[i].above_water = (list->items[i].origin[2] > water_height) ? 1 : 0;
        if (list->items[i].above_water) { n++; list->drawn++; }
    }
    return n;
}

void aether_water_reflect_rt_draw_plan_ents(aether_water_reflect_rt_draw_t *plan,
                                            const aether_water_reflect_ent_list_t *list) {
    if (!plan) return;
    if (!list || !plan->needed) {
        plan->draw_entities = false;
        plan->draw_monsters = false;
        plan->entity_count = 0;
        plan->monster_count = 0;
        plan->draw_studio_skins = false;
        plan->studio_count = 0;
        return;
    }
    u32 ec = 0, mc = 0;
    for (u32 i = 0; i < list->count; ++i) {
        if (!list->items[i].above_water) continue;
        if (list->items[i].kind == 1) mc++;
        else ec++;
    }
    plan->entity_count = ec;
    plan->monster_count = mc;
    plan->draw_entities = ec > 0;
    plan->draw_monsters = mc > 0;
    aether_water_reflect_rt_draw_plan_studio(plan, list);
}

void aether_water_reflect_rt_draw_plan_full(const aether_water_reflect_rt_t *rt,
                                            const aether_water_reflect_t *reflect,
                                            const f32 view[16], const f32 proj[16],
                                            const aether_water_reflect_ent_list_t *ents,
                                            aether_water_reflect_rt_draw_t *out) {
    aether_water_reflect_rt_draw_plan(rt, reflect, view, proj, out);
    aether_water_reflect_rt_draw_plan_ents(out, ents);
}

void aether_water_reflect_skin_tint(u8 skin_group, u8 skin_tex, f32 out_rgba[4]) {
    if (!out_rgba) return;
    /* Procedural clean-room tint from group/tex indices (not HL skins). */
    f32 g = (f32)(skin_group % 8) / 7.f;
    f32 t = (f32)(skin_tex % 8) / 7.f;
    out_rgba[0] = 0.45f + 0.40f * g;
    out_rgba[1] = 0.55f + 0.30f * (1.f - t);
    out_rgba[2] = 0.50f + 0.35f * t;
    out_rgba[3] = 1.f;
}

int aether_water_reflect_ent_list_push_studio(aether_water_reflect_ent_list_t *list,
                                              u32 ent_id, u8 kind,
                                              const f32 origin[3], const f32 half_ext[3],
                                              f32 water_height,
                                              u8 material, u8 skin_group, u8 skin_tex,
                                              i8 attach_index, const f32 tint[4]) {
    if (!aether_water_reflect_ent_list_push(list, ent_id, kind, origin, half_ext, water_height))
        return 0;
    aether_water_reflect_ent_t *e = &list->items[list->count - 1];
    e->material = material;
    e->skin_group = skin_group;
    e->skin_tex = skin_tex;
    e->attach_index = attach_index;
    if (tint) {
        e->tint[0] = tint[0]; e->tint[1] = tint[1];
        e->tint[2] = tint[2]; e->tint[3] = tint[3];
    } else {
        aether_water_reflect_skin_tint(skin_group, skin_tex, e->tint);
    }
    if (attach_index >= 0) {
        e->has_attach = 1;
        /* Attachment origin = entity origin + small upward offset (fixture stub). */
        e->attach_origin[0] = origin[0];
        e->attach_origin[1] = origin[1];
        e->attach_origin[2] = origin[2] + 16.f + (f32)attach_index * 2.f;
    }
    return 1;
}

u32 aether_water_reflect_ent_list_studio_count(const aether_water_reflect_ent_list_t *list) {
    if (!list) return 0;
    u32 n = 0;
    for (u32 i = 0; i < list->count; ++i) {
        if (!list->items[i].above_water) continue;
        if (list->items[i].material == AETHER_WATER_REFLECT_MAT_STUDIO ||
            list->items[i].material == AETHER_WATER_REFLECT_MAT_SKINNED)
            n++;
    }
    return n;
}

void aether_water_reflect_rt_draw_plan_studio(aether_water_reflect_rt_draw_t *plan,
                                              const aether_water_reflect_ent_list_t *list) {
    if (!plan) return;
    u32 n = aether_water_reflect_ent_list_studio_count(list);
    plan->studio_count = n;
    plan->draw_studio_skins = n > 0;
}

int aether_water_reflect_ent_get_studio(const aether_water_reflect_ent_list_t *list,
                                        u32 index, aether_water_reflect_studio_t *out) {
    if (!list || !out || index >= list->count) return 0;
    const aether_water_reflect_ent_t *e = &list->items[index];
    out->material = e->material;
    out->skin_group = e->skin_group;
    out->skin_tex = e->skin_tex;
    out->attach_index = e->attach_index;
    out->tint[0] = e->tint[0]; out->tint[1] = e->tint[1];
    out->tint[2] = e->tint[2]; out->tint[3] = e->tint[3];
    out->attach_origin[0] = e->attach_origin[0];
    out->attach_origin[1] = e->attach_origin[1];
    out->attach_origin[2] = e->attach_origin[2];
    out->has_attach = e->has_attach != 0;
    return 1;
}

void aether_water_reflect_portal_init(aether_water_reflect_portal_t *p) {
    if (!p) return;
    memset(p, 0, sizeof(*p));
}

void aether_water_reflect_portal_set(aether_water_reflect_portal_t *p,
                                     const f32 in_origin[3], const f32 out_origin[3],
                                     bool eye_crossed) {
    if (!p) return;
    memset(p, 0, sizeof(*p));
    p->active = true;
    p->eye_crossed = eye_crossed;
    if (in_origin) {
        p->in_origin[0] = in_origin[0]; p->in_origin[1] = in_origin[1]; p->in_origin[2] = in_origin[2];
    }
    if (out_origin) {
        p->out_origin[0] = out_origin[0]; p->out_origin[1] = out_origin[1]; p->out_origin[2] = out_origin[2];
        p->out_delta[0] = out_origin[0] - (in_origin ? in_origin[0] : 0.f);
        p->out_delta[1] = out_origin[1] - (in_origin ? in_origin[1] : 0.f);
        p->out_delta[2] = out_origin[2] - (in_origin ? in_origin[2] : 0.f);
    }
}

void aether_water_reflect_compute_portal(const aether_water_t *w, const f32 eye[3],
                                         const aether_water_reflect_portal_t *portal,
                                         aether_water_reflect_t *out) {
    f32 eye_use[3] = {0, 0, 64};
    if (eye) { eye_use[0]=eye[0]; eye_use[1]=eye[1]; eye_use[2]=eye[2]; }
    if (portal && portal->active && portal->eye_crossed) {
        eye_use[0] += portal->out_delta[0];
        eye_use[1] += portal->out_delta[1];
        eye_use[2] += portal->out_delta[2];
        /* Stash warped eye on a mutable portal copy via out later — reflect uses warped. */
    }
    aether_water_reflect_compute(w, eye_use, out);
    if (portal && portal->active && out) {
        /* Record warped eye into reflect eye_reflected already from compute;
         * also overwrite plane if needed — compute already set eye_reflected. */
        (void)portal;
    }
}

void aether_water_reflect_rt_build_mirror_mvp_portal(
    const aether_water_reflect_t *reflect,
    const aether_water_reflect_portal_t *portal,
    const f32 view[16], const f32 proj[16],
    f32 out_mvp[16], f32 out_view_m[16]) {
    f32 view_adj[16];
    mat4_identity(view_adj);
    if (view) memcpy(view_adj, view, sizeof view_adj);
    /* When portal-crossed, translate view by -delta so mirrored camera tracks teleport. */
    if (portal && portal->active && portal->eye_crossed) {
        view_adj[12] -= portal->out_delta[0];
        view_adj[13] -= portal->out_delta[1];
        view_adj[14] -= portal->out_delta[2];
    }
    aether_water_reflect_rt_build_mirror_mvp(reflect, view_adj, proj, out_mvp, out_view_m);
}

void aether_water_reflect_studio_tex_init(aether_water_reflect_studio_tex_t *tex,
                                          u8 skin_group, u8 skin_tex) {
    if (!tex) return;
    memset(tex, 0, sizeof(*tex));
    /* 4x4 atlas cells in UV space from skin indices (clean-room procedural). */
    u8 cell = (u8)((skin_group * 3 + skin_tex) & 15);
    u8 cx = cell & 3, cy = (cell >> 2) & 3;
    tex->atlas_u0 = (f32)cx * 0.25f;
    tex->atlas_v0 = (f32)cy * 0.25f;
    tex->atlas_u1 = tex->atlas_u0 + 0.25f;
    tex->atlas_v1 = tex->atlas_v0 + 0.25f;
    tex->uv_scale[0] = 1.f; tex->uv_scale[1] = 1.f;
    tex->uv_offset[0] = 0.f; tex->uv_offset[1] = 0.f;
    tex->sample_mode = 1; /* procedural atlas */
    aether_water_reflect_skin_tint(skin_group, skin_tex, tex->sample_rgba);
    tex->valid = true;
}

void aether_water_reflect_studio_tex_sample(const aether_water_reflect_studio_tex_t *tex,
                                            f32 u, f32 v, f32 out_rgba[4]) {
    if (!out_rgba) return;
    if (!tex || !tex->valid) {
        out_rgba[0]=out_rgba[1]=out_rgba[2]=0.5f; out_rgba[3]=1.f;
        return;
    }
    f32 uu = u * tex->uv_scale[0] + tex->uv_offset[0];
    f32 vv = v * tex->uv_scale[1] + tex->uv_offset[1];
    /* Wrap into atlas cell and procedural filter. */
    uu = uu - floorf(uu); vv = vv - floorf(vv);
    f32 au = tex->atlas_u0 + uu * (tex->atlas_u1 - tex->atlas_u0);
    f32 av = tex->atlas_v0 + vv * (tex->atlas_v1 - tex->atlas_v0);
    f32 wave = 0.5f + 0.5f * sinf(au * 40.f + av * 28.f);
    f32 checker = (((int)(au * 16.f) + (int)(av * 16.f)) & 1) ? 1.f : 0.85f;
    out_rgba[0] = tex->sample_rgba[0] * (0.75f + 0.25f * wave) * checker;
    out_rgba[1] = tex->sample_rgba[1] * (0.80f + 0.20f * (1.f - wave));
    out_rgba[2] = tex->sample_rgba[2] * (0.70f + 0.30f * wave);
    out_rgba[3] = tex->sample_rgba[3];
}

int aether_water_reflect_ent_set_studio_tex(aether_water_reflect_ent_list_t *list,
                                            u32 index, u8 skin_group, u8 skin_tex) {
    if (!list || index >= list->count) return 0;
    aether_water_reflect_studio_tex_t tex;
    aether_water_reflect_studio_tex_init(&tex, skin_group, skin_tex);
    aether_water_reflect_ent_t *e = &list->items[index];
    e->skin_group = skin_group;
    e->skin_tex = skin_tex;
    e->tex_sample_mode = tex.sample_mode;
    e->tex_uv_scale[0] = tex.uv_scale[0]; e->tex_uv_scale[1] = tex.uv_scale[1];
    e->tex_uv_offset[0] = tex.uv_offset[0]; e->tex_uv_offset[1] = tex.uv_offset[1];
    e->tex_atlas[0] = tex.atlas_u0; e->tex_atlas[1] = tex.atlas_v0;
    e->tex_atlas[2] = tex.atlas_u1; e->tex_atlas[3] = tex.atlas_v1;
    e->tex_sample_rgba[0] = tex.sample_rgba[0]; e->tex_sample_rgba[1] = tex.sample_rgba[1];
    e->tex_sample_rgba[2] = tex.sample_rgba[2]; e->tex_sample_rgba[3] = tex.sample_rgba[3];
    e->has_studio_tex = 1;
    if (e->material == AETHER_WATER_REFLECT_MAT_DEBUG_BOX)
        e->material = AETHER_WATER_REFLECT_MAT_STUDIO;
    memcpy(e->tint, tex.sample_rgba, sizeof e->tint);
    return 1;
}

int aether_water_reflect_ent_get_studio_tex(const aether_water_reflect_ent_list_t *list,
                                            u32 index,
                                            aether_water_reflect_studio_tex_t *out) {
    if (!list || !out || index >= list->count) return 0;
    const aether_water_reflect_ent_t *e = &list->items[index];
    if (!e->has_studio_tex) return 0;
    memset(out, 0, sizeof(*out));
    out->uv_scale[0] = e->tex_uv_scale[0]; out->uv_scale[1] = e->tex_uv_scale[1];
    out->uv_offset[0] = e->tex_uv_offset[0]; out->uv_offset[1] = e->tex_uv_offset[1];
    out->atlas_u0 = e->tex_atlas[0]; out->atlas_v0 = e->tex_atlas[1];
    out->atlas_u1 = e->tex_atlas[2]; out->atlas_v1 = e->tex_atlas[3];
    out->sample_mode = e->tex_sample_mode;
    out->sample_rgba[0] = e->tex_sample_rgba[0]; out->sample_rgba[1] = e->tex_sample_rgba[1];
    out->sample_rgba[2] = e->tex_sample_rgba[2]; out->sample_rgba[3] = e->tex_sample_rgba[3];
    out->valid = true;
    return 1;
}


/* ---------- Portal winding + recursive reflect ---------- */
void aether_portal_winding_init(aether_portal_winding_t *w) {
    if (!w) return;
    memset(w, 0, sizeof(*w));
}

static void portal_cross(const f32 a[3], const f32 b[3], f32 o[3]) {
    o[0] = a[1]*b[2] - a[2]*b[1];
    o[1] = a[2]*b[0] - a[0]*b[2];
    o[2] = a[0]*b[1] - a[1]*b[0];
}
static f32 portal_dot(const f32 a[3], const f32 b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
static void portal_norm3(f32 v[3]) {
    f32 L = sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if (L < 1e-6f) { v[0]=0; v[1]=0; v[2]=1; return; }
    v[0]/=L; v[1]/=L; v[2]/=L;
}

int aether_portal_winding_make_rect(aether_portal_winding_t *w,
                                    const f32 center[3], const f32 normal[3],
                                    f32 half_w, f32 half_h) {
    if (!w || !center || !normal) return 0;
    aether_portal_winding_init(w);
    f32 n[3] = {normal[0], normal[1], normal[2]};
    portal_norm3(n);
    f32 up[3] = {0, 0, 1};
    if (fabsf(portal_dot(n, up)) > 0.9f) { up[0]=0; up[1]=1; up[2]=0; }
    f32 right[3]; portal_cross(up, n, right); portal_norm3(right);
    portal_cross(n, right, up); portal_norm3(up);
    if (half_w <= 0.f) half_w = 32.f;
    if (half_h <= 0.f) half_h = 48.f;
    f32 c0[3], c1[3], c2[3], c3[3];
    for (int i = 0; i < 3; ++i) {
        c0[i] = center[i] - right[i]*half_w - up[i]*half_h;
        c1[i] = center[i] + right[i]*half_w - up[i]*half_h;
        c2[i] = center[i] + right[i]*half_w + up[i]*half_h;
        c3[i] = center[i] - right[i]*half_w + up[i]*half_h;
    }
    memcpy(w->verts[0], c0, 3*sizeof(f32));
    memcpy(w->verts[1], c1, 3*sizeof(f32));
    memcpy(w->verts[2], c2, 3*sizeof(f32));
    memcpy(w->verts[3], c3, 3*sizeof(f32));
    w->count = 4;
    w->plane[0]=n[0]; w->plane[1]=n[1]; w->plane[2]=n[2];
    w->plane[3] = -(n[0]*center[0]+n[1]*center[1]+n[2]*center[2]);
    w->valid = true;
    return 1;
}

static f32 plane_dist(const f32 plane[4], const f32 p[3]) {
    return plane[0]*p[0] + plane[1]*p[1] + plane[2]*p[2] + plane[3];
}
static void lerp3(const f32 a[3], const f32 b[3], f32 t, f32 o[3]) {
    o[0]=a[0]+(b[0]-a[0])*t; o[1]=a[1]+(b[1]-a[1])*t; o[2]=a[2]+(b[2]-a[2])*t;
}

u32 aether_portal_winding_clip(const aether_portal_winding_t *in,
                               const f32 clip_plane[4],
                               aether_portal_winding_t *out) {
    if (!out) return 0;
    aether_portal_winding_init(out);
    if (!in || !in->valid || in->count < 3 || !clip_plane) return 0;
    f32 tmp[AETHER_PORTAL_WINDING_MAX_VERTS][3];
    u32 n = 0;
    for (u32 i = 0; i < in->count; ++i) {
        const f32 *a = in->verts[i];
        const f32 *b = in->verts[(i + 1) % in->count];
        f32 da = plane_dist(clip_plane, a);
        f32 db = plane_dist(clip_plane, b);
        int a_in = da >= -1e-4f;
        int b_in = db >= -1e-4f;
        if (a_in && b_in) {
            if (n < AETHER_PORTAL_WINDING_MAX_VERTS) memcpy(tmp[n++], b, 3*sizeof(f32));
        } else if (a_in && !b_in) {
            f32 t = da / (da - db + 1e-8f);
            f32 hit[3]; lerp3(a, b, t, hit);
            if (n < AETHER_PORTAL_WINDING_MAX_VERTS) memcpy(tmp[n++], hit, 3*sizeof(f32));
        } else if (!a_in && b_in) {
            f32 t = da / (da - db + 1e-8f);
            f32 hit[3]; lerp3(a, b, t, hit);
            if (n < AETHER_PORTAL_WINDING_MAX_VERTS) memcpy(tmp[n++], hit, 3*sizeof(f32));
            if (n < AETHER_PORTAL_WINDING_MAX_VERTS) memcpy(tmp[n++], b, 3*sizeof(f32));
        }
    }
    if (n < 3) return 0;
    out->count = n;
    for (u32 i = 0; i < n; ++i) memcpy(out->verts[i], tmp[i], 3*sizeof(f32));
    memcpy(out->plane, in->plane, sizeof out->plane);
    out->valid = true;
    return n;
}

void aether_portal_reflect_plan_init(aether_portal_reflect_plan_t *plan) {
    if (!plan) return;
    memset(plan, 0, sizeof(*plan));
}

u32 aether_water_reflect_recursive_plan(const aether_water_t *water,
                                        const f32 eye[3],
                                        const aether_portal_winding_t *portal,
                                        u32 max_depth,
                                        aether_portal_reflect_plan_t *out) {
    if (!out) return 0;
    aether_portal_reflect_plan_init(out);
    if (!water || !eye || !portal || !portal->valid) return 0;
    if (max_depth == 0) max_depth = 1;
    if (max_depth > AETHER_PORTAL_REFLECT_MAX_DEPTH)
        max_depth = AETHER_PORTAL_REFLECT_MAX_DEPTH;
    out->max_depth = max_depth;
    f32 cur_eye[3] = {eye[0], eye[1], eye[2]};
    aether_portal_winding_t cur_wind = *portal;
    for (u32 d = 0; d < max_depth && out->view_count < AETHER_PORTAL_REFLECT_MAX_VIEWS; ++d) {
        aether_water_reflect_t refl;
        aether_water_reflect_compute(water, cur_eye, &refl);
        if (!refl.enabled) break;
        /* Clip portal winding against water clip plane (keep above-water side). */
        aether_portal_winding_t clipped;
        u32 cv = aether_portal_winding_clip(&cur_wind, refl.clip_plane, &clipped);
        aether_portal_reflect_view_t *v = &out->views[out->view_count];
        memset(v, 0, sizeof(*v));
        v->depth = d;
        memcpy(v->clip_plane, refl.clip_plane, sizeof v->clip_plane);
        memcpy(v->mirror, refl.mirror, sizeof v->mirror);
        memcpy(v->eye, cur_eye, sizeof v->eye);
        memcpy(v->eye_reflected, refl.eye_reflected, sizeof v->eye_reflected);
        v->clipped = (cv >= 3);
        v->winding_verts = cv;
        v->active = true;
        out->view_count++;
        /* Recurse: next eye = reflected eye; portal stays (teleport stub via normal flip). */
        cur_eye[0] = refl.eye_reflected[0];
        cur_eye[1] = refl.eye_reflected[1];
        cur_eye[2] = refl.eye_reflected[2];
        if (cv >= 3) cur_wind = clipped;
        /* Flip portal plane for next bounce (recursive reflect). */
        cur_wind.plane[0] = -cur_wind.plane[0];
        cur_wind.plane[1] = -cur_wind.plane[1];
        cur_wind.plane[2] = -cur_wind.plane[2];
        cur_wind.plane[3] = -cur_wind.plane[3];
    }
    out->needed = (out->view_count > 0);
    return out->view_count;
}

int aether_water_reflect_ent_bind_skin_page(aether_water_reflect_ent_list_t *list,
                                            u32 index,
                                            const aether_mdl_skin_page_t *page) {
    if (!list || index >= list->count || !page || !page->valid) return 0;
    aether_water_reflect_ent_t *e = &list->items[index];
    f32 rgba[4];
    if (!aether_mdl_skin_page_sample(page, 0.5f, 0.5f, rgba)) return 0;
    e->material = AETHER_WATER_REFLECT_MAT_SKINNED;
    e->skin_group = page->group;
    e->skin_tex = page->tex;
    e->tex_sample_mode = 2; /* sampled from real page */
    e->tex_uv_scale[0] = 1.f; e->tex_uv_scale[1] = 1.f;
    e->tex_uv_offset[0] = 0.f; e->tex_uv_offset[1] = 0.f;
    e->tex_atlas[0] = 0.f; e->tex_atlas[1] = 0.f;
    e->tex_atlas[2] = 1.f; e->tex_atlas[3] = 1.f;
    e->tex_sample_rgba[0] = rgba[0];
    e->tex_sample_rgba[1] = rgba[1];
    e->tex_sample_rgba[2] = rgba[2];
    e->tex_sample_rgba[3] = rgba[3];
    e->tint[0] = rgba[0]; e->tint[1] = rgba[1];
    e->tint[2] = rgba[2]; e->tint[3] = rgba[3];
    e->has_studio_tex = 1;
    return 1;
}

int aether_water_reflect_ent_sample_skin_page(const aether_water_reflect_ent_list_t *list,
                                              u32 index, f32 u, f32 v, f32 out_rgba[4]) {
    if (!list || index >= list->count || !out_rgba) return 0;
    const aether_water_reflect_ent_t *e = &list->items[index];
    if (!e->has_studio_tex || e->tex_sample_mode != 2) return 0;
    /* Reconstruct sample from stored page tint + UV checker modulation (page bytes not kept on ent). */
    f32 base[4] = {e->tex_sample_rgba[0], e->tex_sample_rgba[1],
                   e->tex_sample_rgba[2], e->tex_sample_rgba[3]};
    f32 uu = u - floorf(u); if (uu < 0.f) uu += 1.f;
    f32 vv = v - floorf(v); if (vv < 0.f) vv += 1.f;
    int on = (((int)(uu * 16.f) + (int)(vv * 16.f)) & 1);
    f32 m = on ? 1.f : 0.85f;
    out_rgba[0] = base[0] * m;
    out_rgba[1] = base[1] * m;
    out_rgba[2] = base[2] * m;
    out_rgba[3] = base[3];
    return 1;
}


void aether_water_reflect_portal_graph_plan_init(aether_water_reflect_portal_graph_plan_t *plan) {
    if (!plan) return;
    memset(plan, 0, sizeof(*plan));
}

u32 aether_water_reflect_portal_graph_plan(const aether_water_t *water,
                                           const f32 eye[3],
                                           const aether_bsp_portal_graph_t *graph,
                                           u16 eye_leaf,
                                           u32 max_depth,
                                           aether_water_reflect_portal_graph_plan_t *out) {
    if (!out) return 0;
    aether_water_reflect_portal_graph_plan_init(out);
    if (!water || !graph || !eye) return 0;
    if (max_depth == 0) max_depth = 3;
    if (max_depth > AETHER_PORTAL_REFLECT_MAX_DEPTH)
        max_depth = AETHER_PORTAL_REFLECT_MAX_DEPTH;

    aether_bsp_portal_flood_t flood;
    u32 reached = aether_bsp_portal_graph_flood(graph, eye_leaf, max_depth, &flood);
    out->flooded_leaves = reached;
    out->max_depth = max_depth;
    out->from_graph = true;
    if (reached == 0) return 0;

    aether_water_reflect_t refl;
    aether_water_reflect_compute(water, eye, &refl);

    u32 views = 0;
    for (u32 i = 0; i < reached && views < AETHER_PORTAL_REFLECT_MAX_VIEWS; ++i) {
        aether_portal_reflect_view_t *v = &out->views[views];
        memset(v, 0, sizeof(*v));
        v->depth = flood.depth[i];
        v->active = true;
        v->eye[0]=eye[0]; v->eye[1]=eye[1]; v->eye[2]=eye[2];
        v->eye_reflected[0]=refl.eye_reflected[0];
        v->eye_reflected[1]=refl.eye_reflected[1];
        v->eye_reflected[2]=refl.eye_reflected[2];
        memcpy(v->mirror, refl.mirror, sizeof v->mirror);
        memcpy(v->clip_plane, refl.clip_plane, sizeof v->clip_plane);
        /* Offset clip slightly per flooded leaf for multi-portal distinction. */
        v->clip_plane[3] -= (f32)flood.reached[i] * 0.01f;
        v->clipped = (flood.depth[i] > 0);
        v->winding_verts = 4;
        ++views;
    }
    out->view_count = views;
    out->needed = (views > 0);
    return views;
}
