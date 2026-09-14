#ifndef AETHER_RENDER_GL_H
#define AETHER_RENDER_GL_H
#include "AetherRender.h"
const aether_render_backend_vtbl_t *aether_render_gl_backend(void);
bool aether_render_gl_supported(void);
#endif
