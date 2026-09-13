/* AetherMenu.c — Menu framework implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMenu.h"
#include <stdlib.h>
#include <string.h>

struct aether_menu {
    aether_menu_panel_t panels[AETHER_MENU_PANEL_COUNT];
    aether_menu_panel_id_t current_panel;
    bool any_visible;
};

aether_menu_t *aether_menu_create(void) {
    aether_menu_t *menu = (aether_menu_t*)calloc(1, sizeof *menu);
    if (!menu) return NULL;

    for (int i = 0; i < AETHER_MENU_PANEL_COUNT; ++i) {
        menu->panels[i].id = (aether_menu_panel_id_t)i;
        menu->panels[i].visible = false;
        menu->panels[i].item_count = 0;
        menu->panels[i].selected_item = -1;
    }

    menu->panels[AETHER_MENU_MAIN].title = "Main Menu";
    menu->panels[AETHER_MENU_NEW_GAME].title = "New Game";
    menu->panels[AETHER_MENU_LOAD_GAME].title = "Load Game";
    menu->panels[AETHER_MENU_OPTIONS].title = "Options";
    menu->panels[AETHER_MENU_MULTIPLAYER].title = "Multiplayer";
    menu->panels[AETHER_MENU_QUIT_CONFIRM].title = "Quit";

    menu->current_panel = AETHER_MENU_MAIN;
    menu->any_visible = false;

    aether_log(AETHER_LOG_INFO, "menu", "menu system created");
    return menu;
}

void aether_menu_destroy(aether_menu_t *menu) {
    if (!menu) return;
    free(menu);
    aether_log(AETHER_LOG_INFO, "menu", "menu destroyed");
}

aether_menu_panel_t *aether_menu_get_panel(aether_menu_t *menu, aether_menu_panel_id_t id) {
    if (!menu || id < 0 || id >= AETHER_MENU_PANEL_COUNT) return NULL;
    return &menu->panels[id];
}

void aether_menu_show_panel(aether_menu_t *menu, aether_menu_panel_id_t id) {
    if (!menu || id < 0 || id >= AETHER_MENU_PANEL_COUNT) return;
    for (int i = 0; i < AETHER_MENU_PANEL_COUNT; ++i)
        menu->panels[i].visible = false;
    menu->panels[id].visible = true;
    menu->panels[id].selected_item = 0;
    menu->current_panel = id;
    menu->any_visible = true;
    aether_log(AETHER_LOG_INFO, "menu", "showing panel: %s",
               menu->panels[id].title ? menu->panels[id].title : "?");
}

void aether_menu_hide_panel(aether_menu_t *menu, aether_menu_panel_id_t id) {
    if (!menu || id < 0 || id >= AETHER_MENU_PANEL_COUNT) return;
    menu->panels[id].visible = false;
    menu->any_visible = false;
    for (int i = 0; i < AETHER_MENU_PANEL_COUNT; ++i) {
        if (menu->panels[i].visible) { menu->any_visible = true; break; }
    }
}

void aether_menu_hide_all(aether_menu_t *menu) {
    if (!menu) return;
    for (int i = 0; i < AETHER_MENU_PANEL_COUNT; ++i)
        menu->panels[i].visible = false;
    menu->any_visible = false;
}

bool aether_menu_is_visible(const aether_menu_t *menu) {
    return menu && menu->any_visible;
}

static aether_menu_item_t *add_item(aether_menu_panel_t *panel,
                                     const char *label,
                                     aether_menu_item_type_t type) {
    if (!panel || !label) return NULL;
    if (panel->item_count >= AETHER_MENU_MAX_ITEMS) return NULL;

    aether_menu_item_t *it = &panel->items[panel->item_count];
    memset(it, 0, sizeof *it);
    it->label = label;
    it->type = type;
    it->enabled = true;
    it->visible = true;
    it->x = 0.35f;
    it->w = 0.30f;
    it->h = 0.06f;
    it->y = 0.35f + (f32)panel->item_count * 0.08f;
    panel->item_count++;
    return it;
}

i32 aether_menu_add_button(aether_menu_panel_t *panel, const char *label,
                            aether_menu_action_fn action, void *user) {
    aether_menu_item_t *it = add_item(panel, label, AETHER_MENU_ITEM_BUTTON);
    if (!it) return -1;
    it->action = action;
    it->action_user = user;
    return (i32)(panel->item_count - 1);
}

i32 aether_menu_add_label(aether_menu_panel_t *panel, const char *label) {
    aether_menu_item_t *it = add_item(panel, label, AETHER_MENU_ITEM_LABEL);
    if (!it) return -1;
    it->enabled = false;
    return (i32)(panel->item_count - 1);
}

i32 aether_menu_add_slider(aether_menu_panel_t *panel, const char *label,
                            f32 min_v, f32 max_v, f32 def) {
    aether_menu_item_t *it = add_item(panel, label, AETHER_MENU_ITEM_SLIDER);
    if (!it) return -1;
    it->min_value = min_v;
    it->max_value = max_v;
    it->value = def;
    return (i32)(panel->item_count - 1);
}

i32 aether_menu_add_toggle(aether_menu_panel_t *panel, const char *label,
                            bool def) {
    aether_menu_item_t *it = add_item(panel, label, AETHER_MENU_ITEM_TOGGLE);
    if (!it) return -1;
    it->toggle_state = def;
    return (i32)(panel->item_count - 1);
}

void aether_menu_navigate(aether_menu_t *menu, i32 direction) {
    if (!menu || !menu->any_visible) return;
    aether_menu_panel_t *panel = &menu->panels[menu->current_panel];
    if (panel->item_count == 0) return;

    i32 start = panel->selected_item;
    i32 i = start;
    for (u32 step = 0; step < panel->item_count; ++step) {
        i += direction;
        if (i < 0) i = (i32)panel->item_count - 1;
        if (i >= (i32)panel->item_count) i = 0;
        if (panel->items[i].enabled && panel->items[i].type != AETHER_MENU_ITEM_LABEL) {
            panel->selected_item = i;
            return;
        }
    }
}

void aether_menu_activate(aether_menu_t *menu) {
    if (!menu || !menu->any_visible) return;
    aether_menu_panel_t *panel = &menu->panels[menu->current_panel];
    if (panel->selected_item < 0) return;
    aether_menu_item_t *it = &panel->items[panel->selected_item];
    if (!it->enabled) return;

    switch (it->type) {
        case AETHER_MENU_ITEM_BUTTON:
            aether_log(AETHER_LOG_INFO, "menu", "activated button: '%s'", it->label);
            if (it->action) it->action(it->action_user);
            break;
        case AETHER_MENU_ITEM_TOGGLE:
            it->toggle_state = !it->toggle_state;
            aether_log(AETHER_LOG_INFO, "menu", "toggle '%s' -> %d",
                       it->label, it->toggle_state);
            break;
        case AETHER_MENU_ITEM_SLIDER:
            /* Move value right by 10% */
            it->value += (it->max_value - it->min_value) * 0.1f;
            if (it->value > it->max_value) it->value = it->min_value;
            aether_log(AETHER_LOG_INFO, "menu", "slider '%s' -> %.2f",
                       it->label, it->value);
            break;
        default: break;
    }
}

void aether_menu_back(aether_menu_t *menu) {
    if (!menu) return;
    /* From any submenu, go back to main. From main, hide. */
    if (menu->current_panel == AETHER_MENU_MAIN) {
        aether_menu_hide_all(menu);
    } else {
        aether_menu_show_panel(menu, AETHER_MENU_MAIN);
    }
}

void aether_menu_draw(aether_menu_t *menu, void *render_ctx) {
    if (!menu || !menu->any_visible) return;
    aether_menu_panel_t *panel = &menu->panels[menu->current_panel];
    if (!panel->visible) return;

    aether_log(AETHER_LOG_TRACE, "menu", "=== %s ===", panel->title);
    for (u32 i = 0; i < panel->item_count; ++i) {
        aether_menu_item_t *it = &panel->items[i];
        if (!it->visible) continue;
        const char *sel = (i == (u32)panel->selected_item) ? "> " : "  ";
        switch (it->type) {
            case AETHER_MENU_ITEM_BUTTON:
                aether_log(AETHER_LOG_TRACE, "menu", "%s%s", sel, it->label);
                break;
            case AETHER_MENU_ITEM_TOGGLE:
                aether_log(AETHER_LOG_TRACE, "menu", "%s%s [%s]",
                           sel, it->label, it->toggle_state ? "X" : " ");
                break;
            case AETHER_MENU_ITEM_SLIDER:
                aether_log(AETHER_LOG_TRACE, "menu", "%s%s <%.2f>",
                           sel, it->label, it->value);
                break;
            case AETHER_MENU_ITEM_LABEL:
                aether_log(AETHER_LOG_TRACE, "menu", "   %s", it->label);
                break;
        }
    }
    (void)render_ctx;
}

void aether_menu_dump(const aether_menu_t *menu) {
    if (!menu) return;
    aether_log(AETHER_LOG_INFO, "menu", "===== MENU DUMP =====");
    for (int i = 0; i < AETHER_MENU_PANEL_COUNT; ++i) {
        const aether_menu_panel_t *p = &menu->panels[i];
        aether_log(AETHER_LOG_INFO, "menu", "  panel[%d] %-15s items=%u visible=%s",
                   i, p->title ? p->title : "?", p->item_count,
                   p->visible ? "yes" : "no");
    }
    aether_log(AETHER_LOG_INFO, "menu", "  current: %s",
               menu->panels[menu->current_panel].title);
    aether_log(AETHER_LOG_INFO, "menu", "======================");
}
