/* AetherMonsterTypes.h — Monster class names + type constants.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MONSTER_TYPES_H
#define AETHER_MONSTER_TYPES_H

#include "../../core/AetherCore.h"

/* Monster IDs (matching GoldSrc) */
typedef enum aether_monster_id {
    AETHER_MON_NONE = 0,
    AETHER_MON_HEADCRAB,
    AETHER_MON_ZOMBIE,
    AETHER_MON_ZOMBIE_SOLDIER,
    AETHER_MON_ZOMBIE_GONOME,
    AETHER_MON_BARNACLE,
    AETHER_MON_HOUNDEYE,
    AETHER_MON_BULLSQUID,
    AETHER_MON_ALIEN_GRUNT,
    AETHER_MON_ALIEN_SLAVE,
    AETHER_MON_ALIEN_CONTROLLER,
    AETHER_MON_GARGANTUA,
    AETHER_MON_HUMAN_GRUNT,
    AETHER_MON_HUMAN_SERGEANT,
    AETHER_MON_SCIENTIST,
    AETHER_MON_BARNEY,
    AETHER_MON_GMAN,
    AETHER_MON_APACHE,
    AETHER_MON_OSPREY,
    AETHER_MON_ICHTHYOSAUR,
    AETHER_MON_LEECH,
    AETHER_MON_TURRET,
    AETHER_MON_COUNT
} aether_monster_id_t;

/* Monster relationship (GoldSrc style) */
typedef enum aether_monster_disposition {
    AETHER_DISP_NEUTRAL = 0,
    AETHER_DISP_FRIENDLY,
    AETHER_DISP_HOSTILE,
    AETHER_DISP_FEAR,
} aether_monster_disposition_t;

/* Monster category for spawning/budgeting */
typedef enum aether_monster_category {
    AETHER_MCAT_NONE = 0,
    AETHER_MCAT_ALIEN,
    AETHER_MCAT_HUMAN,
    AETHER_MCAT_FRIENDLY,
    AETHER_MCAT_MACHINE,
    AETHER_MCAT_WATER,
    AETHER_MCAT_BOSS,
} aether_monster_category_t;

/* Class name strings */
#define AETHER_CLS_HEADCRAB        "monster_headcrab"
#define AETHER_CLS_ZOMBIE          "monster_zombie"
#define AETHER_CLS_ZOMBIE_SOLDIER  "monster_zombie_soldier"
#define AETHER_CLS_ZOMBIE_GONOME   "monster_zombie_gonome"
#define AETHER_CLS_BARNACLE        "monster_barnacle"
#define AETHER_CLS_HOUNDEYE        "monster_houndeye"
#define AETHER_CLS_BULLSQUID       "monster_bullsquid"
#define AETHER_CLS_ALIEN_GRUNT     "monster_alien_grunt"
#define AETHER_CLS_ALIEN_SLAVE     "monster_alien_slave"
#define AETHER_CLS_ALIEN_CONTROLLER "monster_alien_controller"
#define AETHER_CLS_GARGANTUA       "monster_gargantua"
#define AETHER_CLS_HUMAN_GRUNT     "monster_human_grunt"
#define AETHER_CLS_HUMAN_SERGEANT  "monster_human_sergeant"
#define AETHER_CLS_SCIENTIST       "monster_scientist"
#define AETHER_CLS_BARNEY          "monster_barney"
#define AETHER_CLS_GMAN            "monster_gman"
#define AETHER_CLS_APACHE          "monster_apache"
#define AETHER_CLS_OSPREY          "monster_osprey"
#define AETHER_CLS_ICHTHYOSAUR     "monster_ichthyosaur"
#define AETHER_CLS_LEECH           "monster_leech"
#define AETHER_CLS_TURRET          "monster_turret"

#endif /* AETHER_MONSTER_TYPES_H */
