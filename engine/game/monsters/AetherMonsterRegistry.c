/* AetherMonsterRegistry.c — Monster registry implementation.
 * AetherEngine-iOS · Clean-room.
 *
 * NOTE: All headers explicitly included for compile safety.
 */
#include "AetherMonsterRegistry.h"
#include "AetherMonsterAI.h"
#include "AetherMonsterBase.h"
#include "AetherMonsterTypes.h"
#include "AetherMonsterDefs.h"
#include "../../entity/AetherEntityBase.h"
#include "../../core/AetherCore.h"
#include "../../core/AetherMath.h"
#include <stdlib.h>
#include <string.h>

/* Forward decl (from AetherMonsterBase.c) */
extern const aether_monster_def_t *aether_monster_defs_lookup(aether_monster_id_t id);

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

    if (m->def && m->def->classname) {
        aether_entity_t *e = (aether_entity_t*)calloc(1, sizeof(aether_entity_t));
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

    /* Perception / target select before movement states. */
    (void)aether_monster_ai_tick_registry(reg, dt);

    for (u32 i = 0; i < reg->count; ++i) {
        aether_monster_t *m = &reg->monsters[i];
        if (m->state == AETHER_MST_DEAD) continue;
        aether_monster_tick(m, dt);
    }

    /* Compact dead monsters */
    u32 write = 0;
    for (u32 i = 0; i < reg->count; ++i) {
        if (reg->monsters[i].state == AETHER_MST_DEAD &&
            reg->monsters[i].state_time > 5.0f) {
            free(reg->monsters[i].entity);
            reg->monsters[i].entity = NULL;
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
