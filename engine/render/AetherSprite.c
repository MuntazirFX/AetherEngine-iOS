#include "AetherSprite.h"
#include <string.h>

aether_result_t aether_sprite_init(aether_sprite_t *s) {
    if (!s) return AETHER_ERR_INVALID_ARG;
    memset(s, 0, sizeof(*s));
    s->size[0] = s->size[1] = 16.0f;
    s->uv[2] = s->uv[3] = 1.0f;
    s->color[0] = s->color[1] = s->color[2] = s->color[3] = 1.0f;
    s->visible = true;
    return AETHER_OK;
}
void aether_sprite_set_uv(aether_sprite_t *s, f32 u0, f32 v0, f32 u1, f32 v1) {
    if (!s) return;
    s->uv[0] = u0; s->uv[1] = v0; s->uv[2] = u1; s->uv[3] = v1;
}
void aether_sprite_set_color(aether_sprite_t *s, const f32 c[4]) {
    if (s && c) memcpy(s->color, c, sizeof(s->color));
}
void aether_sprite_set_position(aether_sprite_t *s, f32 x, f32 y, f32 z) {
    if (!s) return;
    s->position[0] = x; s->position[1] = y; s->position[2] = z;
}
void aether_sprite_set_size(aether_sprite_t *s, f32 w, f32 h) {
    if (!s) return;
    s->size[0] = w; s->size[1] = h;
}

u32 aether_sprite_copy_quad(const aether_sprite_t *s,
                            const f32 *view_right, const f32 *view_up,
                            aether_sprite_quad_vertex_t *out, u32 max_out) {
    if (!s || !s->visible || !out || max_out < 6) return 0;
    f32 rx = 1.f, ry = 0.f, rz = 0.f;
    f32 ux = 0.f, uy = 0.f, uz = 1.f;
    if (view_right) { rx = view_right[0]; ry = view_right[1]; rz = view_right[2]; }
    if (view_up)    { ux = view_up[0];    uy = view_up[1];    uz = view_up[2]; }
    f32 hw = s->size[0] * 0.5f, hh = s->size[1] * 0.5f;
    f32 u0 = s->uv[0], v0 = s->uv[1], u1 = s->uv[2], v1 = s->uv[3];
    f32 corners[4][5]; /* xyz uv */
    f32 signs[4][2] = {{-1,-1},{1,-1},{1,1},{-1,1}};
    f32 uvs[4][2] = {{u0,v0},{u1,v0},{u1,v1},{u0,v1}};
    for (int i = 0; i < 4; ++i) {
        corners[i][0] = s->position[0] + rx * signs[i][0] * hw + ux * signs[i][1] * hh;
        corners[i][1] = s->position[1] + ry * signs[i][0] * hw + uy * signs[i][1] * hh;
        corners[i][2] = s->position[2] + rz * signs[i][0] * hw + uz * signs[i][1] * hh;
        corners[i][3] = uvs[i][0];
        corners[i][4] = uvs[i][1];
    }
    int idx[6] = {0,1,2, 0,2,3};
    for (int k = 0; k < 6; ++k) {
        int c = idx[k];
        out[k].x = corners[c][0]; out[k].y = corners[c][1]; out[k].z = corners[c][2];
        out[k].u = corners[c][3]; out[k].v = corners[c][4];
        out[k].r = s->color[0]; out[k].g = s->color[1];
        out[k].b = s->color[2]; out[k].a = s->color[3];
    }
    return 6;
}
