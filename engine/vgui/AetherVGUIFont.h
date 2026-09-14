#ifndef AETHER_VGUI_FONT_H
#define AETHER_VGUI_FONT_H
#include "AetherVGUIScheme.h"
#ifdef __cplusplus
extern "C" {
#endif
aether_result_t aether_vgui_font_register(aether_vgui_scheme_t*,const char*,const char*,i32,bool,bool);
const aether_vgui_font_t *aether_vgui_font_find(const aether_vgui_scheme_t*,const char*);
#ifdef __cplusplus
}
#endif
#endif
