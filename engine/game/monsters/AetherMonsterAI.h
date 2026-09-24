/* AetherMonsterAI.h — Perception (sight + sound) and target selection.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MONSTER_AI_H
#define AETHER_MONSTER_AI_H

#include "AetherMonsterBase.h"
#include "AetherMonsterRegistry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Check if monster can see the target entity (FOV + range + LOS). */
bool aether_monster_ai_can_see(const aether_monster_t *m, const aether_entity_t *target);

/* Check if monster can hear a sound at given position + loudness. */
bool aether_monster_ai_can_hear(const aether_monster_t *m, aether_vec3_t sound_pos, f32 loudness);

/* Find best enemy: iterate entities, prefer visible + closest. */
aether_entity_t *aether_monster_ai_find_enemy(const aether_monster_t *m,
                                                aether_entity_t **candidates,
                                                u32 candidate_count);

/* Compute direction to face target (yaw only) */
f32 aether_monster_ai_face_yaw(const aether_monster_t *m, aether_vec3_t target_pos);

/* Per-frame AI: perception + target select for one monster (uses registry player). */
void aether_monster_ai_tick(aether_monster_t *m, aether_entity_t *player, f32 dt);

/* Hook registry: run AI tick for every alive synthetic monster. Returns ticks. */
u32 aether_monster_ai_tick_registry(struct aether_monster_registry *reg, f32 dt);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MONSTER_AI_H */
