#include "AetherEngine.h"
#include "../thread/AetherThread.h"
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#define AETHER_PATH_MAX 512

struct aether_engine {
    char               base_path [AETHER_PATH_MAX];
    char               asset_path[AETHER_PATH_MAX];
    aether_subsystem_t subs[AETHER_MAX_SUBSYSTEMS];
    u32                sub_count;
    bool               running;
    bool               exit_requested;
    u64                frame;
    f64                t_start;
    f64                t_last;
    f32                fixed_dt;
    f32                last_dt;
};

static f64 now_seconds(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        return (f64)tv.tv_sec + (f64)tv.tv_usec / 1000000.0;
    }
    return 0.0;
}

aether_engine_t *aether_engine_create(const aether_engine_desc_t *desc) {
    if (!desc || !desc->base_path || !desc->asset_path) {
        aether_log(AETHER_LOG_ERROR, "engine", "invalid engine desc");
        return NULL;
    }
    aether_engine_t *e = (aether_engine_t*)calloc(1, sizeof *e);
    if (!e) return NULL;
    aether_str_copy(e->base_path,  sizeof e->base_path,  desc->base_path);
    aether_str_copy(e->asset_path, sizeof e->asset_path, desc->asset_path);
    e->fixed_dt = 1.0f / 120.0f;
    e->last_dt  = 0.0f;
    aether_log(AETHER_LOG_INFO, "engine", "created: base='%s' asset='%s'", e->base_path, e->asset_path);
    return e;
}

aether_result_t aether_engine_destroy(aether_engine_t *e) {
    if (!e) return AETHER_ERR_INVALID_ARG;
    if (e->running) aether_engine_stop(e);
    free(e);
    return AETHER_OK;
}

aether_result_t aether_engine_register_subsystem(aether_engine_t *e, const aether_subsystem_t *sub) {
    if (!e || !sub || !sub->name) return AETHER_ERR_INVALID_ARG;
    if (e->running) return AETHER_ERR_STATE;
    if (e->sub_count >= AETHER_MAX_SUBSYSTEMS) return AETHER_ERR_OUT_OF_MEM;
    e->subs[e->sub_count] = *sub;
    e->subs[e->sub_count].ready = false;
    aether_log(AETHER_LOG_DEBUG, "engine", "registered subsystem '%s' at slot %u", sub->name, e->sub_count);
    e->sub_count++;
    return AETHER_OK;
}

u32 aether_engine_subsystem_count(const aether_engine_t *e) { return e ? e->sub_count : 0; }

const char *aether_engine_subsystem_name(const aether_engine_t *e, u32 idx) {
    if (!e || idx >= e->sub_count) return NULL;
    return e->subs[idx].name;
}

bool aether_engine_subsystem_ready(const aether_engine_t *e, u32 idx) {
    if (!e || idx >= e->sub_count) return false;
    return e->subs[idx].ready;
}

aether_result_t aether_engine_start(aether_engine_t *e) {
    if (!e) return AETHER_ERR_INVALID_ARG;
    if (e->running) return AETHER_ERR_ALREADY;

    for (u32 i = 0; i < e->sub_count; ++i) {
        aether_subsystem_t *s = &e->subs[i];
        if (!s->init) { s->ready = true; continue; }
        aether_result_t r = s->init(s->user);
        if (r != AETHER_OK) {
            aether_log(AETHER_LOG_ERROR, "engine", "subsystem '%s' init failed: %s", s->name, aether_result_string(r));
            for (i32 j = (i32)i - 1; j >= 0; --j) {
                if (e->subs[j].shutdown) (void)e->subs[j].shutdown(e->subs[j].user);
                e->subs[j].ready = false;
            }
            return r;
        }
        s->ready = true;
    }
    e->t_start = now_seconds(); e->t_last = e->t_start;
    e->frame = 0; e->running = true; e->exit_requested = false;
    e->last_dt = 0.0f;
    aether_log(AETHER_LOG_INFO, "engine", "started (%u subsystems)", e->sub_count);
    return AETHER_OK;
}

aether_result_t aether_engine_stop(aether_engine_t *e) {
    if (!e) return AETHER_ERR_INVALID_ARG;
    if (!e->running) return AETHER_ERR_NOT_READY;
    for (i32 i = (i32)e->sub_count - 1; i >= 0; --i) {
        aether_subsystem_t *s = &e->subs[i];
        if (s->ready && s->shutdown) (void)s->shutdown(s->user);
        s->ready = false;
    }
    e->running = false;
    aether_log(AETHER_LOG_INFO, "engine", "stopped after %llu frames", (unsigned long long)e->frame);
    return AETHER_OK;
}

aether_result_t aether_engine_step(aether_engine_t *e, f32 dt) {
    if (!e) return AETHER_ERR_INVALID_ARG;
    if (!e->running) return AETHER_ERR_NOT_READY;
    if (dt < 0.0f) dt = 0.0f;
    e->last_dt = dt;
    for (u32 i = 0; i < e->sub_count; ++i) {
        aether_subsystem_t *s = &e->subs[i];
        if (!s->ready || !s->tick) continue;
        aether_result_t r = s->tick(s->user, dt);
        if (r != AETHER_OK) {
            aether_log(AETHER_LOG_WARN, "engine", "subsystem '%s' tick returned %s", s->name, aether_result_string(r));
        }
    }
    e->frame++;
    return AETHER_OK;
}

aether_result_t aether_engine_host_frame(aether_engine_t *e, f32 real_dt) {
    if (!e) return AETHER_ERR_INVALID_ARG;
    if (!e->running) return AETHER_ERR_NOT_READY;
    f64 t_now = now_seconds();
    f32 dt = real_dt;
    if (dt <= 0.0f) {
        dt = (f32)(t_now - e->t_last);
    }
    e->t_last = t_now;
    if (dt > 0.25f) dt = 0.25f;
    if (dt < 0.0f) dt = 0.0f;
    return aether_engine_step(e, dt);
}

aether_result_t aether_engine_run(aether_engine_t *e) {
    if (!e) return AETHER_ERR_INVALID_ARG;
    if (!e->running) return AETHER_ERR_NOT_READY;
    aether_log(AETHER_LOG_INFO, "engine", "entering main loop @ %.1f Hz", 1.0 / (double)e->fixed_dt);
    while (!e->exit_requested) {
        f64 t_now = now_seconds();
        f32 dt    = (f32)(t_now - e->t_last);
        e->t_last = t_now;
        if (dt > 0.25f) dt = 0.25f;
        (void)aether_engine_step(e, dt);
        aether_thread_sleep_ms(1);
    }
    aether_log(AETHER_LOG_INFO, "engine", "exit requested; leaving main loop");
    return AETHER_OK;
}

void aether_engine_request_exit(aether_engine_t *e) { if (e) e->exit_requested = true; }

const char *aether_engine_base_path (const aether_engine_t *e) { return e ? e->base_path  : NULL; }
const char *aether_engine_asset_path(const aether_engine_t *e) { return e ? e->asset_path : NULL; }
u64         aether_engine_frame_count(const aether_engine_t *e) { return e ? e->frame : 0; }
f64         aether_engine_elapsed    (const aether_engine_t *e) {
    if (!e || !e->running) return 0.0;
    return now_seconds() - e->t_start;
}
f32  aether_engine_last_dt   (const aether_engine_t *e) { return e ? e->last_dt : 0.0f; }
bool aether_engine_is_running(const aether_engine_t *e) { return e ? e->running : false; }

void aether_engine_set_fixed_dt(aether_engine_t *e, f32 dt) {
    if (!e) return;
    if (dt < 1.0f / 1000.0f) dt = 1.0f / 1000.0f;
    if (dt > 0.25f) dt = 0.25f;
    e->fixed_dt = dt;
}
f32 aether_engine_fixed_dt(const aether_engine_t *e) { return e ? e->fixed_dt : (1.0f / 120.0f); }
