/* AetherOptions.h — Options menu (video, audio, controls, touch).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_OPTIONS_H
#define AETHER_OPTIONS_H

#include "AetherMenu.h"
#include "../../config/AetherSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Options menu uses a settings store (borrowed pointer). */
typedef struct aether_options_ctx {
    aether_settings_t *settings;
} aether_options_ctx_t;

/* Setup all option panels (video, audio, controls) into the given menu.
 * Returns true on success. */
bool aether_options_setup(aether_menu_t *menu, aether_settings_t *settings);

/* Save current option values back to settings store. */
void aether_options_save(aether_menu_t *menu, aether_settings_t *settings);

/* Load settings into option widgets. */
void aether_options_load(aether_menu_t *menu, aether_settings_t *settings);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_OPTIONS_H */
