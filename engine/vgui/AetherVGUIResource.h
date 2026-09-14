#ifndef AETHER_VGUI_RESOURCE_H
#define AETHER_VGUI_RESOURCE_H
#include "../core/AetherCore.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_VGUI_RESOURCE_MAX 128
typedef struct { char key[96]; char value[256]; } aether_vgui_kv_t;
typedef struct { aether_vgui_kv_t items[AETHER_VGUI_RESOURCE_MAX]; u32 count; } aether_vgui_resource_t;
void aether_vgui_resource_init(aether_vgui_resource_t*);
aether_result_t aether_vgui_resource_parse(aether_vgui_resource_t*, const char*text);
const char *aether_vgui_resource_get(const aether_vgui_resource_t*, const char*key, const char*fallback);
aether_result_t aether_vgui_resource_load_file(aether_vgui_resource_t*, const char*path);
#ifdef __cplusplus
}
#endif
#endif
