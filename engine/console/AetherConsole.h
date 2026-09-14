#ifndef AETHER_CONSOLE_H
#define AETHER_CONSOLE_H
#include "../core/AetherCore.h"
#include "AetherCVar.h"
#include "../command/AetherCmd.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_CONSOLE_LINE_MAX 1024
#define AETHER_CONSOLE_HISTORY_MAX 256
typedef void (*aether_console_sink_fn)(const char *line, void *user);
typedef struct aether_console aether_console_t;
aether_console_t *aether_console_create(void);
void aether_console_destroy(aether_console_t*c);
aether_cvar_registry_t *aether_console_cvars(aether_console_t*c);
aether_cmd_registry_t *aether_console_commands(aether_console_t*c);
void aether_console_set_sink(aether_console_t*c,aether_console_sink_fn fn,void*user);
aether_result_t aether_console_execute(aether_console_t*c,const char*line);
void aether_console_write(aether_console_t*c,aether_log_level_t level,const char*fmt,...);
void aether_console_register_defaults(aether_console_t*c);
#ifdef __cplusplus
}
#endif
#endif
