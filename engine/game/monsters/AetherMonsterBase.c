/* AetherMonsterBase.c — Base monster implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMonsterBase.h"
#include "AetherMonsterDefs.h"
#include <string.h>
#include <math.h>

/* Forward decl (implemented in AetherMonsterDefs.c) */
const aether_monster_def_t *aether_monster_defs_lookup(aether_monster_id_t id);

void aether_monster_init(aether_monster_t *m, aether_monster_id_t id) {
    if (!m) return;
    memset(m, 0, sizeof *m);
    m->id = id;
    m->def = aether_monster_defs_lookup(id);
    m->state = AETHER_MST_IDLE;
    m->state_time = 0.0f;
    m->next_attack = 0.0f;
    m->next_think = 0.0f;
    m->lost_enemy_time = 0.0f;
    m->sequence = 0;
    m->anim_time = 0.0f;
    aether_log(AETHER_LOG_INFO, "monster", "initialized: %s",
               m->def ? m->def->display_name : "?");
}

void aether_monster_link_entity(aether_monster_t *m, aether_entity_t *e) {
    if (!m || !e) return;
    m->entity = e;
    if (m->def)
        aether_entity_set_health(e, m->def->max_health);
}

void aether_monster_set_enemy(aether_monster_t *m, aether_entity_t *enemy) {
    if (!m) return;
    if (m->enemy != enemy) {
        m->enemy = enemy;
        m->lost_enemy_time = 0.0f;
        if (enemy) {
            m->last_known_enemy_pos = enemy->origin;
            if (m->state == AETHER_MST_IDLE || m->state == AETHER_MST_ALERT)
                aether_monster_set_state(m, AETHER_MST_CHASE);
        }
    }
}

void aether_monster_set_state(aether_monster_t *m, aether_monster_state_t state) {
    if (!m || m->state == state) return;
    aether_log(AETHER_LOG_DEBUG, "monster", "%s: %s -> %s",
               m->def ? m->def->display_name : "?",
               aether_monster_state_name(m->state),
               aether_monster_state_name(state));
    m->state = state;
    m->state_time = 0.0f;
}

const char *aether_monster_state_name(aether_monster_state_t state) {
    switch (state) {
        case AETHER_MST_IDLE:    return "IDLE";
        case AETHER_MST_ALERT:   return "ALERT";
        case AETHER_MST_CHASE:   return "CHASE";
        case AETHER_MST_ATTACK:  return "ATTACK";
        case AETHER_MST_RETREAT: return "RETREAT";
        case AETHER_MST_DEAD:    return "DEAD";
        default:                 return "?";
    }
}

void aether_monster_tick(aether_monster_t *m, f32 dt) {
    if (!m || !m->entity || !m->def) return;
    if (m->state == AETHER_MST_DEAD) return;

    m->state_time += dt;
    m->anim_time += dt;

    switch (m->state) {
        case AETHER_MST_IDLE:
            /* Wait for enemy detection - handled by awareness module */
            break;

        case AETHER_MST_ALERT:
            /* Turn toward last known enemy pos */
            if (m->state_time > 1.5f)
                aether_monster_set_state(m, AETHER_MST_CHASE);
            break;

        case AETHER_MST_CHASE: {
            if (!m->enemy) {
                aether_monster_set_state(m, AETHER_MST_IDLE);
                break;
            }
            f32 dist = aether_vec3_len(aether_vec3_sub(m->enemy->origin, m->entity->origin));
            if (dist <= m->def->attack_range) {
                aether_monster_set_state(m, AETHER_MST_ATTACK);
            } else {
                /* Move toward enemy (simplified: direct vector) */
                aether_vec3_t dir = aether_vec3_normalize(
                    aether_vec3_sub(m->enemy->origin, m->entity->origin));
                f32 speed = m->def->run_speed;
                m->entity->velocity.x = dir.x * speed;
                m->entity->velocity.y = dir.y * speed;
                m->last_known_enemy_pos = m->enemy->origin;
            }
            break;
        }

        case AETHER_MST_ATTACK: {
            /* Stop moving */
            m->entity->velocity.x = 0.0f;
            m->entity->velocity.y = 0.0f;

            if (!m->enemy) {
                aether_monster_set_state(m, AETHER_MST_IDLE);
                break;
            }
            f32 dist = aether_vec3_len(aether_vec3_sub(m->enemy->origin, m->entity->origin));
            if (dist > m->def->attack_range * 1.3f)
                aether_monster_set_state(m, AETHER_MST_CHASE);
            break;
        }

        case AETHER_MST_RETREAT:
            if (m->state_time > 2.0f)
                aether_monster_set_state(m, AETHER_MST_CHASE);
            break;

        default: break;
    }
}

bool aether_monster_can_attack(const aether_monster_t *m, f32 now) {
    if (!m || !m->def) return false;
    if (m->state != AETHER_MST_ATTACK) return false;
    if (now < m->next_attack) return false;
    if (!m->enemy) return false;
    f32 dist = aether_vec3_len(aether_vec3_sub(m->enemy->origin, m->entity->origin));
    return dist <= m->def->attack_range;
}

bool aether_monster_do_attack(aether_monster_t *m, f32 now) {
    if (!aether_monster_can_attack(m, now)) return false;
    m->next_attack = now + m->def->attack_cooldown;
    m->attacking = true;
    aether_log(AETHER_LOG_INFO, "monster",
               "%s attacked (dmg=%.0f, type=%d)",
               m->def->display_name, m->def->attack_damage, (int)m->def->attack_type);
    /* Actual damage application done by combat module */
    return true;
}

void aether_monster_take_damage(aether_monster_t *m, f32 damage, aether_entity_t *attacker) {
    if (!m || !m->entity) return;
    if (m->state == AETHER_MST_DEAD) return;

    aether_entity_apply_damage(m->entity, damage, attacker);
    if (m->entity->health <= 0.0f) {
        aether_monster_die(m);
    } else if (attacker && !m->enemy) {
        aether_monster_set_enemy(m, attacker);
    }
}

void aether_monster_die(aether_monster_t *m) {
    if (!m || m->state == AETHER_MST_DEAD) return;
    aether_monster_set_state(m, AETHER_MST_DEAD);
    m->dying = true;
    m->entity->velocity.x = 0.0f;
    m->entity->velocity.y = 0.0f;
    aether_log(AETHER_LOG_INFO, "monster",
               "%s died", m->def ? m->def->display_name : "?");
}

void aether_monster_dump(const aether_monster_t *m) {
    if (!m) return;
    aether_log(AETHER_LOG_INFO, "monster",
               "  %-20s state=%-8s hp=%.0f enemy=%s",
               m->def ? m->def->display_name : "?",
               aether_monster_state_name(m->state),
               m->entity ? m->entity->health : 0.0f,
               m->enemy ? "yes" : "no");
}
