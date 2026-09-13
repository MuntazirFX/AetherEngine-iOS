/* AetherMainMenu.c — Main menu setup.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMainMenu.h"
#include <stdlib.h>

/* ---- Static action handlers (fallback) ---- */
static void default_new_game(void *u) {
    (void)u;
    aether_log(AETHER_LOG_INFO, "mainmenu", "New Game clicked");
}
static void default_load_game(void *u) {
    (void)u;
    aether_log(AETHER_LOG_INFO, "mainmenu", "Load Game clicked");
}
static void default_options(void *u) {
    (void)u;
    aether_log(AETHER_LOG_INFO, "mainmenu", "Options clicked");
}
static void default_multiplayer(void *u) {
    (void)u;
    aether_log(AETHER_LOG_INFO, "mainmenu", "Multiplayer clicked");
}
static void default_quit(void *u) {
    (void)u;
    aether_log(AETHER_LOG_INFO, "mainmenu", "Quit clicked");
}

/* ---- Wrapper callbacks that invoke user's handlers ---- */
typedef struct cb_wrap {
    void (*fn)(void *user);
    void  *user;
} cb_wrap_t;

static void wrap_new_game(void *u)   { cb_wrap_t *w = (cb_wrap_t*)u; if (w->fn) w->fn(w->user); }
static void wrap_load_game(void *u)  { cb_wrap_t *w = (cb_wrap_t*)u; if (w->fn) w->fn(w->user); }
static void wrap_options(void *u)    { cb_wrap_t *w = (cb_wrap_t*)u; if (w->fn) w->fn(w->user); }
static void wrap_multiplayer(void *u){ cb_wrap_t *w = (cb_wrap_t*)u; if (w->fn) w->fn(w->user); }
static void wrap_quit(void *u)       { cb_wrap_t *w = (cb_wrap_t*)u; if (w->fn) w->fn(w->user); }

/* ---- Setup ---- */
bool aether_main_menu_setup(aether_menu_t *menu,
                             const aether_main_menu_callbacks_t *cb) {
    if (!menu) return false;

    aether_menu_panel_t *main = aether_menu_get_panel(menu, AETHER_MENU_MAIN);
    if (!main) return false;
    main->item_count = 0;
    main->selected_item = 0;

    /* If cb provided, allocate wrappers and store them; else use defaults */
    static cb_wrap_t wrap_storage[5];

    if (cb) {
        wrap_storage[0] = (cb_wrap_t){ cb->on_new_game,    cb->user };
        wrap_storage[1] = (cb_wrap_t){ cb->on_load_game,   cb->user };
        wrap_storage[2] = (cb_wrap_t){ cb->on_options,     cb->user };
        wrap_storage[3] = (cb_wrap_t){ cb->on_multiplayer, cb->user };
        wrap_storage[4] = (cb_wrap_t){ cb->on_quit,        cb->user };

        aether_menu_add_button(main, "New Game",    wrap_new_game,    &wrap_storage[0]);
        aether_menu_add_button(main, "Load Game",   wrap_load_game,   &wrap_storage[1]);
        aether_menu_add_button(main, "Options",     wrap_options,     &wrap_storage[2]);
        aether_menu_add_button(main, "Multiplayer", wrap_multiplayer, &wrap_storage[3]);
        aether_menu_add_button(main, "Quit",        wrap_quit,        &wrap_storage[4]);
    } else {
        aether_menu_add_button(main, "New Game",    default_new_game,    NULL);
        aether_menu_add_button(main, "Load Game",   default_load_game,   NULL);
        aether_menu_add_button(main, "Options",     default_options,     NULL);
        aether_menu_add_button(main, "Multiplayer", default_multiplayer, NULL);
        aether_menu_add_button(main, "Quit",        default_quit,        NULL);
    }

    aether_log(AETHER_LOG_INFO, "mainmenu",
               "main menu setup with %u items", main->item_count);
    return true;
}

bool aether_main_menu_setup_default(aether_menu_t *menu) {
    return aether_main_menu_setup(menu, NULL);
}
