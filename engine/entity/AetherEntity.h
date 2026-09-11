/* AetherEntity.h — GoldSrc BSP ENTITIES parser (STEP 17A).
 * Entities are stored as a text key-value format inside a BSP lump.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_ENTITY_H
#define AETHER_ENTITY_H

#include "../core/AetherCore.h"
#include "../bsp/AetherBSP.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_ENTITY_CLASSNAME_MAX 64
#define AETHER_ENTITY_MAX           4096

typedef enum aether_entity_category {
    AETHER_ENTITY_UNKNOWN = 0,
    AETHER_ENTITY_PLAYER_START,
    AETHER_ENTITY_LIGHT,
    AETHER_ENTITY_MONSTER,
    AETHER_ENTITY_WEAPON,
    AETHER_ENTITY_ITEM,
    AETHER_ENTITY_TRIGGER,
    AETHER_ENTITY_DOOR,
    AETHER_ENTITY_FUNC,
    AETHER_ENTITY_CATEGORY_COUNT
} aether_entity_category_t;

typedef struct aether_entity {
    char                     classname[AETHER_ENTITY_CLASSNAME_MAX];
    f32                      origin[3];
    f32                      angles[3];
    aether_entity_category_t category;
} aether_entity_t;

typedef struct aether_entity_list aether_entity_list_t;

/* Build from BSP ENTITIES lump. Returns NULL if no entities. */
aether_entity_list_t *aether_entity_list_from_bsp(const aether_bsp_t *bsp);
void                  aether_entity_list_free(aether_entity_list_t *list);

u32                     aether_entity_list_count(const aether_entity_list_t *list);
const aether_entity_t  *aether_entity_at(const aether_entity_list_t *list, u32 idx);
u32                     aether_entity_category_count(const aether_entity_list_t *list,
                                                       aether_entity_category_t cat);

/* Find first entity of a category (e.g. player start). Returns NULL if none. */
const aether_entity_t  *aether_entity_find_first(const aether_entity_list_t *list,
                                                  aether_entity_category_t cat);

const char *aether_entity_category_name(aether_entity_category_t cat);
void        aether_entity_list_dump(const aether_entity_list_t *list);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_ENTITY_H */
