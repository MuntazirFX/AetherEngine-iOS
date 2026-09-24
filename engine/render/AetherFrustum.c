/* AetherFrustum.c — Frustum extract + AABB test.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherFrustum.h"
#include <math.h>
#include <string.h>

static void normalize_plane(f32 p[4]) {
    f32 len = sqrtf(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
    if (len < 1e-8f) return;
    p[0] /= len; p[1] /= len; p[2] /= len; p[3] /= len;
}

void aether_frustum_from_view_proj(aether_frustum_t *f, const aether_mat4_t *vp) {
    if (!f) return;
    memset(f, 0, sizeof(*f));
    if (!vp) return;
    const f32 *m = vp->m;
    /* Column-major: row i col j = m[j*4+i] */
    /* Left:   row3 + row0 */
    f->planes[0][0] = m[3]  + m[0];
    f->planes[0][1] = m[7]  + m[4];
    f->planes[0][2] = m[11] + m[8];
    f->planes[0][3] = m[15] + m[12];
    /* Right:  row3 - row0 */
    f->planes[1][0] = m[3]  - m[0];
    f->planes[1][1] = m[7]  - m[4];
    f->planes[1][2] = m[11] - m[8];
    f->planes[1][3] = m[15] - m[12];
    /* Bottom: row3 + row1 */
    f->planes[2][0] = m[3]  + m[1];
    f->planes[2][1] = m[7]  + m[5];
    f->planes[2][2] = m[11] + m[9];
    f->planes[2][3] = m[15] + m[13];
    /* Top:    row3 - row1 */
    f->planes[3][0] = m[3]  - m[1];
    f->planes[3][1] = m[7]  - m[5];
    f->planes[3][2] = m[11] - m[9];
    f->planes[3][3] = m[15] - m[13];
    /* Near:   row3 + row2 */
    f->planes[4][0] = m[3]  + m[2];
    f->planes[4][1] = m[7]  + m[6];
    f->planes[4][2] = m[11] + m[10];
    f->planes[4][3] = m[15] + m[14];
    /* Far:    row3 - row2 */
    f->planes[5][0] = m[3]  - m[2];
    f->planes[5][1] = m[7]  - m[6];
    f->planes[5][2] = m[11] - m[10];
    f->planes[5][3] = m[15] - m[14];
    for (int i = 0; i < 6; ++i) normalize_plane(f->planes[i]);
    f->valid = true;
}

bool aether_frustum_aabb_visible(const aether_frustum_t *f,
                                 const f32 mins[3], const f32 maxs[3]) {
    if (!f || !f->valid || !mins || !maxs) return true; /* fail-open */
    for (int i = 0; i < 6; ++i) {
        const f32 *p = f->planes[i];
        /* Positive vertex along plane normal */
        f32 x = p[0] >= 0.f ? maxs[0] : mins[0];
        f32 y = p[1] >= 0.f ? maxs[1] : mins[1];
        f32 z = p[2] >= 0.f ? maxs[2] : mins[2];
        if (p[0]*x + p[1]*y + p[2]*z + p[3] < 0.f)
            return false;
    }
    return true;
}
