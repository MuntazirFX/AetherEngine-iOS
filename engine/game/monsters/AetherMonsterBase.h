/* AetherMonsterBase.h — Base monster class with AI state machine.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MONSTER_BASE_H
#define AETHER_MONSTER_BASE_H

#include "AetherMonsterTypes.h"
#include "../../entity/AetherEntityBase.h"
#include "../../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AI states */
typedef enum aether_monster_state {
    AETHER_MST_IDLE = 0,
    AETHER_MST_ALERT,
    AETHER_MST_CHASE,
    AETHER_MST_ATTACK,
    AETHER_MST_RETREAT,
    AETHER_MST_DEAD,
    AETHER_MST_COUNT
} aether_monster_state_t;

/* Attack types */
typedef enum aether_attack_type {
    AETHER_ATK_NONE = 0,
    AETHER_ATK_MELEE,
    AETHER_ATK_RANGED,
    AETHER_ATK_PROJECTILE,
    AETHER_ATK_SPECIAL,
} aether_attack_type_t;

/* Monster definition (immutable, one per monster type) */
typedef struct aether_monster_def {
    aether_monster_id_t        id;
    const char                *display_name;
    const char                *classname;         /* "monster_zombie" */
    aether_monster_category_t  category;
    aether_monster_disposition_t default_disposition;

    /* Stats */
    f32   max_health;
    f32   move_speed;
    f32   run_speed;
    f32   turn_speed;         /* rad/sec */
    f32   attack_range;
    f32   sight_range;
    f32   hearing_range;
    f32   field_of_view;      /* radians, half-angle */

    /* Attack */
    aether_attack_type_t attack_type;
    f32   attack_damage;
    f32   attack_cooldown;
    f32   projectile_speed;
    i32   melee_hits_per_swing;

    /* Model */
    const char *model_path;   /* "models/zombie.mdl" */
    f32   model_scale;

    /* Flags */
    bool  is_flying;
    bool  is_swimming;
    bool  is_boss;
    bool  can_open_doors;
    bool  has_ranged_attack;

    /* Sounds (optional) */
    const char *sound_idle;
    const char *sound_alert;
    const char *sound_attack;
    const char *sound_pain;
    const char *sound_death;
} aether_monster_def_t;

/* Runtime monster state */
typedef struct aether_monster {
    aether_entity_t            *entity;        /* linked runtime entity */
    aether_monster_id_t         id;
    const aether_monster_def_t *def;

    aether_monster_state_t      state;
    f32                         state_time;    /* time in current state */
    f32                         next_attack;
    f32                         next_think;

    /* Navigation */
    aether_vec3_t               target_pos;
    aether_vec3_t               last_known_enemy_pos;
    aether_entity_t            *enemy;
    f32                         lost_enemy_time;

    /* Animation */
    i32                         sequence;
    f32                         anim_time;
    bool                        attacking;
    bool                        dying;
} aether_monster_t;

/* ---------- Lifecycle ---------- */
void aether_monster_init(aether_monster_t *m, aether_monster_id_t id);
void aether_monster_link_entity(aether_monster_t *m, aether_entity_t *e);
void aether_monster_set_enemy(aether_monster_t *m, aether_entity_t *enemy);

/* ---------- State machine ---------- */
void aether_monster_set_state(aether_monster_t *m, aether_monster_state_t state);
const char *aether_monster_state_name(aether_monster_state_t state);

/* ---------- AI tick ---------- */
void aether_monster_tick(aether_monster_t *m, f32 dt);

/* ---------- Attack ---------- */
bool aether_monster_can_attack(const aether_monster_t *m, f32 now);
bool aether_monster_do_attack(aether_monster_t *m, f32 now);

/* ---------- Damage / Death ---------- */
void aether_monster_take_damage(aether_monster_t *m, f32 damage, aether_entity_t *attacker);
void aether_monster_die(aether_monster_t *m);

/* ---------- Diagnostics ---------- */
void aether_monster_dump(const aether_monster_t *m);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MONSTER_BASE_H */
