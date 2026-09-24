#include "AetherShadow.h"
#include <math.h>
#include <string.h>

aether_result_t aether_shadow_init(aether_shadow_t *s, u32 size) {
    if (!s || !size) return AETHER_ERR_INVALID_ARG;
    memset(s, 0, sizeof(*s));
    s->map_size = size;
    s->light_dir[0] = 0.3f; s->light_dir[1] = -1.0f; s->light_dir[2] = 0.4f;
    s->bias = 0.001f; s->strength = 0.7f; s->enabled = true;
    return AETHER_OK;
}
void aether_shadow_shutdown(aether_shadow_t *s) { if (s) memset(s, 0, sizeof(*s)); }
void aether_shadow_set_light(aether_shadow_t *s, const f32 d[3]) {
    if (s && d) memcpy(s->light_dir, d, sizeof(s->light_dir));
}
void aether_shadow_set_enabled(aether_shadow_t *s, bool e) { if (s) s->enabled = e; }

u32 aether_shadow_copy_blob(const aether_shadow_t *s,
                            f32 px, f32 py, f32 ground_z, f32 radius,
                            aether_blob_shadow_vertex_t *out, u32 max_out) {
    if (!s || !s->enabled || !out || max_out < 6 || radius <= 0.f) return 0;
    f32 r = radius;
    /* Slight light-dir skew so blob isn't perfectly centered (cheap soft-shadow cue). */
    f32 ox = -s->light_dir[0] * 4.f;
    f32 oy = -s->light_dir[1] * 4.f;
    f32 cx = px + ox, cy = py + oy, cz = ground_z + 0.5f;
    f32 corners[4][2] = {{-r,-r},{r,-r},{r,r},{-r,r}};
    f32 uvs[4][2] = {{0,0},{1,0},{1,1},{0,1}};
    int idx[6] = {0,1,2, 0,2,3};
    f32 a = s->strength;
    if (a < 0.f) a = 0.f;
    if (a > 1.f) a = 1.f;
    for (int k = 0; k < 6; ++k) {
        int c = idx[k];
        aether_blob_shadow_vertex_t *v = &out[k];
        v->x = cx + corners[c][0];
        v->y = cy + corners[c][1];
        v->z = cz;
        v->u = uvs[c][0]; v->v = uvs[c][1];
        v->alpha = a;
        v->pad = 0.f;
    }
    return 6;
}
