#include "AetherMath.h"
#include <string.h>

/* ---------- Vec2 ---------- */
aether_vec2_t aether_vec2_add(aether_vec2_t a, aether_vec2_t b) { return (aether_vec2_t){a.x+b.x, a.y+b.y}; }
aether_vec2_t aether_vec2_sub(aether_vec2_t a, aether_vec2_t b) { return (aether_vec2_t){a.x-b.x, a.y-b.y}; }
aether_vec2_t aether_vec2_scale(aether_vec2_t v, f32 s)         { return (aether_vec2_t){v.x*s, v.y*s}; }
f32           aether_vec2_dot(aether_vec2_t a, aether_vec2_t b) { return a.x*b.x + a.y*b.y; }
f32           aether_vec2_len(aether_vec2_t v)                  { return sqrtf(aether_vec2_dot(v, v)); }
aether_vec2_t aether_vec2_normalize(aether_vec2_t v) {
    f32 l = aether_vec2_len(v);
    if (l < 1e-6f) return (aether_vec2_t){0,0};
    return aether_vec2_scale(v, 1.0f/l);
}

/* ---------- Vec3 ---------- */
aether_vec3_t aether_vec3_add(aether_vec3_t a, aether_vec3_t b) { return (aether_vec3_t){a.x+b.x, a.y+b.y, a.z+b.z}; }
aether_vec3_t aether_vec3_sub(aether_vec3_t a, aether_vec3_t b) { return (aether_vec3_t){a.x-b.x, a.y-b.y, a.z-b.z}; }
aether_vec3_t aether_vec3_scale(aether_vec3_t v, f32 s)         { return (aether_vec3_t){v.x*s, v.y*s, v.z*s}; }
f32           aether_vec3_dot(aether_vec3_t a, aether_vec3_t b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
aether_vec3_t aether_vec3_cross(aether_vec3_t a, aether_vec3_t b) {
    return (aether_vec3_t){
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}
f32 aether_vec3_len(aether_vec3_t v) { return sqrtf(aether_vec3_dot(v, v)); }
aether_vec3_t aether_vec3_normalize(aether_vec3_t v) {
    f32 l = aether_vec3_len(v);
    if (l < 1e-6f) return (aether_vec3_t){0,0,0};
    return aether_vec3_scale(v, 1.0f/l);
}

/* ---------- Vec4 ---------- */
aether_vec4_t aether_vec4_add(aether_vec4_t a, aether_vec4_t b) { return (aether_vec4_t){a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w}; }
aether_vec4_t aether_vec4_scale(aether_vec4_t v, f32 s)         { return (aether_vec4_t){v.x*s, v.y*s, v.z*s, v.w*s}; }
f32           aether_vec4_dot(aether_vec4_t a, aether_vec4_t b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

/* ---------- Mat4 ---------- */
aether_mat4_t aether_mat4_identity(void) {
    aether_mat4_t r;
    memset(&r, 0, sizeof r);
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

aether_mat4_t aether_mat4_multiply(aether_mat4_t a, aether_mat4_t b) {
    aether_mat4_t r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r.m[j*4 + i] = a.m[0*4 + i] * b.m[j*4 + 0] +
                           a.m[1*4 + i] * b.m[j*4 + 1] +
                           a.m[2*4 + i] * b.m[j*4 + 2] +
                           a.m[3*4 + i] * b.m[j*4 + 3];
        }
    }
    return r;
}

aether_mat4_t aether_mat4_translation(aether_vec3_t t) {
    aether_mat4_t r = aether_mat4_identity();
    r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
    return r;
}

aether_mat4_t aether_mat4_scale(aether_vec3_t s) {
    aether_mat4_t r = aether_mat4_identity();
    r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
    return r;
}

aether_mat4_t aether_mat4_rotation_x(f32 radians) {
    aether_mat4_t r = aether_mat4_identity();
    f32 c = cosf(radians), s = sinf(radians);
    r.m[5] = c;  r.m[6] = s;
    r.m[9] = -s; r.m[10] = c;
    return r;
}

aether_mat4_t aether_mat4_rotation_y(f32 radians) {
    aether_mat4_t r = aether_mat4_identity();
    f32 c = cosf(radians), s = sinf(radians);
    r.m[0] = c;  r.m[2] = -s;
    r.m[8] = s;  r.m[10] = c;
    return r;
}

aether_mat4_t aether_mat4_rotation_z(f32 radians) {
    aether_mat4_t r = aether_mat4_identity();
    f32 c = cosf(radians), s = sinf(radians);
    r.m[0] = c;  r.m[1] = s;
    r.m[4] = -s; r.m[5] = c;
    return r;
}

aether_mat4_t aether_mat4_perspective(f32 fov, f32 aspect, f32 near_z, f32 far_z) {
    aether_mat4_t r;
    memset(&r, 0, sizeof r);
    f32 tan_half = tanf(fov * 0.5f);
    r.m[0]  = 1.0f / (aspect * tan_half);
    r.m[5]  = 1.0f / tan_half;
    r.m[10] = -(far_z + near_z) / (far_z - near_z);
    r.m[11] = -1.0f;
    r.m[14] = -(2.0f * far_z * near_z) / (far_z - near_z);
    return r;
}

aether_mat4_t aether_mat4_look_at(aether_vec3_t eye, aether_vec3_t center, aether_vec3_t up) {
    aether_vec3_t f = aether_vec3_normalize(aether_vec3_sub(center, eye));
    aether_vec3_t s = aether_vec3_normalize(aether_vec3_cross(f, up));
    aether_vec3_t u = aether_vec3_cross(s, f);

    aether_mat4_t r = aether_mat4_identity();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -aether_vec3_dot(s, eye);
    r.m[13] = -aether_vec3_dot(u, eye);
    r.m[14] =  aether_vec3_dot(f, eye);
    return r;
}
