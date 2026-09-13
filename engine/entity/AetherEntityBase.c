/* AetherEntityBase.c — Runtime entity system (STEP 18A).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherEntityBase.h"
#include <stdlib.h>
#include <string.h>

struct aether_entity_mgr {
    aether_entity_t *entities;
    u32              count;
    u32              capacity;
    u32              next_id;
    f32              time;
};

aether_entity_mgr_t *aether_entity_mgr_create(void) {
    aether_entity_mgr_t *mgr = (aether_entity_mgr_t*)calloc(1, sizeof *mgr);
    if (!mgr) return NULL;
    mgr->capacity = AETHER_ENTITY_MAX;
    mgr->entities = (aether_entity_t*)calloc(mgr->capacity, sizeof(aether_entity_t));
    if (!mgr->entities) { free(mgr); return NULL; }
    mgr->count = 0; mgr->next_id = 1; mgr->time = 0.0f;
    aether_log(AETHER_LOG_INFO, "entity", "manager created (cap=%u)", mgr->capacity);
    return mgr;
}

void aether_entity_mgr_destroy(aether_entity_mgr_t *mgr) {
    if (!mgr) return;
    free(mgr->entities);
    free(mgr);
}

aether_entity_t *aether_entity_spawn(aether_entity_mgr_t *mgr, const char *classname) {
    if (!mgr || !classname) return NULL;
    if (mgr->count >= mgr->capacity) return NULL;

    aether_entity_t *e = &mgr->entities[mgr->count];
    memset(e, 0, sizeof *e);

    e->id = mgr->next_id++;
    aether_str_copy(e->classname, AETHER_ENTITY_CLASSNAME_MAX, classname);

    e->mins = (aether_vec3_t){ -16, -16, -16 };
    e->maxs = (aether_vec3_t){  16,  16,  16 };

    e->health = 100.0f;
    e->max_health = 100.0f;
    e->gravity = 800.0f;
    e->friction = 4.0f;
    e->flags = AETHER_ENT_FLAG_ACTIVE | AETHER_ENT_FLAG_VISIBLE;

    mgr->count++;
    if (e->spawn) e->spawn(e);
    return e;
}

aether_entity_t *aether_entity_find_by_class(aether_entity_mgr_t *mgr, const char *classname) {
    if (!mgr || !classname) return NULL;
    for (u32 i = 0; i < mgr->count; ++i) {
        aether_entity_t *e = &mgr->entities[i];
        if (!(e->flags & AETHER_ENT_FLAG_ACTIVE)) continue;
        if (aether_str_eq(e->classname, classname)) return e;
    }
    return NULL;
}

aether_entity_t *aether_entity_find_by_name(aether_entity_mgr_t *mgr, const char *targetname) {
    if (!mgr || !targetname) return NULL;
    for (u32 i = 0; i < mgr->count; ++i) {
        aether_entity_t *e = &mgr->entities[i];
        if (!(e->flags & AETHER_ENT_FLAG_ACTIVE)) continue;
        if (aether_str_eq(e->targetname, targetname)) return e;
    }
    return NULL;
}

void aether_entity_remove(aether_entity_t *e) {
    if (!e) return;
    e->flags |= AETHER_ENT_FLAG_PENDING_KILL;
    e->flags &= ~AETHER_ENT_FLAG_ACTIVE;
}

void aether_entity_mgr_tick(aether_entity_mgr_t *mgr, f32 dt) {
    if (!mgr) return;
    mgr->time += dt;

    for (u32 i = 0; i < mgr->count; ++i) {
        aether_entity_t *e = &mgr->entities[i];
        if (!(e->flags & AETHER_ENT_FLAG_ACTIVE)) continue;

        if (e->think && mgr->time >= e->next_think) e->think(e);

        if (!(e->flags & AETHER_ENT_FLAG_ON_GROUND))
            aether_entity_apply_gravity(e, dt);

        e->origin.x += e->velocity.x * dt;
        e->origin.y += e->velocity.y * dt;
        e->origin.z += e->velocity.z * dt;

        e->angles.x += e->avelocity.x * dt;
        e->angles.y += e->avelocity.y * dt;
        e->angles.z += e->avelocity.z * dt;

        if (e->angles.x >  89.0f) e->angles.x =  89.0f;
        if (e->angles.x < -89.0f) e->angles.x = -89.0f;
    }

    /* Deferred cleanup */
    u32 write = 0;
    for (u32 i = 0; i < mgr->count; ++i) {
        aether_entity_t *e = &mgr->entities[i];
        if (e->flags & AETHER_ENT_FLAG_PENDING_KILL) {
            if (e->on_destroy) e->on_destroy(e);
            continue;
        }
        if (write != i) mgr->entities[write] = mgr->entities[i];
        write++;
    }
    mgr->count = write;
}

u32 aether_entity_mgr_count(const aether_entity_mgr_t *mgr) { return mgr ? mgr->count : 0; }

const aether_entity_t *aether_entity_mgr_at(const aether_entity_mgr_t *mgr, u32 idx) {
    if (!mgr || idx >= mgr->count) return NULL;
    return &mgr->entities[idx];
}

u32 aether_entity_mgr_active_count(const aether_entity_mgr_t *mgr) {
    if (!mgr) return 0;
    u32 n = 0;
    for (u32 i = 0; i < mgr->count; ++i)
        if (mgr->entities[i].flags & AETHER_ENT_FLAG_ACTIVE) n++;
    return n;
}

void aether_entity_set_origin(aether_entity_t *e, aether_vec3_t origin) { if (e) e->origin = origin; }
void aether_entity_set_angles(aether_entity_t *e, aether_vec3_t angles) { if (e) e->angles = angles; }

void aether_entity_set_model(aether_entity_t *e, const char *model) {
    if (!e || !model) return;
    aether_str_copy(e->model_name, AETHER_ENTITY_MODEL_MAX, model);
    size_t len = strlen(model);
    if (len > 4) {
        const char *ext = model + len - 4;
        if (aether_str_eq(ext, ".mdl")) e->model_type = AETHER_MODEL_STUDIO;
        else if (aether_str_eq(ext, ".spr")) e->model_type = AETHER_MODEL_SPRITE;
        else if (ext[0] == '*' || aether_str_eq(ext, ".bsp")) e->model_type = AETHER_MODEL_BRUSH;
    }
}

void aether_entity_set_health(aether_entity_t *e, f32 hp) {
    if (!e) return;
    e->health = hp;
    if (e->max_health < hp) e->max_health = hp;
}

void aether_entity_apply_damage(aether_entity_t *e, f32 dmg, aether_entity_t *attacker) {
    if (!e || dmg <= 0) return;
    if (e->take_damage) { e->take_damage(e, attacker, dmg); return; }

    e->damage_taken += dmg;
    if (e->armor > 0.0f) {
        f32 absorbed = dmg * 0.5f;
        if (absorbed > e->armor) absorbed = e->armor;
        e->armor -= absorbed;
        dmg -= absorbed;
    }
    e->health -= dmg;
    if (e->health <= 0.0f) {
        e->health = 0.0f;
        aether_entity_remove(e);
    }
}

void aether_entity_apply_gravity(aether_entity_t *e, f32 dt) {
    if (!e) return;
    e->velocity.z -= e->gravity * dt;
    if (e->velocity.z < -2000.0f) e->velocity.z = -2000.0f;
}

void aether_entity_set_next_think(aether_entity_t *e, f32 delay) {
    if (!e) return;
    e->next_think = e->anim_time + delay;
}

void aether_entity_dump(const aether_entity_t *e) {
    if (!e) return;
    aether_log(AETHER_LOG_INFO, "entity", "  #%u %-24s (%.0f,%.0f,%.0f) hp=%.0f",
               e->id, e->classname, e->origin.x, e->origin.y, e->origin.z, e->health);
}

void aether_entity_mgr_dump(const aether_entity_mgr_t *mgr) {
    if (!mgr) return;
    aether_log(AETHER_LOG_INFO, "entity", "===== ENTITIES: %u total, %u active =====",
               mgr->count, aether_entity_mgr_active_count(mgr));
    u32 shown = mgr->count > 20 ? 20 : mgr->count;
    for (u32 i = 0; i < shown; ++i) aether_entity_dump(&mgr->entities[i]);
    if (mgr->count > shown)
        aether_log(AETHER_LOG_INFO, "entity", "  ... +%u more", mgr->count - shown);
}
