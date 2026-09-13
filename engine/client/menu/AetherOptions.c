/* AetherOptions.c — Options menu implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherOptions.h"
#include <string.h>

bool aether_options_setup(aether_menu_t *menu, aether_settings_t *settings) {
    if (!menu) return false;

    aether_menu_panel_t *opts = aether_menu_get_panel(menu, AETHER_MENU_OPTIONS);
    if (!opts) return false;
    opts->item_count = 0;
    opts->selected_item = 0;

    /* --- VIDEO --- */
    aether_menu_add_label(opts, "--- VIDEO ---");
    aether_menu_add_slider(opts, "Brightness",  0.0f, 2.0f, 1.0f);
    aether_menu_add_slider(opts, "Gamma",       0.5f, 3.0f, 1.0f);
    aether_menu_add_toggle(opts, "VSync",       true);
    aether_menu_add_slider(opts, "FPS Limit",   30.0f, 240.0f, 120.0f);

    /* --- AUDIO --- */
    aether_menu_add_label(opts, "--- AUDIO ---");
    aether_menu_add_slider(opts, "Master Volume",  0.0f, 1.0f, 1.0f);
    aether_menu_add_slider(opts, "Music Volume",   0.0f, 1.0f, 0.7f);
    aether_menu_add_slider(opts, "Effects Volume", 0.0f, 1.0f, 1.0f);
    aether_menu_add_toggle(opts, "Mute",           false);

    /* --- CONTROLS --- */
    aether_menu_add_label(opts, "--- CONTROLS ---");
    aether_menu_add_slider(opts, "Look Sensitivity", 0.1f, 5.0f, 1.0f);
    aether_menu_add_toggle(opts, "Invert Y-Axis",    false);
    aether_menu_add_toggle(opts, "Left-handed",      false);

    /* --- TOUCH --- */
    aether_menu_add_label(opts, "--- TOUCH ---");
    aether_menu_add_slider(opts, "Touch Opacity",  0.2f, 1.0f, 0.75f);
    aether_menu_add_slider(opts, "Touch Scale",    0.5f, 2.0f, 1.0f);

    aether_log(AETHER_LOG_INFO, "options",
               "options panel setup with %u items", opts->item_count);

    if (settings) aether_options_load(menu, settings);
    return true;
}

/* Helper: find slider item by label */
static aether_menu_item_t *find_slider(aether_menu_panel_t *opts, const char *label) {
    if (!opts || !label) return NULL;
    for (u32 i = 0; i < opts->item_count; ++i) {
        if (opts->items[i].type == AETHER_MENU_ITEM_SLIDER &&
            aether_str_eq(opts->items[i].label, label)) return &opts->items[i];
    }
    return NULL;
}

static aether_menu_item_t *find_toggle(aether_menu_panel_t *opts, const char *label) {
    if (!opts || !label) return NULL;
    for (u32 i = 0; i < opts->item_count; ++i) {
        if (opts->items[i].type == AETHER_MENU_ITEM_TOGGLE &&
            aether_str_eq(opts->items[i].label, label)) return &opts->items[i];
    }
    return NULL;
}

void aether_options_load(aether_menu_t *menu, aether_settings_t *settings) {
    if (!menu || !settings) return;
    aether_menu_panel_t *opts = aether_menu_get_panel(menu, AETHER_MENU_OPTIONS);
    if (!opts) return;

    f32 f = 0.0f; i32 iv = 0; bool bv = false;

    if (aether_settings_get_float(settings, "s_master_volume", &f)) {
        aether_menu_item_t *s = find_slider(opts, "Master Volume");
        if (s) s->value = f;
    }
    if (aether_settings_get_float(settings, "s_music_volume", &f)) {
        aether_menu_item_t *s = find_slider(opts, "Music Volume");
        if (s) s->value = f;
    }
    if (aether_settings_get_float(settings, "s_effects_volume", &f)) {
        aether_menu_item_t *s = find_slider(opts, "Effects Volume");
        if (s) s->value = f;
    }
    if (aether_settings_get_bool(settings, "s_mute", &bv)) {
        aether_menu_item_t *t = find_toggle(opts, "Mute");
        if (t) t->toggle_state = bv;
    }
    if (aether_settings_get_int(settings, "r_fps_limit", &iv)) {
        aether_menu_item_t *s = find_slider(opts, "FPS Limit");
        if (s) s->value = (f32)iv;
    }
    if (aether_settings_get_bool(settings, "r_vsync", &bv)) {
        aether_menu_item_t *t = find_toggle(opts, "VSync");
        if (t) t->toggle_state = bv;
    }
    if (aether_settings_get_float(settings, "in_look_sensitivity", &f)) {
        aether_menu_item_t *s = find_slider(opts, "Look Sensitivity");
        if (s) s->value = f;
    }
    if (aether_settings_get_bool(settings, "in_invert_y", &bv)) {
        aether_menu_item_t *t = find_toggle(opts, "Invert Y-Axis");
        if (t) t->toggle_state = bv;
    }

    aether_log(AETHER_LOG_INFO, "options", "options loaded from settings");
}

void aether_options_save(aether_menu_t *menu, aether_settings_t *settings) {
    if (!menu || !settings) return;
    aether_menu_panel_t *opts = aether_menu_get_panel(menu, AETHER_MENU_OPTIONS);
    if (!opts) return;

    aether_menu_item_t *s;

    s = find_slider(opts, "Master Volume");
    if (s) aether_settings_set_float(settings, "s_master_volume", s->value);
    s = find_slider(opts, "Music Volume");
    if (s) aether_settings_set_float(settings, "s_music_volume", s->value);
    s = find_slider(opts, "Effects Volume");
    if (s) aether_settings_set_float(settings, "s_effects_volume", s->value);
    s = find_slider(opts, "FPS Limit");
    if (s) aether_settings_set_int(settings, "r_fps_limit", (i32)s->value);
    s = find_slider(opts, "Look Sensitivity");
    if (s) aether_settings_set_float(settings, "in_look_sensitivity", s->value);

    aether_menu_item_t *t;
    t = find_toggle(opts, "Mute");
    if (t) aether_settings_set_bool(settings, "s_mute", t->toggle_state);
    t = find_toggle(opts, "VSync");
    if (t) aether_settings_set_bool(settings, "r_vsync", t->toggle_state);
    t = find_toggle(opts, "Invert Y-Axis");
    if (t) aether_settings_set_bool(settings, "in_invert_y", t->toggle_state);

    aether_log(AETHER_LOG_INFO, "options", "options saved to settings");
}
