/* AetherWeaponView.h — First-person viewmodel animation + positioning.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_WEAPON_VIEW_H
#define AETHER_WEAPON_VIEW_H

#include "AetherWeapon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aether_view_anim {
    AETHER_VIEW_ANIM_IDLE = 0,
    AETHER_VIEW_ANIM_FIRE,
    AETHER_VIEW_ANIM_RELOAD,
    AETHER_VIEW_ANIM_DRAW,
    AETHER_VIEW_ANIM_HOLSTER,
} aether_view_anim_t;

typedef struct aether_weapon_view {
    aether_weapon_id_t  weapon;
    aether_view_anim_t  current_anim;
    f32                 anim_time;       /* elapsed */
    f32                 anim_length;     /* duration */
    bool                anim_looping;
    aether_vec3_t       offset;          /* screen-space offset */
    aether_vec3_t       angles;          /* rotation */
    f32                 bob_phase;       /* walk bob */
    f32                 bob_amount;      /* how much bob from movement */
} aether_weapon_view_t;

void aether_weapon_view_init(aether_weapon_view_t *v, aether_weapon_id_t id);
void aether_weapon_view_play(aether_weapon_view_t *v, aether_view_anim_t anim);
void aether_weapon_view_tick(aether_weapon_view_t *v, f32 dt,
                              f32 player_speed, bool on_ground);
void aether_weapon_view_compute_transform(const aether_weapon_view_t *v,
                                           aether_vec3_t eye_pos,
                                           aether_vec3_t eye_angles,
                                           aether_vec3_t *out_pos,
                                           aether_vec3_t *out_angles);
void aether_weapon_view_dump(const aether_weapon_view_t *v);

/* Simple viewmodel stub: 2 tris (quad) in view space for Metal draw.
 * out verts: x,y,z,u,v,r,g,b,a (9 floats). Returns vertex count (6). */
typedef struct aether_viewmodel_vertex {
    f32 x, y, z;
    f32 u, v;
    f32 r, g, b, a;
} aether_viewmodel_vertex_t;

u32 aether_weapon_view_copy_stub(const aether_weapon_view_t *v,
                                 aether_viewmodel_vertex_t *out, u32 max_verts);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_WEAPON_VIEW_H */
