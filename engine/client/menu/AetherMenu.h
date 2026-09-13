/* AetherMenu.h — VGUI-style menu framework.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MENU_H
#define AETHER_MENU_H

#include "../../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MENU_MAX_PANELS  8
#define AETHER_MENU_MAX_ITEMS   32

/* Menu panel IDs */
typedef enum aether_menu_panel_id {
    AETHER_MENU_MAIN = 0,
    AETHER_MENU_NEW_GAME,
    AETHER_MENU_LOAD_GAME,
    AETHER_MENU_OPTIONS,
    AETHER_MENU_MULTIPLAYER,
    AETHER_MENU_QUIT_CONFIRM,
    AETHER_MENU_PANEL_COUNT
} aether_menu_panel_id_t;

/* Menu item types */
typedef enum aether_menu_item_type {
    AETHER_MENU_ITEM_BUTTON = 0,
    AETHER_MENU_ITEM_SLIDER,
    AETHER_MENU_ITEM_TOGGLE,
    AETHER_MENU_ITEM_LABEL,
} aether_menu_item_type_t;

/* Menu item callback */
typedef void (*aether_menu_action_fn)(void *user);

/* Menu item */
typedef struct aether_menu_item {
    const char               *label;
    aether_menu_item_type_t   type;
    bool                      enabled;
    bool                      visible;

    /* For sliders */
    f32                       value;
    f32                       min_value;
    f32                       max_value;

    /* For toggles */
    bool                      toggle_state;

    /* Action */
    aether_menu_action_fn     action;
    void                     *action_user;

    /* Layout (normalized 0..1) */
    f32                       x, y;
    f32                       w, h;
} aether_menu_item_t;

/* Menu panel */
typedef struct aether_menu_panel {
    aether_menu_panel_id_t  id;
    const char             *title;
    bool                    visible;
    aether_menu_item_t      items[AETHER_MENU_MAX_ITEMS];
    u32                     item_count;
    i32                     selected_item;
} aether_menu_panel_t;

/* Menu manager */
typedef struct aether_menu aether_menu_t;

aether_menu_t *aether_menu_create(void);
void           aether_menu_destroy(aether_menu_t *menu);

/* Panel management */
aether_menu_panel_t *aether_menu_get_panel(aether_menu_t *menu, aether_menu_panel_id_t id);
void                 aether_menu_show_panel(aether_menu_t *menu, aether_menu_panel_id_t id);
void                 aether_menu_hide_panel(aether_menu_t *menu, aether_menu_panel_id_t id);
void                 aether_menu_hide_all(aether_menu_t *menu);
bool                 aether_menu_is_visible(const aether_menu_t *menu);

/* Item management */
i32  aether_menu_add_button(aether_menu_panel_t *panel, const char *label,
                             aether_menu_action_fn action, void *user);
i32  aether_menu_add_label (aether_menu_panel_t *panel, const char *label);
i32  aether_menu_add_slider(aether_menu_panel_t *panel, const char *label,
                             f32 min_v, f32 max_v, f32 def);
i32  aether_menu_add_toggle(aether_menu_panel_t *panel, const char *label,
                             bool def);

/* Input handling */
void aether_menu_navigate(aether_menu_t *menu, i32 direction);
void aether_menu_activate(aether_menu_t *menu);
void aether_menu_back    (aether_menu_t *menu);

/* Drawing (called each frame) */
void aether_menu_draw(aether_menu_t *menu, void *render_ctx);

/* Diagnostics */
void aether_menu_dump(const aether_menu_t *menu);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MENU_H */
