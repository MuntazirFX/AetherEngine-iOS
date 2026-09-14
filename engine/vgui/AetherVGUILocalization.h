#ifndef AETHER_VGUI_LOCALIZATION_H
#define AETHER_VGUI_LOCALIZATION_H
#include "../core/AetherCore.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_VGUI_MAX_TOKENS 256
typedef struct { char token[96]; char text[256]; } aether_vgui_loc_entry_t;
typedef struct { aether_vgui_loc_entry_t entries[AETHER_VGUI_MAX_TOKENS]; u32 count; } aether_vgui_localization_t;
void aether_vgui_localization_init(aether_vgui_localization_t*);
aether_result_t aether_vgui_localization_parse(aether_vgui_localization_t*,const char*text);
const char *aether_vgui_localize(const aether_vgui_localization_t*,const char*token);
#ifdef __cplusplus
}
#endif
#endif
