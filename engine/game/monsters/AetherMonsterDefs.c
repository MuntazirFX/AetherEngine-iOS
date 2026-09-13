/* AetherMonsterDefs.c — Data table of all 21 monsters.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMonsterDefs.h"
#include <string.h>

static const aether_monster_def_t k_monsters[AETHER_MON_COUNT] = {
    /* 0: NONE */
    { AETHER_MON_NONE, "None", NULL, AETHER_MCAT_NONE, AETHER_DISP_NEUTRAL,
      0,0,0,0,0,0,0,0, AETHER_ATK_NONE,0,0,0,0, NULL,1.0f,
      false,false,false,false,false, NULL,NULL,NULL,NULL,NULL },

    /* 1: HEADCRAB */
    { AETHER_MON_HEADCRAB, "Headcrab", AETHER_CLS_HEADCRAB,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      10.0f, 140.0f, 260.0f, 3.5f, 45.0f, 600.0f, 500.0f, 1.2f,
      AETHER_ATK_MELEE, 10.0f, 0.8f, 0.0f, 1, "models/headcrab.mdl", 1.0f,
      false,false,false,false,false,
      "headcrab_idle1","headcrab_alert1","headcrab_attack1","headcrab_pain1","headcrab_die1" },

    /* 2: ZOMBIE */
    { AETHER_MON_ZOMBIE, "Zombie", AETHER_CLS_ZOMBIE,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      50.0f, 90.0f, 180.0f, 2.5f, 55.0f, 500.0f, 400.0f, 1.0f,
      AETHER_ATK_MELEE, 20.0f, 1.2f, 0.0f, 1, "models/zombie.mdl", 1.0f,
      false,false,false,true,false,
      "zombie_idle1","zombie_alert1","zombie_attack1","zombie_pain1","zombie_die1" },

    /* 3: ZOMBIE SOLDIER (OpFor) */
    { AETHER_MON_ZOMBIE_SOLDIER, "Zombie Soldier", AETHER_CLS_ZOMBIE_SOLDIER,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      60.0f, 100.0f, 190.0f, 2.5f, 60.0f, 600.0f, 500.0f, 1.1f,
      AETHER_ATK_MELEE, 25.0f, 1.0f, 0.0f, 1, "models/zombie_soldier.mdl", 1.0f,
      false,false,false,true,false,
      NULL,"zombie_soldier_alert","zombie_soldier_attack",NULL,NULL },

    /* 4: ZOMBIE GONOME (OpFor) */
    { AETHER_MON_ZOMBIE_GONOME, "Gonome", AETHER_CLS_ZOMBIE_GONOME,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      120.0f, 110.0f, 220.0f, 2.8f, 75.0f, 700.0f, 600.0f, 1.1f,
      AETHER_ATK_MELEE, 40.0f, 1.5f, 0.0f, 1, "models/gonome.mdl", 1.1f,
      false,false,false,true,false, NULL,NULL,NULL,NULL,NULL },

    /* 5: BARNACLE */
    { AETHER_MON_BARNACLE, "Barnacle", AETHER_CLS_BARNACLE,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      25.0f, 0.0f, 0.0f, 3.0f, 128.0f, 500.0f, 0.0f, 0.6f,
      AETHER_ATK_MELEE, 25.0f, 2.0f, 0.0f, 1, "models/barnacle.mdl", 1.0f,
      false,false,false,false,false, "barnacle_idle1",NULL,"barnacle_attack1","barnacle_pain1","barnacle_die1" },

    /* 6: HOUNDEYE */
    { AETHER_MON_HOUNDEYE, "Houndeye", AETHER_CLS_HOUNDEYE,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      20.0f, 110.0f, 220.0f, 3.0f, 100.0f, 700.0f, 600.0f, 1.0f,
      AETHER_ATK_SPECIAL, 15.0f, 1.8f, 0.0f, 1, "models/houndeye.mdl", 1.0f,
      false,false,false,false,true, NULL,NULL,NULL,NULL,NULL },

    /* 7: BULLSQUID */
    { AETHER_MON_BULLSQUID, "Bullsquid", AETHER_CLS_BULLSQUID,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      40.0f, 120.0f, 260.0f, 2.8f, 400.0f, 700.0f, 600.0f, 1.0f,
      AETHER_ATK_PROJECTILE, 15.0f, 1.5f, 900.0f, 1, "models/bullsquid.mdl", 1.0f,
      false,false,false,false,true, NULL,NULL,NULL,NULL,NULL },

    /* 8: ALIEN GRUNT */
    { AETHER_MON_ALIEN_GRUNT, "Alien Grunt", AETHER_CLS_ALIEN_GRUNT,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      90.0f, 110.0f, 220.0f, 3.0f, 500.0f, 900.0f, 800.0f, 1.1f,
      AETHER_ATK_PROJECTILE, 20.0f, 0.7f, 1000.0f, 1, "models/agrunt.mdl", 1.0f,
      false,false,false,true,true, NULL,NULL,NULL,NULL,NULL },

    /* 9: ALIEN SLAVE (Vortigaunt) */
    { AETHER_MON_ALIEN_SLAVE, "Alien Slave", AETHER_CLS_ALIEN_SLAVE,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      30.0f, 90.0f, 180.0f, 3.5f, 500.0f, 800.0f, 700.0f, 1.1f,
      AETHER_ATK_SPECIAL, 10.0f, 1.0f, 0.0f, 1, "models/islave.mdl", 1.0f,
      false,false,false,true,false, NULL,NULL,NULL,NULL,NULL },

    /* 10: ALIEN CONTROLLER */
    { AETHER_MON_ALIEN_CONTROLLER, "Alien Controller", AETHER_CLS_ALIEN_CONTROLLER,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      60.0f, 150.0f, 300.0f, 3.0f, 700.0f, 900.0f, 800.0f, 1.1f,
      AETHER_ATK_PROJECTILE, 20.0f, 1.2f, 1200.0f, 1, "models/controller.mdl", 1.0f,
      true,false,false,false,true, NULL,NULL,NULL,NULL,NULL },

    /* 11: GARGANTUA */
    { AETHER_MON_GARGANTUA, "Gargantua", AETHER_CLS_GARGANTUA,
      AETHER_MCAT_ALIEN, AETHER_DISP_HOSTILE,
      800.0f, 110.0f, 220.0f, 2.0f, 200.0f, 1000.0f, 900.0f, 1.0f,
      AETHER_ATK_SPECIAL, 100.0f, 2.0f, 0.0f, 2, "models/garg.mdl", 2.5f,
      false,false,true,true,true, NULL,NULL,NULL,NULL,NULL },

    /* 12: HUMAN GRUNT */
    { AETHER_MON_HUMAN_GRUNT, "HECU Grunt", AETHER_CLS_HUMAN_GRUNT,
      AETHER_MCAT_HUMAN, AETHER_DISP_HOSTILE,
      80.0f, 140.0f, 280.0f, 4.0f, 1000.0f, 1200.0f, 1000.0f, 1.2f,
      AETHER_ATK_PROJECTILE, 12.0f, 0.5f, 1500.0f, 1, "models/hgrunt.mdl", 1.0f,
      false,false,false,true,true, NULL,NULL,NULL,NULL,NULL },

    /* 13: HUMAN SERGEANT */
    { AETHER_MON_HUMAN_SERGEANT, "HECU Sergeant", AETHER_CLS_HUMAN_SERGEANT,
      AETHER_MCAT_HUMAN, AETHER_DISP_HOSTILE,
      100.0f, 140.0f, 280.0f, 4.0f, 1000.0f, 1200.0f, 1000.0f, 1.2f,
      AETHER_ATK_PROJECTILE, 12.0f, 0.4f, 1500.0f, 1, "models/hgrunt.mdl", 1.0f,
      false,false,false,true,true, NULL,NULL,NULL,NULL,NULL },

    /* 14: SCIENTIST */
    { AETHER_MON_SCIENTIST, "Scientist", AETHER_CLS_SCIENTIST,
      AETHER_MCAT_FRIENDLY, AETHER_DISP_FRIENDLY,
      20.0f, 80.0f, 180.0f, 3.0f, 0.0f, 500.0f, 400.0f, 1.2f,
      AETHER_ATK_NONE, 0.0f, 0.0f, 0.0f, 0, "models/scientist.mdl", 1.0f,
      false,false,false,false,false, NULL,NULL,NULL,NULL,NULL },

    /* 15: BARNEY */
    { AETHER_MON_BARNEY, "Barney", AETHER_CLS_BARNEY,
      AETHER_MCAT_FRIENDLY, AETHER_DISP_FRIENDLY,
      35.0f, 120.0f, 240.0f, 3.5f, 900.0f, 800.0f, 700.0f, 1.2f,
      AETHER_ATK_PROJECTILE, 8.0f, 0.7f, 1500.0f, 1, "models/barney.mdl", 1.0f,
      false,false,false,true,true, NULL,NULL,NULL,NULL,NULL },

    /* 16: GMAN */
    { AETHER_MON_GMAN, "G-Man", AETHER_CLS_GMAN,
      AETHER_MCAT_FRIENDLY, AETHER_DISP_NEUTRAL,
      1000.0f, 60.0f, 120.0f, 2.0f, 0.0f, 400.0f, 300.0f, 1.0f,
      AETHER_ATK_NONE, 0.0f, 0.0f, 0.0f, 0, "models/gman.mdl", 1.0f,
      false,false,false,false,false, NULL,NULL,NULL,NULL,NULL },

    /* 17: APACHE */
    { AETHER_MON_APACHE, "Apache", AETHER_CLS_APACHE,
      AETHER_MCAT_MACHINE, AETHER_DISP_HOSTILE,
      200.0f, 400.0f, 500.0f, 2.5f, 1200.0f, 1500.0f, 1200.0f, 1.2f,
      AETHER_ATK_PROJECTILE, 20.0f, 0.15f, 2000.0f, 1, "models/apache.mdl", 3.0f,
      true,false,false,false,true, NULL,NULL,NULL,NULL,NULL },

    /* 18: OSPREY */
    { AETHER_MON_OSPREY, "Osprey", AETHER_CLS_OSPREY,
      AETHER_MCAT_MACHINE, AETHER_DISP_NEUTRAL,
      300.0f, 300.0f, 400.0f, 2.0f, 0.0f, 1000.0f, 800.0f, 1.0f,
      AETHER_ATK_NONE, 0.0f, 0.0f, 0.0f, 0, "models/osprey.mdl", 4.0f,
      true,false,false,false,false, NULL,NULL,NULL,NULL,NULL },

    /* 19: ICHTHYOSAUR */
    { AETHER_MON_ICHTHYOSAUR, "Ichthyosaur", AETHER_CLS_ICHTHYOSAUR,
      AETHER_MCAT_WATER, AETHER_DISP_HOSTILE,
      200.0f, 200.0f, 400.0f, 3.0f, 250.0f, 800.0f, 700.0f, 1.0f,
      AETHER_ATK_MELEE, 25.0f, 1.2f, 0.0f, 1, "models/ichthyosaur.mdl", 2.0f,
      false,true,false,false,false, NULL,NULL,NULL,NULL,NULL },

    /* 20: LEECH */
    { AETHER_MON_LEECH, "Leech", AETHER_CLS_LEECH,
      AETHER_MCAT_WATER, AETHER_DISP_HOSTILE,
      5.0f, 120.0f, 200.0f, 2.0f, 30.0f, 300.0f, 200.0f, 1.5f,
      AETHER_ATK_MELEE, 5.0f, 1.5f, 0.0f, 1, "models/leech.mdl", 0.5f,
      false,true,false,false,false, NULL,NULL,NULL,NULL,NULL },

    /* 21: TURRET */
    { AETHER_MON_TURRET, "Turret", AETHER_CLS_TURRET,
      AETHER_MCAT_MACHINE, AETHER_DISP_HOSTILE,
      50.0f, 0.0f, 0.0f, 2.0f, 1000.0f, 1200.0f, 1000.0f, 1.2f,
      AETHER_ATK_PROJECTILE, 15.0f, 0.4f, 1500.0f, 1, "models/turret.mdl", 1.0f,
      false,false,false,false,true, NULL,NULL,NULL,NULL,NULL },
};

const aether_monster_def_t *aether_monster_defs_lookup(aether_monster_id_t id) {
    if (id <= 0 || id >= AETHER_MON_COUNT) return NULL;
    return &k_monsters[id];
}

const aether_monster_def_t *aether_monster_defs_table(void) { return k_monsters; }
u32 aether_monster_defs_count(void) { return AETHER_MON_COUNT; }
