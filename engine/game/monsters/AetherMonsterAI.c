/* AetherMonsterAI.c — AI perception + target selection.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMonsterAI.h"
#include <math.h>

/* Simple LOS check: assume clear if within range (real engine uses BSP trace) */
static bool los_clear(aether_vec3_t from, aether_vec3_t to) {
    (void)from; (void)to;
    return true;  /* TODO: BSP line trace */
}

bool aether_monster_ai_can_see(const aether_monster_t *m, const aether_entity_t *target) {
    if (!m || !m->def || !m->entity || !target) return false;
    if (m->state == AETHER_MST_DEAD) return false;

    /* Range check */
    aether_vec3_t diff = aether_vec3_sub(target->origin, m->entity->origin);
    f32 dist = aether_vec3_len(diff);
    if (dist > m->def->sight_range) return false;

    /* FOV check */
    aether_vec3_t forward = {
        cosf(m->entity->angles.y * 3.14159f / 180.0f),
        sinf(m->entity->angles.y * 3.14159f / 180.0f),
        0.0f
    };
    aether_vec3_t to_target = aether_vec3_normalize(diff);
    f32 dot = forward.x * to_target.x + forward.y * to_target.y;
    if (dot < cosf(m->def->field_of_view)) return false;

    /* LOS check */
    return los_clear(m->entity->origin, target->origin);
}

bool aether_monster_ai_can_hear(const aether_monster_t *m, aether_vec3_t sound_pos, f32 loudness) {
    if (!m || !m->def || !m->entity) return false;
    f32 dist = aether_vec3_len(aether_vec3_sub(sound_pos, m->entity->origin));
    return dist <= m->def->hearing_range * loudness;
}

aether_entity_t *aether_monster_ai_find_enemy(const aether_monster_t *m,
                                                aether_entity_t **candidates,
                                                u32 candidate_count) {
    if (!m || !candidates) return NULL;

    aether_entity_t *best = NULL;
    f32 best_dist = 1e30f;

    for (u32 i = 0; i < candidate_count; ++i) {
        aether_entity_t *cand = candidates[i];
        if (!cand) continue;
        if (!aether_monster_ai_can_see(m, cand)) continue;

        f32 dist = aether_vec3_len(aether_vec3_sub(cand->origin, m->entity->origin));
        if (dist < best_dist) {
            best_dist = dist;
            best = cand;
        }
    }
    return best;
}

f32 aether_monster_ai_face_yaw(const aether_monster_t *m, aether_vec3_t target_pos) {
    if (!m || !m->entity) return 0.0f;
    aether_vec3_t d = aether_vec3_sub(target_pos, m->entity->origin);
    return atan2f(d.y, d.x) * 180.0f / 3.14159f;
}
