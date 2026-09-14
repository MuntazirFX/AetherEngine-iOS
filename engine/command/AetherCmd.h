#ifndef AETHER_CMD_H
#define AETHER_CMD_H
#include "../core/AetherCore.h"
#include "../console/AetherCVar.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_CMD_NAME_MAX 64
#define AETHER_CMD_MAX 256
#define AETHER_CMD_MAX_ARGS 64

typedef struct aether_cmd_context { int argc; const char *argv[AETHER_CMD_MAX_ARGS]; } aether_cmd_context_t;
typedef void (*aether_cmd_fn)(const aether_cmd_context_t *ctx, void *user);
typedef struct aether_cmd { char name[AETHER_CMD_NAME_MAX]; aether_cmd_fn fn; void *user; bool registered; } aether_cmd_t;
typedef struct aether_cmd_registry aether_cmd_registry_t;
aether_cmd_registry_t *aether_cmd_create(void);
void aether_cmd_destroy(aether_cmd_registry_t *r);
aether_cmd_t *aether_cmd_register(aether_cmd_registry_t *r,const char*name,aether_cmd_fn fn,void*user);
aether_cmd_t *aether_cmd_find(aether_cmd_registry_t*r,const char*name);
aether_result_t aether_cmd_execute(aether_cmd_registry_t*r,const char*line);
void aether_cmd_list(const aether_cmd_registry_t*r);
u32 aether_cmd_count(const aether_cmd_registry_t*r);
const aether_cmd_t *aether_cmd_at(const aether_cmd_registry_t*r,u32 index);
void aether_cmd_set_cvars(aether_cmd_registry_t*r,aether_cvar_registry_t*cvars);
#ifdef __cplusplus
}
#endif
#endif
