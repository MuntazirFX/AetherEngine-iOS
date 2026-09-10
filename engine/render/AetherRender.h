/* AetherRender.h — Renderer abstraction layer.
 * Multiple backends possible: Metal (iOS), Null (tests/host).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_RENDER_H
#define AETHER_RENDER_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aether_render_backend {
    AETHER_RENDER_METAL = 0,   /* iOS default */
    AETHER_RENDER_NULL,        /* headless / tests */
    AETHER_RENDER_COUNT
} aether_render_backend_t;

/* A renderer command is the boundary between engine and backend.
 * Backends interpret these; the engine never touches platform APIs directly. */
typedef enum aether_render_cmd_type {
    AETHER_CMD_NONE = 0,
    AETHER_CMD_BEGIN_FRAME,
    AETHER_CMD_CLEAR,
    AETHER_CMD_SET_VIEWPORT,
    AETHER_CMD_DRAW_WORLD,
    AETHER_CMD_DRAW_HUD,
    AETHER_CMD_END_FRAME,
} aether_render_cmd_type_t;

typedef struct aether_render_cmd {
    aether_render_cmd_type_t type;
    f32                      clear_rgba[4];
    aether_mat4_t            view;
    aether_mat4_t            projection;
    u32                      viewport_w;
    u32                      viewport_h;
} aether_render_cmd_t;

/* Backend vtable — implemented by the platform (Metal / Null). */
typedef struct aether_render_backend_vtbl {
    const char       *name;
    aether_result_t (*init)    (void *user, u32 w, u32 h);
    aether_result_t (*resize)  (void *user, u32 w, u32 h);
    aether_result_t (*submit)  (void *user, const aether_render_cmd_t *cmd);
    aether_result_t (*shutdown)(void *user);
} aether_render_backend_vtbl_t;

typedef struct aether_renderer aether_renderer_t;

aether_renderer_t *aether_renderer_create(aether_render_backend_t backend,
                                          void *backend_user);

/* Plug in a custom backend (iOS Swift Metal bridge). */
aether_result_t aether_renderer_set_backend_vtbl(aether_renderer_t *r,
                                                 const aether_render_backend_vtbl_t *vt,
                                                 void *user);

void            aether_renderer_destroy(aether_renderer_t *r);
aether_result_t aether_renderer_init(aether_renderer_t *r, u32 w, u32 h);
aether_result_t aether_renderer_resize(aether_renderer_t *r, u32 w, u32 h);
aether_result_t aether_renderer_shutdown(aether_renderer_t *r);

/* Engine-side frame API */
aether_result_t aether_renderer_begin_frame(aether_renderer_t *r,
                                             f32 r_, f32 g, f32 b, f32 a);
aether_result_t aether_renderer_set_camera(aether_renderer_t *r,
                                           aether_mat4_t view,
                                           aether_mat4_t proj);
aether_result_t aether_renderer_draw_world(aether_renderer_t *r);
aether_result_t aether_renderer_draw_hud  (aether_renderer_t *r);
aether_result_t aether_renderer_end_frame (aether_renderer_t *r);

u32 aether_renderer_width (const aether_renderer_t *r);
u32 aether_renderer_height(const aether_renderer_t *r);

/* Convenience: install the Metal backend vtable.
 * Called by EngineBridge.c on iOS. */
aether_result_t aether_renderer_install_metal(aether_renderer_t *r, void *user);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_RENDER_H */
