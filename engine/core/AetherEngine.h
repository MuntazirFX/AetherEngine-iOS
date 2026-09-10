#ifndef AETHER_ENGINE_H
#define AETHER_ENGINE_H

#include "AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MAX_SUBSYSTEMS 16

typedef struct aether_engine_desc {
    const char *base_path;
    const char *asset_path;
    u32         flags;
} aether_engine_desc_t;

typedef struct aether_subsystem {
    const char        *name;
    void              *user;
    aether_result_t  (*init)    (void *user);
    aether_result_t  (*tick)    (void *user, f32 dt);
    aether_result_t  (*shutdown)(void *user);
    bool               ready;
} aether_subsystem_t;

typedef struct aether_engine aether_engine_t;

aether_engine_t *aether_engine_create(const aether_engine_desc_t *desc);
aether_result_t  aether_engine_destroy(aether_engine_t *e);

aether_result_t  aether_engine_start(aether_engine_t *e);
aether_result_t  aether_engine_stop (aether_engine_t *e);
aether_result_t  aether_engine_run  (aether_engine_t *e);
aether_result_t  aether_engine_step (aether_engine_t *e, f32 dt);

aether_result_t  aether_engine_register_subsystem(aether_engine_t *e, const aether_subsystem_t *sub);

u32              aether_engine_subsystem_count(const aether_engine_t *e);
const char      *aether_engine_subsystem_name (const aether_engine_t *e, u32 idx);
bool             aether_engine_subsystem_ready(const aether_engine_t *e, u32 idx);

const char      *aether_engine_base_path (const aether_engine_t *e);
const char      *aether_engine_asset_path(const aether_engine_t *e);

u64              aether_engine_frame_count(const aether_engine_t *e);
f64              aether_engine_elapsed    (const aether_engine_t *e);

void             aether_engine_request_exit(aether_engine_t *e);

#ifdef __cplusplus
}
#endif
#endif
