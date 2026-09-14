#ifndef AETHER_VGUI1_H
#define AETHER_VGUI1_H
#include "AetherVGUI.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef aether_vgui_t aether_vgui1_t;
static inline aether_vgui1_t *aether_vgui1_create(void){return aether_vgui_create();}
static inline void aether_vgui1_destroy(aether_vgui1_t*v){aether_vgui_destroy(v);}
#ifdef __cplusplus
}
#endif
#endif
