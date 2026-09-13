/* AetherSaveEntity.c — Entity state serialization implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherSaveEntity.h"
#include <string.h>

void aether_save_entity_pack(aether_save_entity_t *out, const aether_entity_t *e) {
    if (!out || !e) return;
    memset(out, 0, sizeof *out);

    out->id = e->id;
    aether_str_copy(out->classname, AETHER_ENTITY_CLASSNAME_MAX, e->classname);
    aether_str_copy(out->targetname, AETHER_ENTITY_TARGETNAME_MAX, e->targetname);

    out->origin[0] = e->origin.x; out->origin[1] = e->origin.y; out->origin[2] = e->origin.z;
    out->angles[0] = e->angles.x; out->angles[1] = e->angles.y; out->angles[2] = e->angles.z;
    out->velocity[0] = e->velocity.x; out->velocity[1] = e->velocity.y; out->velocity[2] = e->velocity.z;

    out->health = e->health;
    out->armor  = e->armor;
    out->flags  = e->flags;
    out->sequence = e->sequence;
    out->body     = e->body;
    out->skin     = e->skin;

    aether_str_copy(out->model_name, AETHER_ENTITY_MODEL_MAX, e->model_name);
}

void aether_save_entity_unpack(const aether_save_entity_t *in, aether_entity_t *e) {
    if (!in || !e) return;

    aether_str_copy(e->classname, AETHER_ENTITY_CLASSNAME_MAX, in->classname);
    aether_str_copy(e->targetname, AETHER_ENTITY_TARGETNAME_MAX, in->targetname);

    e->origin   = (aether_vec3_t){ in->origin[0], in->origin[1], in->origin[2] };
    e->angles   = (aether_vec3_t){ in->angles[0], in->angles[1], in->angles[2] };
    e->velocity = (aether_vec3_t){ in->velocity[0], in->velocity[1], in->velocity[2] };

    e->health   = in->health;
    e->max_health = in->health > 100.0f ? in->health : 100.0f;
    e->armor    = in->armor;
    e->flags    = in->flags;
    e->sequence = in->sequence;
    e->body     = in->body;
    e->skin     = in->skin;

    if (in->model_name[0])
        aether_entity_set_model(e, in->model_name);
}

u32 aether_save_entities_write(const aether_entity_mgr_t *mgr,
                                u8 *out_buf, u32 buf_size, u32 *out_count) {
    if (!mgr || !out_buf || buf_size == 0) return 0;

    u32 count = aether_entity_mgr_count(mgr);
    if (out_count) *out_count = count;

    u32 per = sizeof(aether_save_entity_t);
    if (count * per > buf_size) {
        aether_log(AETHER_LOG_WARN, "save-ent", "buffer too small (%u need %u)",
                   buf_size, count * per);
        return 0;
    }

    u32 written = 0;
    for (u32 i = 0; i < count; ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(mgr, i);
        if (!e) continue;
        if (e->flags & AETHER_ENT_FLAG_PENDING_KILL) continue;
        aether_save_entity_t packed;
        aether_save_entity_pack(&packed, e);
        memcpy(out_buf + written, &packed, per);
        written += per;
    }
    if (out_count) *out_count = written / per;
    aether_log(AETHER_LOG_INFO, "save-ent", "wrote %u entities (%u bytes)",
               written / per, written);
    return written;
}

u32 aether_save_entities_read(aether_entity_mgr_t *mgr,
                                const u8 *buf, u32 buf_size, u32 count) {
    if (!mgr || !buf || buf_size == 0 || count == 0) return 0;

    u32 per = sizeof(aether_save_entity_t);
    if (count * per > buf_size) {
        aether_log(AETHER_LOG_WARN, "save-ent", "read: buffer too small");
        return 0;
    }

    u32 spawned = 0;
    for (u32 i = 0; i < count; ++i) {
        aether_save_entity_t packed;
        memcpy(&packed, buf + i * per, per);
        if (packed.classname[0] == 0) continue;

        aether_entity_t *e = aether_entity_spawn(mgr, packed.classname);
        if (!e) continue;
        aether_save_entity_unpack(&packed, e);
        spawned++;
    }
    aether_log(AETHER_LOG_INFO, "save-ent", "read %u entities", spawned);
    return spawned;
}
