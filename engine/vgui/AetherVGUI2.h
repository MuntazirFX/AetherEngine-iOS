#ifndef AETHER_VGUI2_H
#define AETHER_VGUI2_H
#include "AetherVGUI.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef aether_vgui_t aether_vgui2_t;
static inline aether_vgui2_t *aether_vgui2_create(void){return aether_vgui_create();}
static inline void aether_vgui2_destroy(aether_vgui2_t*v){aether_vgui_destroy(v);}
#ifdef __cplusplus
}
#endif
#endif
