/* AetherEntityTypes.h — Entity class names and type constants.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_ENTITY_TYPES_H
#define AETHER_ENTITY_TYPES_H

/* ---------- Players ---------- */
#define AETHER_ENT_INFO_PLAYER_START       "info_player_start"
#define AETHER_ENT_INFO_PLAYER_DEATHMATCH  "info_player_deathmatch"
#define AETHER_ENT_INFO_PLAYER_COOP        "info_player_coop"

/* ---------- Lights ---------- */
#define AETHER_ENT_LIGHT                   "light"
#define AETHER_ENT_LIGHT_SPOT              "light_spot"
#define AETHER_ENT_LIGHT_ENVIRONMENT       "light_environment"
#define AETHER_ENT_LIGHT_GLOW              "light_glow"

/* ---------- Monsters ---------- */
#define AETHER_ENT_MONSTER_HEADCRAB        "monster_headcrab"
#define AETHER_ENT_MONSTER_ZOMBIE          "monster_zombie"
#define AETHER_ENT_MONSTER_BARNACLE        "monster_barnacle"
#define AETHER_ENT_MONSTER_HOUNDEYE        "monster_houndeye"
#define AETHER_ENT_MONSTER_BULLSQUID       "monster_bullsquid"
#define AETHER_ENT_MONSTER_ALIEN_GRUNT     "monster_alien_grunt"
#define AETHER_ENT_MONSTER_ALIEN_SLAVE     "monster_alien_slave"
#define AETHER_ENT_MONSTER_ALIEN_CONT      "monster_alien_controller"
#define AETHER_ENT_MONSTER_GARGANTUA       "monster_gargantua"
#define AETHER_ENT_MONSTER_HUMAN_GRUNT     "monster_human_grunt"
#define AETHER_ENT_MONSTER_HUMAN_SERGEANT  "monster_human_sergeant"
#define AETHER_ENT_MONSTER_SCIENTIST       "monster_scientist"
#define AETHER_ENT_MONSTER_BARNEY          "monster_barney"
#define AETHER_ENT_MONSTER_GMAN            "monster_gman"
#define AETHER_ENT_MONSTER_APACHE          "monster_apache"
#define AETHER_ENT_MONSTER_OSPREY          "monster_osprey"
#define AETHER_ENT_MONSTER_ICHTHYOSAUR     "monster_ichthyosaur"
#define AETHER_ENT_MONSTER_LEECH           "monster_leech"
#define AETHER_ENT_MONSTER_TURRET          "monster_turret"

/* ---------- Weapons (pickups) ---------- */
#define AETHER_ENT_WEAPON_CROWBAR          "weapon_crowbar"
#define AETHER_ENT_WEAPON_GLOCK            "weapon_9mmhandgun"
#define AETHER_ENT_WEAPON_PYTHON           "weapon_357"
#define AETHER_ENT_WEAPON_MP5              "weapon_9mmAR"
#define AETHER_ENT_WEAPON_SHOTGUN          "weapon_shotgun"
#define AETHER_ENT_WEAPON_CROSSBOW         "weapon_crossbow"
#define AETHER_ENT_WEAPON_RPG              "weapon_rpg"
#define AETHER_ENT_WEAPON_GAUSS            "weapon_gauss"
#define AETHER_ENT_WEAPON_EGON             "weapon_egon"
#define AETHER_ENT_WEAPON_HIVEHAND         "weapon_hivehand"
#define AETHER_ENT_WEAPON_GRENADE          "weapon_handgrenade"
#define AETHER_ENT_WEAPON_SATCHEL          "weapon_satchel"
#define AETHER_ENT_WEAPON_TRIPMINE         "weapon_tripmine"
#define AETHER_ENT_WEAPON_SNARK            "weapon_snark"

/* ---------- Items ---------- */
#define AETHER_ENT_ITEM_SUIT               "item_suit"
#define AETHER_ENT_ITEM_HEALTHKIT          "item_healthkit"
#define AETHER_ENT_ITEM_BATTERY            "item_battery"
#define AETHER_ENT_ITEM_ANTIDOTE           "item_antidote"
#define AETHER_ENT_ITEM_SECURITY           "item_security"
#define AETHER_ENT_ITEM_LONGJUMP           "item_longjump"

/* ---------- Ammo ---------- */
#define AETHER_ENT_AMMO_9MM                "ammo_9mmclip"
#define AETHER_ENT_AMMO_357                "ammo_357"
#define AETHER_ENT_AMMO_BUCKSHOT           "ammo_buckshot"
#define AETHER_ENT_AMMO_BOLT               "ammo_crossbow"
#define AETHER_ENT_AMMO_RPG                "ammo_rpgclip"
#define AETHER_ENT_AMMO_URANIUM            "ammo_gaussclip"
#define AETHER_ENT_AMMO_ARGREN             "ammo_ARgrenade"
#define AETHER_ENT_AMMO_HANDGRENADE        "ammo_handgrenade"

/* ---------- Triggers ---------- */
#define AETHER_ENT_TRIGGER_MULTIPLE        "trigger_multiple"
#define AETHER_ENT_TRIGGER_ONCE            "trigger_once"
#define AETHER_ENT_TRIGGER_HURT            "trigger_hurt"
#define AETHER_ENT_TRIGGER_PUSH            "trigger_push"
#define AETHER_ENT_TRIGGER_TELEPORT        "trigger_teleport"
#define AETHER_ENT_TRIGGER_CHANGELEVEL     "trigger_changelevel"
#define AETHER_ENT_TRIGGER_GRAVITY         "trigger_gravity"
#define AETHER_ENT_TRIGGER_AUTO            "trigger_auto"
#define AETHER_ENT_TRIGGER_CAMERA          "trigger_camera"

/* ---------- Functionals ---------- */
#define AETHER_ENT_FUNC_DOOR               "func_door"
#define AETHER_ENT_FUNC_DOOR_ROTATING      "func_door_rotating"
#define AETHER_ENT_FUNC_BUTTON             "func_button"
#define AETHER_ENT_FUNC_BREAKABLE          "func_breakable"
#define AETHER_ENT_FUNC_PUSHABLE           "func_pushable"
#define AETHER_ENT_FUNC_TRAIN              "func_train"
#define AETHER_ENT_FUNC_PLAT               "func_plat"
#define AETHER_ENT_FUNC_PLATROT            "func_platrot"
#define AETHER_ENT_FUNC_WATER              "func_water"
#define AETHER_ENT_FUNC_WALL               "func_wall"
#define AETHER_ENT_FUNC_ILLUSIONARY        "func_illusionary"
#define AETHER_ENT_FUNC_HEALTHCHARGER      "func_healthcharger"
#define AETHER_ENT_FUNC_RECHARGER          "func_recharge"
#define AETHER_ENT_FUNC_TANK               "func_tank"
#define AETHER_ENT_FUNC_TANKLASER          "func_tanklaser"
#define AETHER_ENT_FUNC_TANKROCKET         "func_tankrocket"
#define AETHER_ENT_FUNC_TRACKTRAIN         "func_tracktrain"
#define AETHER_ENT_FUNC_ROTATING           "func_rotating"
#define AETHER_ENT_FUNC_CONVEYOR           "func_conveyor"

/* ---------- Environment ---------- */
#define AETHER_ENT_ENV_EXPLOSION           "env_explosion"
#define AETHER_ENT_ENV_FIRE                "env_fire"
#define AETHER_ENT_ENV_SMOKER              "env_smoker"
#define AETHER_ENT_ENV_SPARK               "env_spark"
#define AETHER_ENT_ENV_BEAM                "env_beam"
#define AETHER_ENT_ENV_LIGHTNING           "env_lightning"
#define AETHER_ENT_ENV_LASER               "env_laser"
#define AETHER_ENT_ENV_GLOW                "env_glow"
#define AETHER_ENT_ENV_SOUND               "env_sound"
#define AETHER_ENT_ENV_SHAKE               "env_shake"
#define AETHER_ENT_ENV_FADE                "env_fade"
#define AETHER_ENT_ENV_RENDER              "env_render"
#define AETHER_ENT_ENV_BUBBLES             "env_bubbles"

/* ---------- Ambient ---------- */
#define AETHER_ENT_AMBIENT_GENERIC         "ambient_generic"

/* ---------- Info ---------- */
#define AETHER_ENT_INFO_TARGET             "info_target"
#define AETHER_ENT_INFO_TELEPORT_DEST      "info_teleport_destination"
#define AETHER_ENT_INFO_LANDMARK           "info_landmark"
#define AETHER_ENT_INFO_NODE               "info_node"
#define AETHER_ENT_INFO_NODE_HGRUNT        "info_node_hgrunt"
#define AETHER_ENT_INFO_NODE_AIR           "info_node_air"

/* ---------- Paths ---------- */
#define AETHER_ENT_PATH_CORNER             "path_corner"
#define AETHER_ENT_PATH_TRACK              "path_track"

/* ---------- Game rules ---------- */
#define AETHER_ENT_GAME_COUNTER            "game_counter"
#define AETHER_ENT_GAME_TEXT               "game_text"
#define AETHER_ENT_GAME_PLAYER_EQUIP       "game_player_equip"
#define AETHER_ENT_GAME_PLAYER_HURT        "game_player_hurt"
#define AETHER_ENT_GAME_PLAYER_TEAM        "game_player_team"
#define AETHER_ENT_GAME_ZONE_PLAYER        "game_zone_player"

/* ---------- Misc ---------- */
#define AETHER_ENT_MULTI_MANAGER           "multi_manager"
#define AETHER_ENT_SCRIPTED_SEQUENCE       "scripted_sequence"
#define AETHER_ENT_MOMENTARY_ROT_BUTTON    "momentary_rot_button"

#endif /* AETHER_ENTITY_TYPES_H */
