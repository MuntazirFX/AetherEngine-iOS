/* AetherEntityBase.h — Runtime entity system (STEP 18A).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_ENTITY_BASE_H
#define AETHER_ENTITY_BASE_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_ENTITY_CLASSNAME_MAX  64
#define AETHER_ENTITY_TARGETNAME_MAX 64
#define AETHER_ENTITY_MODEL_MAX      128
#define AETHER_ENTITY_MAX            4096

typedef enum aether_entity_flags {
    AETHER_ENT_FLAG_NONE         = 0,
    AETHER_ENT_FLAG_ACTIVE       = (1u << 0),
    AETHER_ENT_FLAG_VISIBLE      = (1u << 1),
    AETHER_ENT_FLAG_SOLID        = (1u << 2),
    AETHER_ENT_FLAG_ON_GROUND    = (1u << 3),
    AETHER_ENT_FLAG_CLIENT_ONLY  = (1u << 4),
    AETHER_ENT_FLAG_PENDING_KILL = (1u << 5),
} aether_entity_flags_t;

typedef enum aether_model_type {
    AETHER_MODEL_NONE = 0,
    AETHER_MODEL_BRUSH,
    AETHER_MODEL_STUDIO,
    AETHER_MODEL_SPRITE,
} aether_model_type_t;

typedef struct aether_entity aether_entity_t;

typedef void (*aether_spawn_fn)  (aether_entity_t *e);
typedef void (*aether_think_fn)  (aether_entity_t *e);
typedef void (*aether_use_fn)    (aether_entity_t *e, aether_entity_t *activator);
typedef void (*aether_touch_fn)  (aether_entity_t *e, aether_entity_t *other);
typedef void (*aether_blocked_fn)(aether_entity_t *e, aether_entity_t *other);
typedef void (*aether_damage_fn) (aether_entity_t *e, aether_entity_t *attacker, f32 damage);
typedef void (*aether_destroy_fn)(aether_entity_t *e);

struct aether_entity {
    u32   id;
    char  classname[AETHER_ENTITY_CLASSNAME_MAX];
    char  targetname[AETHER_ENTITY_TARGETNAME_MAX];

    aether_vec3_t origin;
    aether_vec3_t angles;
    aether_vec3_t velocity;
    aether_vec3_t avelocity;

    aether_vec3_t mins;
    aether_vec3_t maxs;

    f32   health;
    f32   max_health;
    f32   armor;
    f32   damage_taken;

    aether_model_type_t model_type;
    char                model_name[AETHER_ENTITY_MODEL_MAX];
    u32                 model_index;

    f32   gravity;
    f32   friction;
    f32   speed;

    u32   flags;
    f32   next_think;
    f32   anim_time;
    i32   sequence;
    i32   body;
    i32   skin;

    aether_entity_t *owner;
    aether_entity_t *enemy;
    aether_entity_t *ground_entity;

    aether_spawn_fn    spawn;
    aether_think_fn    think;
    aether_use_fn      use;
    aether_touch_fn    touch;
    aether_blocked_fn  blocked;
    aether_damage_fn   take_damage;
    aether_destroy_fn  on_destroy;

    void *user_data;
};

typedef struct aether_entity_mgr aether_entity_mgr_t;

aether_entity_mgr_t *aether_entity_mgr_create(void);
void                 aether_entity_mgr_destroy(aether_entity_mgr_t *mgr);

aether_entity_t *aether_entity_spawn(aether_entity_mgr_t *mgr, const char *classname);
aether_entity_t *aether_entity_find_by_class(aether_entity_mgr_t *mgr, const char *classname);
aether_entity_t *aether_entity_find_by_name (aether_entity_mgr_t *mgr, const char *targetname);
void             aether_entity_remove(aether_entity_t *e);
void             aether_entity_mgr_tick(aether_entity_mgr_t *mgr, f32 dt);

u32                    aether_entity_mgr_count(const aether_entity_mgr_t *mgr);
const aether_entity_t *aether_entity_mgr_at(const aether_entity_mgr_t *mgr, u32 idx);
u32                    aether_entity_mgr_active_count(const aether_entity_mgr_t *mgr);

void aether_entity_set_origin(aether_entity_t *e, aether_vec3_t origin);
void aether_entity_set_angles(aether_entity_t *e, aether_vec3_t angles);
void aether_entity_set_model (aether_entity_t *e, const char *model);
void aether_entity_set_health(aether_entity_t *e, f32 hp);
void aether_entity_apply_damage(aether_entity_t *e, f32 dmg, aether_entity_t *attacker);
void aether_entity_apply_gravity(aether_entity_t *e, f32 dt);
void aether_entity_set_next_think(aether_entity_t *e, f32 delay);
void aether_entity_dump(const aether_entity_t *e);
void aether_entity_mgr_dump(const aether_entity_mgr_t *mgr);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_ENTITY_BASE_H */
