/* AetherSaveEntity.h — Entity state serialization.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_SAVE_ENTITY_H
#define AETHER_SAVE_ENTITY_H

#include "AetherSaveFormat.h"
#include "../entity/AetherEntityBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Serialized entity state (compact, fixed layout). */
typedef struct aether_save_entity {
    u32   id;
    char  classname[AETHER_ENTITY_CLASSNAME_MAX];
    char  targetname[AETHER_ENTITY_TARGETNAME_MAX];

    f32   origin[3];
    f32   angles[3];
    f32   velocity[3];

    f32   health;
    f32   armor;

    u32   flags;
    i32   sequence;
    i32   body;
    i32   skin;

    char  model_name[AETHER_ENTITY_MODEL_MAX];
} aether_save_entity_t;

/* Pack a live entity → save buffer */
void aether_save_entity_pack(aether_save_entity_t *out, const aether_entity_t *e);
/* Unpack a save buffer → live entity (must already exist / be spawned) */
void aether_save_entity_unpack(const aether_save_entity_t *in, aether_entity_t *e);

/* Serialize all entities in the manager to a buffer.
 * Returns bytes written, or 0 on failure. */
u32  aether_save_entities_write(const aether_entity_mgr_t *mgr,
                                 u8 *out_buf, u32 buf_size,
                                 u32 *out_count);

/* Read entities from buffer and spawn into manager.
 * Returns number of entities spawned. */
u32  aether_save_entities_read(aether_entity_mgr_t *mgr,
                                const u8 *buf, u32 buf_size,
                                u32 count);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_SAVE_ENTITY_H */
