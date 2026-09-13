/* AetherAutoSave.c — Auto-save implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherAutoSave.h"
#include <stdio.h>
#include <string.h>

void aether_autosave_init(aether_autosave_t *a, const char *dir) {
    if (!a) return;
    memset(a, 0, sizeof *a);
    a->enabled = true;
    if (dir) aether_str_copy(a->dir, AETHER_AUTOSAVE_DIR_MAX, dir);
    aether_log(AETHER_LOG_INFO, "autosave", "initialized (dir=%s)", a->dir);
}

void aether_autosave_enable(aether_autosave_t *a, bool enabled) {
    if (!a) return;
    a->enabled = enabled;
    aether_log(AETHER_LOG_INFO, "autosave", "%s", enabled ? "enabled" : "disabled");
}

static const char *trigger_name(aether_autosave_trigger_t r) {
    switch (r) {
        case AETHER_AUTOSAVE_TRIGGER_CHECKPOINT:  return "checkpoint";
        case AETHER_AUTOSAVE_TRIGGER_LEVEL_START: return "level_start";
        case AETHER_AUTOSAVE_TRIGGER_LEVEL_END:   return "level_end";
        case AETHER_AUTOSAVE_TRIGGER_PLAYER_DEATH:return "player_death";
        case AETHER_AUTOSAVE_TRIGGER_MANUAL:      return "manual";
        default:                                  return "unknown";
    }
}

aether_result_t aether_autosave_trigger(aether_autosave_t *a,
                                         aether_autosave_trigger_t reason,
                                         const aether_save_ctx_t *ctx,
                                         f32 now) {
    if (!a || !ctx) return AETHER_ERR_INVALID_ARG;
    if (!a->enabled) return AETHER_ERR_NOT_READY;

    /* Enforce min interval for non-manual, non-level triggers */
    bool bypass_interval = (reason == AETHER_AUTOSAVE_TRIGGER_MANUAL ||
                            reason == AETHER_AUTOSAVE_TRIGGER_LEVEL_START ||
                            reason == AETHER_AUTOSAVE_TRIGGER_LEVEL_END);
    if (!bypass_interval && (now - a->last_save_time) < AETHER_AUTOSAVE_MIN_INTERVAL) {
        aether_log(AETHER_LOG_DEBUG, "autosave",
                   "skipped (%s) — too soon (%.1fs)", trigger_name(reason),
                   now - a->last_save_time);
        return AETHER_ERR_ALREADY;
    }

    char filepath[512];
    snprintf(filepath, sizeof filepath, "%s/autosave_%s.sav",
             a->dir, trigger_name(reason));

    aether_result_t r = aether_save_write(filepath, ctx);
    if (r == AETHER_OK) {
        a->last_save_time = now;
        a->save_count++;
        aether_log(AETHER_LOG_INFO, "autosave",
                   "saved (%s) → %s (total=%u)",
                   trigger_name(reason), filepath, a->save_count);
    }
    return r;
}

aether_result_t aether_autosave_force(aether_autosave_t *a,
                                       const char *slot_name,
                                       const aether_save_ctx_t *ctx) {
    if (!a || !ctx || !slot_name) return AETHER_ERR_INVALID_ARG;

    char filepath[512];
    snprintf(filepath, sizeof filepath, "%s/%s.sav", a->dir, slot_name);

    aether_result_t r = aether_save_write(filepath, ctx);
    if (r == AETHER_OK) {
        a->save_count++;
        aether_log(AETHER_LOG_INFO, "autosave",
                   "manual save → %s (total=%u)", filepath, a->save_count);
    }
    return r;
}

void aether_autosave_dump(const aether_autosave_t *a) {
    if (!a) return;
    aether_log(AETHER_LOG_INFO, "autosave",
               "dir=%s enabled=%s saves=%u last=%.1fs",
               a->dir, a->enabled ? "yes" : "no",
               a->save_count, a->last_save_time);
}
