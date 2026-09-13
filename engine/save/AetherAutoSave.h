/* AetherAutoSave.h — Auto-save triggers (checkpoints, level transitions).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_AUTOSAVE_H
#define AETHER_AUTOSAVE_H

#include "AetherSave.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_AUTOSAVE_DIR_MAX   256
#define AETHER_AUTOSAVE_MIN_INTERVAL 60.0f   /* seconds between auto-saves */

typedef enum aether_autosave_trigger {
    AETHER_AUTOSAVE_TRIGGER_CHECKPOINT = 0,
    AETHER_AUTOSAVE_TRIGGER_LEVEL_START,
    AETHER_AUTOSAVE_TRIGGER_LEVEL_END,
    AETHER_AUTOSAVE_TRIGGER_PLAYER_DEATH,
    AETHER_AUTOSAVE_TRIGGER_MANUAL,
} aether_autosave_trigger_t;

typedef struct aether_autosave {
    char   dir[AETHER_AUTOSAVE_DIR_MAX];
    f32    last_save_time;
    u32    save_count;
    bool   enabled;
} aether_autosave_t;

void aether_autosave_init(aether_autosave_t *a, const char *dir);
void aether_autosave_enable(aether_autosave_t *a, bool enabled);

/* Attempt an auto-save with a trigger reason.
 * Enforces min interval between saves (except for manual / level transitions). */
aether_result_t aether_autosave_trigger(aether_autosave_t *a,
                                         aether_autosave_trigger_t reason,
                                         const aether_save_ctx_t *ctx,
                                         f32 now);

/* Force save regardless of interval. */
aether_result_t aether_autosave_force(aether_autosave_t *a,
                                       const char *slot_name,
                                       const aether_save_ctx_t *ctx);

void aether_autosave_dump(const aether_autosave_t *a);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_AUTOSAVE_H */
