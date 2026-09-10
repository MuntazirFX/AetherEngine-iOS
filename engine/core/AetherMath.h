/* AetherMath.h — 3D Math library (Vectors, Matrices) for AetherEngine.
 * Clean-room implementation. Uses AetherCore types.
 */
#ifndef AETHER_MATH_H
#define AETHER_MATH_H

#include "AetherCore.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_PI 3.14159265358979323846f

/* ---------- Vector 2 ---------- */
typedef struct aether_vec2 { f32 x, y; } aether_vec2_t;
aether_vec2_t aether_vec2_add(aether_vec2_t a, aether_vec2_t b);
aether_vec2_t aether_vec2_sub(aether_vec2_t a, aether_vec2_t b);
aether_vec2_t aether_vec2_scale(aether_vec2_t v, f32 s);
f32           aether_vec2_dot(aether_vec2_t a, aether_vec2_t b);
f32           aether_vec2_len(aether_vec2_t v);
aether_vec2_t aether_vec2_normalize(aether_vec2_t v);

/* ---------- Vector 3 ---------- */
typedef struct aether_vec3 { f32 x, y, z; } aether_vec3_t;
aether_vec3_t aether_vec3_add(aether_vec3_t a, aether_vec3_t b);
aether_vec3_t aether_vec3_sub(aether_vec3_t a, aether_vec3_t b);
aether_vec3_t aether_vec3_scale(aether_vec3_t v, f32 s);
f32           aether_vec3_dot(aether_vec3_t a, aether_vec3_t b);
aether_vec3_t aether_vec3_cross(aether_vec3_t a, aether_vec3_t b);
f32           aether_vec3_len(aether_vec3_t v);
aether_vec3_t aether_vec3_normalize(aether_vec3_t v);

/* ---------- Vector 4 ---------- */
typedef struct aether_vec4 { f32 x, y, z, w; } aether_vec4_t;
aether_vec4_t aether_vec4_add(aether_vec4_t a, aether_vec4_t b);
aether_vec4_t aether_vec4_scale(aether_vec4_t v, f32 s);
f32           aether_vec4_dot(aether_vec4_t a, aether_vec4_t b);

/* ---------- Matrix 4x4 (Column-Major, like OpenGL/Metal) ---------- */
typedef struct aether_mat4 { f32 m[16]; } aether_mat4_t;

aether_mat4_t aether_mat4_identity(void);
aether_mat4_t aether_mat4_multiply(aether_mat4_t a, aether_mat4_t b);
aether_mat4_t aether_mat4_translation(aether_vec3_t t);
aether_mat4_t aether_mat4_scale(aether_vec3_t s);
aether_mat4_t aether_mat4_rotation_x(f32 radians);
aether_mat4_t aether_mat4_rotation_y(f32 radians);
aether_mat4_t aether_mat4_rotation_z(f32 radians);
aether_mat4_t aether_mat4_perspective(f32 fov_radians, f32 aspect, f32 near_z, f32 far_z);
aether_mat4_t aether_mat4_look_at(aether_vec3_t eye, aether_vec3_t center, aether_vec3_t up);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MATH_H */
