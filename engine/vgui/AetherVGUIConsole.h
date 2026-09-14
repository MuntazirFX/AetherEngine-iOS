#ifndef AETHER_VGUI_CONSOLE_H
#define AETHER_VGUI_CONSOLE_H
#include "../core/AetherCore.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_VGUI_CONSOLE_LINES 128
typedef struct { char text[1024]; aether_log_level_t level; } aether_vgui_console_line_t;
typedef struct { aether_vgui_console_line_t lines[AETHER_VGUI_CONSOLE_LINES]; u32 count; u32 scroll; bool visible; char input[1024]; } aether_vgui_console_t;
void aether_vgui_console_init(aether_vgui_console_t*);
void aether_vgui_console_write(aether_vgui_console_t*,aether_log_level_t,const char*);
aether_result_t aether_vgui_console_set_input(aether_vgui_console_t*,const char*);
const aether_vgui_console_line_t*aether_vgui_console_line(const aether_vgui_console_t*,u32 index);
#ifdef __cplusplus
}
#endif
#endif
