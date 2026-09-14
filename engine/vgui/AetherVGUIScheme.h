#ifndef AETHER_VGUI_SCHEME_H
#define AETHER_VGUI_SCHEME_H
#include "AetherVGUIResource.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_VGUI_MAX_FONTS 32
typedef struct { f32 r,g,b,a; } aether_vgui_rgba_t;
typedef struct { char name[64]; char face[128]; i32 tall; bool bold; bool italic; } aether_vgui_font_t;
typedef struct { aether_vgui_rgba_t fg,bg,accent; aether_vgui_font_t fonts[AETHER_VGUI_MAX_FONTS]; u32 font_count; } aether_vgui_scheme_t;
void aether_vgui_scheme_init(aether_vgui_scheme_t*);
aether_result_t aether_vgui_scheme_parse(aether_vgui_scheme_t*,const char*text);
const aether_vgui_font_t *aether_vgui_scheme_font(const aether_vgui_scheme_t*,const char*name);
#ifdef __cplusplus
}
#endif
#endif
