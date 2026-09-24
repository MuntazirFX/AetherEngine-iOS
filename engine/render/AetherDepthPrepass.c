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
