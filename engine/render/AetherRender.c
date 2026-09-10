#include "AetherRender.h"
#include <stdlib.h>
#include <string.h>

static const aether_render_backend_vtbl_t k_metal_vtbl = {
    "metal",
    aether_metal_init,
    aether_metal_resize,
    aether_metal_submit,
    aether_metal_shutdown
};

struct aether_renderer {
    aether_render_backend_t            backend;
    const aether_render_backend_vtbl_t *vt;
    void                              *user;
    u32                                w, h;
    bool                               initialized;
};

static aether_result_t null_init    (void *user, u32 w, u32 h) { (void)user; (void)w; (void)h; return AETHER_OK; }
static aether_result_t null_resize  (void *user, u32 w, u32 h) { (void)user; (void)w; (void)h; return AETHER_OK; }
static aether_result_t null_submit  (void *user, const aether_render_cmd_t *c) { (void)user; (void)c; return AETHER_OK; }
static aether_result_t null_shutdown(void *user) { (void)user; return AETHER_OK; }

static const aether_render_backend_vtbl_t k_null_vtbl = {
    "null", null_init, null_resize, null_submit, null_shutdown
};

aether_renderer_t *aether_renderer_create(aether_render_backend_t backend,
                                          void *backend_user) {
    aether_renderer_t *r = (aether_renderer_t*)calloc(1, sizeof *r);
    if (!r) return NULL;
    r->backend = backend;
    r->user    = backend_user;
    switch (backend) {
        case AETHER_RENDER_NULL:  r->vt = &k_null_vtbl; break;
        case AETHER_RENDER_METAL: r->vt = NULL;         break;
        default: free(r); return NULL;
    }
    aether_log(AETHER_LOG_INFO, "render", "renderer created (backend=%d)", (int)backend);
    return r;
}

aether_result_t aether_renderer_set_backend_vtbl(aether_renderer_t *r,
                                                 const aether_render_backend_vtbl_t *vt,
                                                 void *user) {
    if (!r || !vt || !vt->init || !vt->submit || !vt->shutdown)
        return AETHER_ERR_INVALID_ARG;
    r->vt   = vt;
    r->user = user;
    aether_log(AETHER_LOG_INFO, "render", "backend vtable set: '%s'", vt->name);
    return AETHER_OK;
}

aether_result_t aether_renderer_install_metal(aether_renderer_t *r, void *user) {
    if (!r) return AETHER_ERR_INVALID_ARG;
    return aether_renderer_set_backend_vtbl(r, &k_metal_vtbl, user);
}

void aether_renderer_destroy(aether_renderer_t *r) {
    if (!r) return;
    if (r->initialized) aether_renderer_shutdown(r);
    free(r);
}

aether_result_t aether_renderer_init(aether_renderer_t *r, u32 w, u32 h) {
    if (!r || !r->vt) return AETHER_ERR_NOT_READY;
    aether_result_t res = r->vt->init(r->user, w, h);
    if (res != AETHER_OK) return res;
    r->w = w; r->h = h; r->initialized = true;
    aether_log(AETHER_LOG_INFO, "render", "initialized %ux%u", w, h);
    return AETHER_OK;
}

aether_result_t aether_renderer_resize(aether_renderer_t *r, u32 w, u32 h) {
    if (!r || !r->vt || !r->vt->resize) return AETHER_ERR_NOT_READY;
    aether_result_t res = r->vt->resize(r->user, w, h);
    if (res == AETHER_OK) { r->w = w; r->h = h; }
    return res;
}

aether_result_t aether_renderer_shutdown(aether_renderer_t *r) {
    if (!r || !r->vt) return AETHER_ERR_NOT_READY;
    aether_result_t res = r->vt->shutdown(r->user);
    r->initialized = false;
    aether_log(AETHER_LOG_INFO, "render", "shutdown");
    return res;
}

static aether_result_t submit(aether_renderer_t *r, const aether_render_cmd_t *c) {
    if (!r || !r->vt || !r->vt->submit) return AETHER_ERR_NOT_READY;
    return r->vt->submit(r->user, c);
}

aether_result_t aether_renderer_begin_frame(aether_renderer_t *r,
                                             f32 cr, f32 cg, f32 cb, f32 ca) {
    if (!r) return AETHER_ERR_INVALID_ARG;
    aether_render_cmd_t c = {0};
    c.type = AETHER_CMD_BEGIN_FRAME;
    c.viewport_w = r->w; c.viewport_h = r->h;
    c.clear_rgba[0] = cr; c.clear_rgba[1] = cg;
    c.clear_rgba[2] = cb; c.clear_rgba[3] = ca;
    return submit(r, &c);
}

aether_result_t aether_renderer_set_camera(aether_renderer_t *r,
                                           aether_mat4_t view,
                                           aether_mat4_t proj) {
    if (!r) return AETHER_ERR_INVALID_ARG;
    aether_render_cmd_t c = {0};
    c.type = AETHER_CMD_SET_VIEWPORT;
    c.view = view; c.projection = proj;
    c.viewport_w = r->w; c.viewport_h = r->h;
    return submit(r, &c);
}

aether_result_t aether_renderer_draw_world(aether_renderer_t *r) {
    if (!r) return AETHER_ERR_INVALID_ARG;
    aether_render_cmd_t c = {0}; c.type = AETHER_CMD_DRAW_WORLD;
    return submit(r, &c);
}

aether_result_t aether_renderer_draw_hud(aether_renderer_t *r) {
    if (!r) return AETHER_ERR_INVALID_ARG;
    aether_render_cmd_t c = {0}; c.type = AETHER_CMD_DRAW_HUD;
    return submit(r, &c);
}

aether_result_t aether_renderer_end_frame(aether_renderer_t *r) {
    if (!r) return AETHER_ERR_INVALID_ARG;
    aether_render_cmd_t c = {0}; c.type = AETHER_CMD_END_FRAME;
    return submit(r, &c);
}

u32 aether_renderer_width (const aether_renderer_t *r) { return r ? r->w : 0; }
u32 aether_renderer_height(const aether_renderer_t *r) { return r ? r->h : 0; }
