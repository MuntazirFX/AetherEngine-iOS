/* AetherMainMenu.h — G-Man style main menu with buttons.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MAIN_MENU_H
#define AETHER_MAIN_MENU_H

#include "AetherMenu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Callbacks for main menu actions */
typedef struct aether_main_menu_callbacks {
    void (*on_new_game)   (void *user);
    void (*on_load_game)  (void *user);
    void (*on_options)    (void *user);
    void (*on_multiplayer)(void *user);
    void (*on_quit)       (void *user);
    void *user;
} aether_main_menu_callbacks_t;

/* Create and register all main menu items into the given menu.
 * Returns true on success. */
bool aether_main_menu_setup(aether_menu_t *menu,
                             const aether_main_menu_callbacks_t *cb);

/* Trigger default callback wiring (logs only). Useful for testing. */
bool aether_main_menu_setup_default(aether_menu_t *menu);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MAIN_MENU_H */
