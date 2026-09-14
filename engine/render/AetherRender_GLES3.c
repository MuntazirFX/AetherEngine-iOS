#include "AetherRender_GLES3.h"
#include <string.h>

static aether_result_t init(void *u,u32 w,u32 h){(void)u;(void)w;(void)h;return AETHER_OK;}
static aether_result_t resize(void *u,u32 w,u32 h){(void)u;(void)w;(void)h;return AETHER_OK;}
static aether_result_t submit(void *u,const aether_render_cmd_t *c){(void)u;(void)c;return AETHER_OK;}
static aether_result_t shutdown(void *u){(void)u;return AETHER_OK;}
static const aether_render_backend_vtbl_t vtbl={"GLES3",init,resize,submit,shutdown};
const aether_render_backend_vtbl_t *aether_render_gles3_backend(void){return &vtbl;}
bool aether_render_gles3_supported(void){return true;}
