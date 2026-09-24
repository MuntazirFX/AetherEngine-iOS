#include "AetherDepthPrepass.h"
#include <math.h>
#include <string.h>

aether_result_t aether_depth_prepass_init(aether_depth_prepass_t *d) {
    if (!d) return AETHER_ERR_INVALID_ARG;
    memset(d, 0, sizeof(*d));
    d->clear_depth = 1.f;
    d->write_depth = true;
    d->enabled = true;
    return AETHER_OK;
}

void aether_depth_prepass_shutdown(aether_depth_prepass_t *d) {
    if (d) memset(d, 0, sizeof(*d));
}

void aether_depth_prepass_set_enabled(aether_depth_prepass_t *d, bool enabled) {
    if (d) d->enabled = enabled;
}

aether_result_t aether_depth_prepass_ensure(aether_depth_prepass_t *d, u32 w, u32 h) {
    if (!d) return AETHER_ERR_INVALID_ARG;
    if (w == 0 || h == 0) return AETHER_ERR_INVALID_ARG;
    d->width = w;
    d->height = h;
    return AETHER_OK;
}

bool aether_depth_prepass_encode_needed(const aether_depth_prepass_t *d) {
    return d && d->enabled && d->width > 0 && d->height > 0 && d->write_depth;
}

void aether_depth_prepass_encode_plan(const aether_depth_prepass_t *d,
                                      aether_depth_prepass_plan_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->clear_depth = 1.f;
    out->write_depth = true;
    if (!d) return;
    out->needed = aether_depth_prepass_encode_needed(d);
    out->width = d->width;
    out->height = d->height;
    out->clear_depth = d->clear_depth > 0.f ? d->clear_depth : 1.f;
    out->write_depth = d->write_depth;
    out->pass_count = out->needed ? 1u : 0u;
}

f32 aether_depth_prepass_linearize(f32 depth01, f32 near_z, f32 far_z, bool reverse_z) {
    if (near_z <= 0.f) near_z = 0.1f;
    if (far_z <= near_z) far_z = near_z + 1.f;
    f32 z = depth01;
    if (reverse_z) z = 1.f - z;
    /* Standard perspective linearize */
    f32 ndc = z * 2.f - 1.f;
    f32 lin = (2.f * near_z * far_z) / (far_z + near_z - ndc * (far_z - near_z));
    return lin;
}

u32 aether_depth_prepass_record_stub(aether_depth_prepass_t *d,
                                     const f32 *positions_xyz, u32 count,
                                     f32 near_z, f32 far_z,
                                     f32 *out_depths, u32 max_out) {
    if (!d || !positions_xyz || !out_depths || count == 0 || max_out == 0) return 0;
    if (!d->enabled) return 0;
    u32 n = count < max_out ? count : max_out;
    for (u32 i = 0; i < n; ++i) {
        f32 z = positions_xyz[i * 3 + 2];
        /* Map world Z to 0..1 using near/far (stub camera looking -Z-ish uses |z|). */
        f32 dist = fabsf(z);
        f32 t = (dist - near_z) / (far_z - near_z);
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        out_depths[i] = aether_depth_prepass_linearize(t, near_z, far_z, false);
    }
    d->recorded = true;
    d->sample_count = n;
    return n;
}

void aether_depth_prepass_frame_init(aether_depth_prepass_frame_t *f) {
    if (!f) return;
    memset(f, 0, sizeof(*f));
    f->bind_before_main = true;
    f->main_pass_index = 1;
    f->prepass_index = 0;
}

int aether_depth_prepass_bind_before_main(const aether_depth_prepass_t *d,
                                          aether_depth_prepass_frame_t *f) {
    if (!f) return 0;
    aether_depth_prepass_frame_init(f);
    if (!aether_depth_prepass_encode_needed(d)) {
        f->bind_before_main = false;
        return 0;
    }
    f->bind_before_main = true;
    f->prepass_index = 0;
    f->main_pass_index = 1;
    return 1;
}

void aether_depth_prepass_mark_bound(aether_depth_prepass_frame_t *f) {
    if (f) f->bound = true;
}

bool aether_depth_prepass_was_bound_before_main(const aether_depth_prepass_frame_t *f) {
    return f && f->bind_before_main && f->bound && f->prepass_index < f->main_pass_index;
}

static void dp_mat4_mul(const f32 A[16], const f32 B[16], f32 out[16]) {
    f32 t[16];
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            t[c*4+r] = A[0*4+r]*B[c*4+0] + A[1*4+r]*B[c*4+1]
                     + A[2*4+r]*B[c*4+2] + A[3*4+r]*B[c*4+3];
    memcpy(out, t, sizeof t);
}
static void dp_mat4_id(f32 m[16]) {
    memset(m, 0, 16 * sizeof(f32));
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

void aether_depth_prepass_camera_init(aether_depth_prepass_camera_t *c) {
    if (!c) return;
    memset(c, 0, sizeof(*c));
    dp_mat4_id(c->view);
    dp_mat4_id(c->proj);
    dp_mat4_id(c->mvp);
}

void aether_depth_prepass_camera_set(aether_depth_prepass_camera_t *c,
                                     const f32 view[16], const f32 proj[16],
                                     const f32 eye[3]) {
    if (!c) return;
    aether_depth_prepass_camera_init(c);
    if (view) memcpy(c->view, view, sizeof c->view);
    if (proj) memcpy(c->proj, proj, sizeof c->proj);
    if (eye) { c->eye[0]=eye[0]; c->eye[1]=eye[1]; c->eye[2]=eye[2]; }
    dp_mat4_mul(c->proj, c->view, c->mvp);
    c->valid = true;
}

void aether_depth_prepass_camera_fill_mvp(const aether_depth_prepass_camera_t *c,
                                          f32 out_mvp[16]) {
    if (!out_mvp) return;
    if (c && c->valid) memcpy(out_mvp, c->mvp, 16 * sizeof(f32));
    else dp_mat4_id(out_mvp);
}

bool aether_depth_prepass_camera_valid(const aether_depth_prepass_camera_t *c) {
    return c && c->valid;
}

void aether_depth_prepass_encode_plan_ex(const aether_depth_prepass_t *d,
                                         const aether_depth_prepass_camera_t *cam,
                                         aether_depth_prepass_plan_ex_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    aether_depth_prepass_encode_plan(d, &out->base);
    aether_depth_prepass_camera_fill_mvp(cam, out->mvp);
    out->has_mvp = cam && cam->valid;
}


/* ---------- Depth → Hi-Z bind plan ---------- */
void aether_depth_hiz_bind_plan_init(aether_depth_hiz_bind_plan_t *plan) {
    if (!plan) return;
    memset(plan, 0, sizeof(*plan));
}

int aether_depth_hiz_bind_plan_encode(const aether_depth_prepass_t *d,
                                      u32 mip0_w, u32 mip0_h,
                                      aether_depth_hiz_bind_plan_t *out) {
    if (!out) return 0;
    aether_depth_hiz_bind_plan_init(out);
    if (mip0_w < 2) mip0_w = 2;
    if (mip0_h < 2) mip0_h = 2;
    out->mip0_w = mip0_w;
    out->mip0_h = mip0_h;
    if (!aether_depth_prepass_encode_needed(d)) {
        out->needed = false;
        return 0;
    }
    out->needed = true;
    out->depth_first = true;
    out->fill_mip0_from_depth = true;
    out->build_pyramid = true;
    out->texture_views = true;
    out->encode_steps = 4;
    return 1;
}

void aether_depth_hiz_bind_plan_mark_bound(aether_depth_hiz_bind_plan_t *plan) {
    if (plan) plan->bound = true;
}

bool aether_depth_hiz_bind_plan_was_bound(const aether_depth_hiz_bind_plan_t *plan) {
    return plan && plan->needed && plan->bound && plan->depth_first
        && plan->fill_mip0_from_depth && plan->build_pyramid;
}

u32 aether_depth_hiz_bind_plan_fill_views(aether_depth_hiz_bind_plan_t *plan,
                                          const u32 *level_w, const u32 *level_h,
                                          const u32 *level_off, u32 levels) {
    if (!plan || !level_w || !level_h || !level_off || levels == 0) return 0;
    u32 n = levels < AETHER_DEPTH_HIZ_MAX_MIP_VIEWS ? levels : AETHER_DEPTH_HIZ_MAX_MIP_VIEWS;
    for (u32 i = 0; i < n; ++i) {
        plan->views[i].mip = i;
        plan->views[i].width = level_w[i];
        plan->views[i].height = level_h[i];
        plan->views[i].texel_offset = level_off[i];
        plan->views[i].valid = (level_w[i] > 0 && level_h[i] > 0);
    }
    plan->mip_view_count = n;
    plan->texture_views = (n > 0);
    return n;
}


void aether_depth_hiz_array_bind_init(aether_depth_hiz_array_bind_t *b) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
}

int aether_depth_hiz_array_bind_encode(const aether_depth_hiz_bind_plan_t *plan,
                                       u32 slice_count,
                                       aether_depth_hiz_array_bind_t *out) {
    if (!out) return 0;
    aether_depth_hiz_array_bind_init(out);
    if (!plan || !plan->needed) return 0;
    out->array_texture = true;
    out->mip_chain = (plan->mip_view_count > 1) || (slice_count > 1);
    out->vis_query_array = true;
    out->slice_count = slice_count ? slice_count : plan->mip_view_count;
    if (out->slice_count == 0) out->slice_count = 1;
    out->mip0_w = plan->mip0_w;
    out->mip0_h = plan->mip0_h;
    return 1;
}

void aether_depth_hiz_array_bind_mark_bound(aether_depth_hiz_array_bind_t *b) {
    if (b) b->bound = (b->array_texture && b->slice_count > 0);
}

bool aether_depth_hiz_array_bind_was_bound(const aether_depth_hiz_array_bind_t *b) {
    return b && b->bound;
}


void aether_depth_hiz_downsample_bind_init(aether_depth_hiz_downsample_bind_t *b) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
}

int aether_depth_hiz_downsample_bind_encode(const aether_depth_hiz_array_bind_t *arr_bind,
                                            u32 slices, u32 compute_passes,
                                            aether_depth_hiz_downsample_bind_t *out) {
    if (!out) return 0;
    aether_depth_hiz_downsample_bind_init(out);
    if (!arr_bind || !arr_bind->array_texture) return 0;
    out->downsample_ready = (slices > 0);
    out->vis_query_bound = (slices > 0) && arr_bind->vis_query_array;
    out->gpu_chain = true;
    out->slices = slices ? slices : arr_bind->slice_count;
    out->compute_passes = compute_passes ? compute_passes : (out->slices > 1 ? out->slices - 1 : 1);
    out->mip0_w = arr_bind->mip0_w;
    out->mip0_h = arr_bind->mip0_h;
    return out->downsample_ready ? 1 : 0;
}

void aether_depth_hiz_downsample_bind_mark(aether_depth_hiz_downsample_bind_t *b) {
    if (!b) return;
    b->bound = b->downsample_ready && b->vis_query_bound && b->slices > 0;
}

bool aether_depth_hiz_downsample_bind_was_bound(const aether_depth_hiz_downsample_bind_t *b) {
    return b && b->bound;
}

bool aether_depth_hiz_downsample_vis_ready(const aether_depth_hiz_downsample_bind_t *b) {
    return b && b->bound && b->vis_query_bound;
}



/* ===== Live Hi-Z encode from depth prepass (batch18) ===== */

void aether_depth_hiz_live_encode_init(aether_depth_hiz_live_encode_t *e) {
    if (!e) return;
    memset(e, 0, sizeof(*e));
}

int aether_depth_hiz_live_encode_plan(const aether_depth_prepass_t *d,
                                      u32 mip0_w, u32 mip0_h, u32 slices,
                                      aether_depth_hiz_live_encode_t *out) {
    if (!out) return 0;
    aether_depth_hiz_live_encode_init(out);
    if (!d || !d->enabled) return 0;
    if (mip0_w == 0 || mip0_h == 0) return 0;
    out->depth_ready = (d->width > 0 && d->height > 0) || d->recorded;
    out->encode_from_depth = true;
    out->downsample_after = true;
    out->needed = true;
    out->mip0_w = mip0_w;
    out->mip0_h = mip0_h;
    out->slices = slices ? slices : 4;
    /* fill + (slices-1) downsample passes */
    out->encode_passes = 1u + (out->slices > 1 ? out->slices - 1 : 0);
    return 1;
}

void aether_depth_hiz_live_encode_mark(aether_depth_hiz_live_encode_t *e) {
    if (!e) return;
    e->encoded = e->needed && e->encode_from_depth && e->slices > 0;
}

bool aether_depth_hiz_live_encode_was_encoded(const aether_depth_hiz_live_encode_t *e) {
    return e && e->encoded;
}

bool aether_depth_hiz_live_encode_needed(const aether_depth_hiz_live_encode_t *e) {
    return e && e->needed;
}
