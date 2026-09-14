/* AetherVGUI.h — clean-room VGUI-style retained-mode UI core. */
#ifndef AETHER_VGUI_H
#define AETHER_VGUI_H
#include "../core/AetherCore.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_VGUI_MAX_PANELS 64
#define AETHER_VGUI_MAX_CHILDREN 32
#define AETHER_VGUI_MAX_TEXT 256
typedef enum { AETHER_VGUI_PANEL=0, AETHER_VGUI_BUTTON, AETHER_VGUI_LABEL, AETHER_VGUI_CHECKBOX, AETHER_VGUI_SLIDER, AETHER_VGUI_TEXTFIELD, AETHER_VGUI_LIST } aether_vgui_type_t;
typedef struct aether_vgui_panel aether_vgui_panel_t;
typedef void (*aether_vgui_action_fn)(aether_vgui_panel_t*, void*);
typedef struct { f32 x,y,w,h; } aether_vgui_rect_t;
typedef struct { f32 r,g,b,a; } aether_vgui_color_t;
typedef struct aether_vgui_panel {
    u32 id; aether_vgui_type_t type; bool visible; bool enabled; bool focused;
    aether_vgui_rect_t bounds; char name[AETHER_VGUI_MAX_TEXT]; char text[AETHER_VGUI_MAX_TEXT];
    i32 value; f32 slider_min, slider_max, slider_value; bool checked;
    u32 parent_id; u32 child_count; u32 children[AETHER_VGUI_MAX_CHILDREN];
    aether_vgui_action_fn action; void *action_user;
} aether_vgui_panel_t;
typedef struct aether_vgui aether_vgui_t;
aether_vgui_t *aether_vgui_create(void); void aether_vgui_destroy(aether_vgui_t*);
u32 aether_vgui_add(aether_vgui_t*, aether_vgui_type_t, u32 parent_id, const char*, const char*);
aether_vgui_panel_t *aether_vgui_panel(aether_vgui_t*, u32 id);
aether_result_t aether_vgui_remove(aether_vgui_t*, u32 id);
aether_result_t aether_vgui_set_visible(aether_vgui_t*, u32 id, bool visible);
aether_result_t aether_vgui_set_bounds(aether_vgui_t*, u32 id, f32 x,f32 y,f32 w,f32 h);
aether_result_t aether_vgui_set_text(aether_vgui_t*, u32 id, const char* text);
aether_result_t aether_vgui_set_action(aether_vgui_t*, u32 id, aether_vgui_action_fn fn, void* user);
aether_result_t aether_vgui_activate(aether_vgui_t*, u32 id);
aether_result_t aether_vgui_navigate(aether_vgui_t*, u32 parent_id, i32 direction);
u32 aether_vgui_child_count(const aether_vgui_t*, u32 parent_id);
const aether_vgui_panel_t *aether_vgui_child_at(const aether_vgui_t*, u32 parent_id, u32 index);
void aether_vgui_draw(const aether_vgui_t*, void *render_ctx);
#ifdef __cplusplus
}
#endif
#endif
