/* AetherMonsterRegistry.c — Monster registry implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMonsterRegistry.h"
#include <string.h>

void aether_monster_registry_init(aether_monster_registry_t *reg, aether_entity_t *player) {
    if (!reg) return;
    memset(reg, 0, sizeof *reg);
    reg->player = player;
    reg->time = 0.0f;
    aether_log(AETHER_LOG_INFO, "monreg", "monster registry initialized");
}

void aether_monster_registry_reset(aether_monster_registry_t *reg) {
    if (!reg) return;
    aether_entity_t *player = reg->player;
    memset(reg, 0, sizeof *reg);
    reg->player = player;
}

aether_monster_t *aether_monster_registry_spawn(aether_monster_registry_t *reg,
                                                  aether_monster_id_t id,
                                                  aether_vec3_t pos) {
    if (!reg || id <= 0 || id >= AETHER_MON_COUNT) return NULL;
    if (reg->count >= AETHER_MONSTER_MAX) {
        aether_log(AETHER_LOG_WARN, "monreg", "max monsters reached");
        return NULL;
    }

    aether_monster_t *m = &reg->monsters[reg->count];
    aether_monster_init(m, id);

    /* Create a backing entity */
    if (m->def && m->def->classname) {
        /* In real engine, this would go through entity spawn.
         * For now, allocate a lightweight entity-like wrapper. */
        aether_entity_t *e = (aether_entity_t*)calloc(1, sizeof *e);
        if (!e) return NULL;
        e->id = reg->count + 1000;
        aether_str_copy(e->classname, AETHER_ENTITY_CLASSNAME_MAX, m->def->classname);
        e->origin = pos;
        e->health = m->def->max_health;
        e->max_health = m->def->max_health;
        e->flags = AETHER_ENT_FLAG_ACTIVE | AETHER_ENT_FLAG_SOLID | AETHER_ENT_FLAG_VISIBLE;
        e->gravity = m->def->is_flying ? 0.0f : 800.0f;
        if (m->def->model_path)
            aether_entity_set_model(e, m->def->model_path);
        m->entity = e;
    }

    reg->count++;
    aether_log(AETHER_LOG_INFO, "monreg", "spawned %s at (%.0f,%.0f,%.0f)",
               m->def ? m->def->display_name : "?",
               pos.x, pos.y, pos.z);
    return m;
}

void aether_monster_registry_tick(aether_monster_registry_t *reg, f32 dt) {
    if (!reg) return;
    reg->time += dt;

    for (u32 i = 0; i < reg->count; ++i) {
        aether_monster_t *m = &reg->monsters[i];
        if (m->state == AETHER_MST_DEAD) continue;
        aether_monster_tick(m, dt);

        /* Simple enemy detection: if player in sight range, set as enemy */
        if (reg->player && !m->enemy && m->def) {
            f32 dist = aether_vec3_len(aether_vec3_sub(reg->player->origin, m->entity->origin));
            if (dist <= m->def->sight_range) {
                aether_monster_set_enemy(m, reg->player);
            }
        }
    }

    /* Compact dead monsters */
    u32 write = 0;
    for (u32 i = 0; i < reg->count; ++i) {
        if (reg->monsters[i].state == AETHER_MST_DEAD &&
            reg->monsters[i].state_time > 5.0f) {
            /* Free entity */
            free(reg->monsters[i].entity);
            continue;
        }
        if (write != i) reg->monsters[write] = reg->monsters[i];
        write++;
    }
    reg->count = write;
}

u32 aether_monster_registry_count(const aether_monster_registry_t *reg) {
    return reg ? reg->count : 0;
}

u32 aether_monster_registry_alive(const aether_monster_registry_t *reg) {
    if (!reg) return 0;
    u32 n = 0;
    for (u32 i = 0; i < reg->count; ++i)
        if (reg->monsters[i].state != AETHER_MST_DEAD) n++;
    return n;
}

void aether_monster_registry_dump(const aether_monster_registry_t *reg) {
    if (!reg) return;
    aether_log(AETHER_LOG_INFO, "monreg",
               "===== MONSTERS: %u total, %u alive =====",
               reg->count, aether_monster_registry_alive(reg));
    u32 shown = reg->count > 10 ? 10 : reg->count;
    for (u32 i = 0; i < shown; ++i) aether_monster_dump(&reg->monsters[i]);
    if (reg->count > shown)
        aether_log(AETHER_LOG_INFO, "monreg", "  ... +%u more", reg->count - shown);
}
