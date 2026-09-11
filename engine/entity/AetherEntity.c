/* AetherEntity.c — BSP ENTITIES lump parser (STEP 17A).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherEntity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct aether_entity_list {
    aether_entity_t *items;
    u32              count;
    u32              capacity;
};

/* ---------- Helpers ---------- */
static void parse_3floats(const char *s, f32 out[3]) {
    out[0] = out[1] = out[2] = 0.0f;
    if (!s) return;
    sscanf(s, "%f %f %f", &out[0], &out[1], &out[2]);
}

static aether_entity_category_t categorize(const char *cls) {
    if (!cls) return AETHER_ENTITY_UNKNOWN;
    if (strncmp(cls, "info_player", 11) == 0) return AETHER_ENTITY_PLAYER_START;
    if (strcmp(cls, "light") == 0)            return AETHER_ENTITY_LIGHT;
    if (strncmp(cls, "light_", 6) == 0)       return AETHER_ENTITY_LIGHT;
    if (strncmp(cls, "monster_", 8) == 0)     return AETHER_ENTITY_MONSTER;
    if (strncmp(cls, "weapon_", 7) == 0)      return AETHER_ENTITY_WEAPON;
    if (strncmp(cls, "item_", 5) == 0)        return AETHER_ENTITY_ITEM;
    if (strncmp(cls, "ammo_", 5) == 0)        return AETHER_ENTITY_ITEM;
    if (strncmp(cls, "trigger_", 8) == 0)     return AETHER_ENTITY_TRIGGER;
    if (strcmp(cls, "func_door") == 0)        return AETHER_ENTITY_DOOR;
    if (strncmp(cls, "func_door_", 10) == 0)  return AETHER_ENTITY_DOOR;
    if (strcmp(cls, "func_button") == 0)      return AETHER_ENTITY_DOOR;
    if (strncmp(cls, "func_", 5) == 0)        return AETHER_ENTITY_FUNC;
    if (strncmp(cls, "env_", 4) == 0)         return AETHER_ENTITY_FUNC;
    if (strncmp(cls, "path_", 5) == 0)        return AETHER_ENTITY_FUNC;
    return AETHER_ENTITY_UNKNOWN;
}

const char *aether_entity_category_name(aether_entity_category_t cat) {
    switch (cat) {
        case AETHER_ENTITY_PLAYER_START: return "PLAYER_START";
        case AETHER_ENTITY_LIGHT:        return "LIGHT";
        case AETHER_ENTITY_MONSTER:      return "MONSTER";
        case AETHER_ENTITY_WEAPON:       return "WEAPON";
        case AETHER_ENTITY_ITEM:         return "ITEM";
        case AETHER_ENTITY_TRIGGER:      return "TRIGGER";
        case AETHER_ENTITY_DOOR:         return "DOOR";
        case AETHER_ENTITY_FUNC:         return "FUNC";
        default:                         return "UNKNOWN";
    }
}

/* Add entity to list (dynamic array) */
static aether_entity_t *list_add(aether_entity_list_t *list) {
    if (!list) return NULL;
    if (list->count >= AETHER_ENTITY_MAX) return NULL;
    if (list->count >= list->capacity) {
        u32 newcap = list->capacity ? list->capacity * 2 : 32;
        if (newcap > AETHER_ENTITY_MAX) newcap = AETHER_ENTITY_MAX;
        aether_entity_t *n = (aether_entity_t*)realloc(list->items,
                                                        newcap * sizeof(aether_entity_t));
        if (!n) return NULL;
        list->items = n;
        list->capacity = newcap;
    }
    aether_entity_t *e = &list->items[list->count];
    memset(e, 0, sizeof *e);
    e->category = AETHER_ENTITY_UNKNOWN;
    list->count++;
    return e;
}

/* ---------- Parser ---------- */
/* Entities lump is a sequence of:
 *   {
 *   "key" "value"
 *   "key" "value"
 *   }
 *   {
 *   ...
 *   }
 * We keep it simple: read line by line, when we see { start a new entity,
 * when we see } finalize it. Lines starting with " are key-value pairs.
 */
aether_entity_list_t *aether_entity_list_from_bsp(const aether_bsp_t *bsp) {
    if (!bsp) return NULL;
    size_t size = 0;
    const u8 *raw = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_ENTITIES);
    size = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_ENTITIES);
    if (!raw || size == 0) {
        aether_log(AETHER_LOG_WARN, "entity", "no ENTITIES lump");
        return NULL;
    }

    aether_entity_list_t *list = (aether_entity_list_t*)calloc(1, sizeof *list);
    if (!list) return NULL;

    /* Work on a NUL-terminated copy */
    char *buf = (char*)malloc(size + 1);
    if (!buf) { free(list); return NULL; }
    memcpy(buf, raw, size);
    buf[size] = 0;

    char *p = buf;
    aether_entity_t *cur = NULL;

    while (*p) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;

        if (*p == '{') {
            /* New entity begins */
            cur = list_add(list);
            p++;
            continue;
        }
        if (*p == '}') {
            /* Finalize current entity */
            if (cur) {
                cur->category = categorize(cur->classname);
            }
            cur = NULL;
            p++;
            continue;
        }
        if (*p == '"' && cur) {
            /* Parse "key" "value" pair */
            p++; /* skip opening quote */
            char key[64] = {0};
            int ki = 0;
            while (*p && *p != '"' && ki < 63) key[ki++] = *p++;
            key[ki] = 0;
            if (*p == '"') p++; /* skip closing quote */

            /* Skip whitespace */
            while (*p == ' ' || *p == '\t') p++;

            if (*p == '"') {
                p++;
                char val[128] = {0};
                int vi = 0;
                while (*p && *p != '"' && vi < 127) val[vi++] = *p++;
                val[vi] = 0;
                if (*p == '"') p++;

                if (strcmp(key, "classname") == 0) {
                    aether_str_copy(cur->classname, AETHER_ENTITY_CLASSNAME_MAX, val);
                } else if (strcmp(key, "origin") == 0) {
                    parse_3floats(val, cur->origin);
                } else if (strcmp(key, "angles") == 0) {
                    parse_3floats(val, cur->angles);
                }
            }
            continue;
        }
        /* Unknown character, skip */
        p++;
    }

    free(buf);
    aether_log(AETHER_LOG_INFO, "entity", "parsed %u entities from BSP", list->count);
    return list;
}

void aether_entity_list_free(aether_entity_list_t *list) {
    if (!list) return;
    free(list->items);
    free(list);
}

u32 aether_entity_list_count(const aether_entity_list_t *list) {
    return list ? list->count : 0;
}

const aether_entity_t *aether_entity_at(const aether_entity_list_t *list, u32 idx) {
    if (!list || idx >= list->count) return NULL;
    return &list->items[idx];
}

u32 aether_entity_category_count(const aether_entity_list_t *list,
                                  aether_entity_category_t cat) {
    if (!list) return 0;
    u32 n = 0;
    for (u32 i = 0; i < list->count; ++i) {
        if (list->items[i].category == cat) n++;
    }
    return n;
}

const aether_entity_t *aether_entity_find_first(const aether_entity_list_t *list,
                                                  aether_entity_category_t cat) {
    if (!list) return NULL;
    for (u32 i = 0; i < list->count; ++i) {
        if (list->items[i].category == cat) return &list->items[i];
    }
    return NULL;
}

void aether_entity_list_dump(const aether_entity_list_t *list) {
    if (!list) { aether_log(AETHER_LOG_WARN, "entity", "dump: null"); return; }
    aether_log(AETHER_LOG_INFO, "entity", "=====================================");
    aether_log(AETHER_LOG_INFO, "entity", "  total entities: %u", list->count);

    /* Category summary */
    for (u32 c = 0; c < AETHER_ENTITY_CATEGORY_COUNT; ++c) {
        u32 n = aether_entity_category_count(list, (aether_entity_category_t)c);
        if (n > 0) {
            aether_log(AETHER_LOG_INFO, "entity", "  %-14s : %u",
                       aether_entity_category_name((aether_entity_category_t)c), n);
        }
    }

    /* First 15 entities */
    u32 shown = list->count > 15 ? 15 : list->count;
    aether_log(AETHER_LOG_INFO, "entity", "  --- first %u ---", shown);
    for (u32 i = 0; i < shown; ++i) {
        const aether_entity_t *e = &list->items[i];
        aether_log(AETHER_LOG_INFO, "entity", "  [%u] %-24s (%.0f,%.0f,%.0f)",
                   i, e->classname, e->origin[0], e->origin[1], e->origin[2]);
    }
    if (list->count > shown) {
        aether_log(AETHER_LOG_INFO, "entity", "  … +%u more", list->count - shown);
    }
    aether_log(AETHER_LOG_INFO, "entity", "=====================================");
}
