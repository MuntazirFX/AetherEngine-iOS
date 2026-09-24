/* AetherEntitySpawn.c — Spawn runtime entities from BSP (STEP 18B).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherEntitySpawn.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Internal representation of one BSP entity (parsed). */
typedef struct bsp_entity_info {
    char classname[AETHER_ENTITY_CLASSNAME_MAX];
    char targetname[AETHER_ENTITY_TARGETNAME_MAX];
    f32  origin[3];
    f32  angles[3];
    f32  health;
    char model[AETHER_ENTITY_MODEL_MAX];
} bsp_entity_info_t;

/* ---------- Parse ENTITIES lump text ---------- */
static u32 parse_bsp_entities(const aether_bsp_t *bsp,
                               bsp_entity_info_t **out_entities) {
    if (!bsp || !out_entities) return 0;
    *out_entities = NULL;

    size_t size = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_ENTITIES);
    const u8 *raw = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_ENTITIES);
    if (!raw || size == 0) return 0;

    char *buf = (char*)malloc(size + 1);
    if (!buf) return 0;
    memcpy(buf, raw, size);
    buf[size] = 0;

    /* Count entities (open braces) */
    u32 count = 0;
    for (size_t i = 0; i < size; ++i) {
        if (buf[i] == '{') count++;
    }
    if (count == 0) { free(buf); return 0; }

    bsp_entity_info_t *list = (bsp_entity_info_t*)calloc(count, sizeof(bsp_entity_info_t));
    if (!list) { free(buf); return 0; }

    char *p = buf;
    u32 idx = 0;
    bsp_entity_info_t *cur = NULL;

    while (*p && idx < count) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;

        if (*p == '{') {
            cur = &list[idx];
            memset(cur, 0, sizeof *cur);
            p++;
            continue;
        }
        if (*p == '}') {
            if (cur) idx++;
            cur = NULL;
            p++;
            continue;
        }
        if (*p == '"' && cur) {
            p++;
            char key[64] = {0};
            int ki = 0;
            while (*p && *p != '"' && ki < 63) key[ki++] = *p++;
            key[ki] = 0;
            if (*p == '"') p++;

            while (*p == ' ' || *p == '\t') p++;

            if (*p == '"') {
                p++;
                char val[256] = {0};
                int vi = 0;
                while (*p && *p != '"' && vi < 255) val[vi++] = *p++;
                val[vi] = 0;
                if (*p == '"') p++;

                if (aether_str_eq(key, "classname"))
                    aether_str_copy(cur->classname, AETHER_ENTITY_CLASSNAME_MAX, val);
                else if (aether_str_eq(key, "targetname"))
                    aether_str_copy(cur->targetname, AETHER_ENTITY_TARGETNAME_MAX, val);
                else if (aether_str_eq(key, "model"))
                    aether_str_copy(cur->model, AETHER_ENTITY_MODEL_MAX, val);
                else if (aether_str_eq(key, "origin"))
                    sscanf(val, "%f %f %f", &cur->origin[0], &cur->origin[1], &cur->origin[2]);
                else if (aether_str_eq(key, "angles"))
                    sscanf(val, "%f %f %f", &cur->angles[0], &cur->angles[1], &cur->angles[2]);
                else if (aether_str_eq(key, "health"))
                    cur->health = (f32)atof(val);
            }
            continue;
        }
        p++;
    }

    free(buf);
    *out_entities = list;
    return idx;
}

/* ---------- Default health per monster class ---------- */
static f32 default_health_for(const char *classname) {
    if (!classname) return 100.0f;
    if (strncmp(classname, "monster_headcrab", 16) == 0)    return 10.0f;
    if (strncmp(classname, "monster_zombie", 14) == 0)      return 50.0f;
    if (strncmp(classname, "monster_houndeye", 16) == 0)    return 20.0f;
    if (strncmp(classname, "monster_bullsquid", 17) == 0)   return 40.0f;
    if (strncmp(classname, "monster_alien_grunt", 19) == 0) return 90.0f;
    if (strncmp(classname, "monster_alien_slave", 19) == 0) return 30.0f;
    if (strncmp(classname, "monster_human_grunt", 19) == 0) return 80.0f;
    if (strncmp(classname, "monster_gargantua", 17) == 0)   return 800.0f;
    if (strncmp(classname, "monster_scientist", 17) == 0)   return 20.0f;
    if (strncmp(classname, "monster_barney", 14) == 0)      return 35.0f;
    if (strncmp(classname, "monster_gman", 12) == 0)        return 1000.0f;
    return 100.0f;
}

static bool is_monster(const char *cn) {
    return cn && strncmp(cn, "monster_", 8) == 0;
}
static bool is_weapon_pickup(const char *cn) {
    return cn && strncmp(cn, "weapon_", 7) == 0;
}
static bool is_item(const char *cn) {
    return cn && (strncmp(cn, "item_", 5) == 0 || strncmp(cn, "ammo_", 5) == 0);
}

/* ---------- Spawn from BSP ---------- */
u32 aether_entity_spawn_from_bsp_ex(aether_entity_mgr_t *mgr,
                                    const aether_bsp_t *bsp,
                                    aether_entity_spawn_stats_t *stats) {
    if (stats) memset(stats, 0, sizeof(*stats));
    if (!mgr || !bsp) return 0;

    bsp_entity_info_t *list = NULL;
    u32 count = parse_bsp_entities(bsp, &list);
    if (!list || count == 0) { free(list); return 0; }

    u32 spawned = 0;
    for (u32 i = 0; i < count; ++i) {
        bsp_entity_info_t *src = &list[i];
        if (src->classname[0] == 0) continue;

        /* Skip navigation / ambient helpers; keep lights for dynlight path. */
        if (strncmp(src->classname, "info_node", 9) == 0) continue;
        if (strncmp(src->classname, "path_", 5) == 0) continue;
        if (strncmp(src->classname, "env_", 4) == 0) continue;

        aether_entity_t *e = aether_entity_spawn(mgr, src->classname);
        if (!e) continue;

        e->origin = (aether_vec3_t){ src->origin[0], src->origin[1], src->origin[2] };
        e->angles = (aether_vec3_t){ src->angles[0], src->angles[1], src->angles[2] };

        if (src->targetname[0])
            aether_str_copy(e->targetname, AETHER_ENTITY_TARGETNAME_MAX, src->targetname);

        if (src->model[0])
            aether_entity_set_model(e, src->model);

        if (src->health > 0)
            aether_entity_set_health(e, src->health);
        else if (is_monster(src->classname))
            aether_entity_set_health(e, default_health_for(src->classname));

        if (is_monster(src->classname) || is_weapon_pickup(src->classname) || is_item(src->classname))
            e->flags |= AETHER_ENT_FLAG_SOLID;

        if (stats) {
            stats->total++;
            if (strcmp(src->classname, "worldspawn") == 0) stats->worldspawn++;
            else if (strncmp(src->classname, "info_player", 11) == 0) stats->player_starts++;
            else if (is_monster(src->classname)) stats->monsters++;
            else if (strncmp(src->classname, "light", 5) == 0) stats->lights++;
            else stats->other++;
        }
        spawned++;
    }

    free(list);
    aether_log(AETHER_LOG_INFO, "spawn",
               "spawned %u entities from BSP (lights included)", spawned);
    return spawned;
}

u32 aether_entity_spawn_from_bsp(aether_entity_mgr_t *mgr,
                                  const aether_bsp_t *bsp) {
    return aether_entity_spawn_from_bsp_ex(mgr, bsp, NULL);
}


/* ---------- Player start ---------- */
aether_result_t aether_entity_get_player_start(const aether_entity_mgr_t *mgr,
                                                aether_vec3_t *out_pos,
                                                aether_vec3_t *out_angles) {
    if (!mgr) return AETHER_ERR_INVALID_ARG;

    for (u32 i = 0; i < aether_entity_mgr_count(mgr); ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(mgr, i);
        if (!e) continue;
        if (strncmp(e->classname, "info_player_start", 17) == 0 ||
            strncmp(e->classname, "info_player_deathmatch", 22) == 0 ||
            strncmp(e->classname, "info_player_coop", 16) == 0) {
            if (out_pos) *out_pos = e->origin;
            if (out_angles) *out_angles = e->angles;
            return AETHER_OK;
        }
    }
    return AETHER_ERR_NOT_FOUND;
}

/* ---------- Diagnostics ---------- */
void aether_entity_spawn_dump(const aether_entity_mgr_t *mgr) {
    if (!mgr) return;
    u32 total = aether_entity_mgr_count(mgr);
    u32 monsters = 0, weapons = 0, items = 0, player_start = 0;
    u32 doors = 0, triggers = 0, other = 0;

    for (u32 i = 0; i < total; ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(mgr, i);
        if (!e) continue;
        if (is_monster(e->classname))                monsters++;
        else if (is_weapon_pickup(e->classname))     weapons++;
        else if (is_item(e->classname))              items++;
        else if (strncmp(e->classname, "info_player", 11) == 0) player_start++;
        else if (strncmp(e->classname, "func_door", 9) == 0)    doors++;
        else if (strncmp(e->classname, "trigger_", 8) == 0)     triggers++;
        else                                         other++;
    }

    aether_log(AETHER_LOG_INFO, "spawn", "===== ENTITY SPAWN SUMMARY =====");
    aether_log(AETHER_LOG_INFO, "spawn", "  total      : %u", total);
    aether_log(AETHER_LOG_INFO, "spawn", "  monsters   : %u", monsters);
    aether_log(AETHER_LOG_INFO, "spawn", "  weapons    : %u", weapons);
    aether_log(AETHER_LOG_INFO, "spawn", "  items/ammo : %u", items);
    aether_log(AETHER_LOG_INFO, "spawn", "  playerstart: %u", player_start);
    aether_log(AETHER_LOG_INFO, "spawn", "  doors      : %u", doors);
    aether_log(AETHER_LOG_INFO, "spawn", "  triggers   : %u", triggers);
    aether_log(AETHER_LOG_INFO, "spawn", "  other      : %u", other);
    aether_log(AETHER_LOG_INFO, "spawn", "================================");
}
