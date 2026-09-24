/* AetherFrustum.h — View-frustum planes + AABB cull for world leaves/faces.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_FRUSTUM_H
#define AETHER_FRUSTUM_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_frustum {
    /* 6 planes: left, right, bottom, top, near, far. xyz = normal, w = dist (ax+by+cz+d >= 0 inside). */
    f32 planes[6][4];
    bool valid;
} aether_frustum_t;

/* Extract frustum from column-major view*proj (clip space). */
void aether_frustum_from_view_proj(aether_frustum_t *f, const aether_mat4_t *view_proj);

/* True if AABB (mins/maxs) intersects or is inside the frustum. */
bool aether_frustum_aabb_visible(const aether_frustum_t *f,
                                 const f32 mins[3], const f32 maxs[3]);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_FRUSTUM_H */
