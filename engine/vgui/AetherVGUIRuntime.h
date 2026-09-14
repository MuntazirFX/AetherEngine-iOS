#ifndef AETHER_VGUI_RUNTIME_H
#define AETHER_VGUI_RUNTIME_H
#include "AetherVGUI.h"
#include "AetherVGUIScheme.h"
#include "AetherVGUILocalization.h"
#include "AetherVGUIServerBrowser.h"
#include "AetherVGUIConsole.h"
#ifdef __cplusplus
extern "C" {
#endif
aether_result_t aether_vgui_runtime_init(void);
void aether_vgui_runtime_shutdown(void);
aether_vgui_t *aether_vgui_runtime_ui(void);
aether_vgui_scheme_t *aether_vgui_runtime_scheme(void);
aether_vgui_localization_t *aether_vgui_runtime_localization(void);
aether_vgui_server_browser_t *aether_vgui_runtime_server_browser(void);
aether_vgui_console_t *aether_vgui_runtime_console(void);
void aether_vgui_runtime_show_main(void);
void aether_vgui_runtime_show_options(void);
void aether_vgui_runtime_show_load_game(void);
void aether_vgui_runtime_show_multiplayer(void);
void aether_vgui_runtime_toggle_console(void);
bool aether_vgui_runtime_console_visible(void);
aether_result_t aether_vgui_runtime_console_execute(const char *line);
u32 aether_vgui_runtime_console_count(void);
const aether_vgui_console_line_t *aether_vgui_runtime_console_line(u32 index);
aether_result_t aether_vgui_runtime_console_set_input(const char *text);
const char *aether_vgui_runtime_console_input(void);
u32 aether_vgui_runtime_active_panel(void);
#ifdef __cplusplus
}
#endif
#endif
