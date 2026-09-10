#include "AetherSettings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct aether_setting {
    char                  key[AETHER_SETTING_KEY_MAX];
    aether_setting_type_t type;
    bool                  used;

    bool                  b_val;
    i32                   i_val;
    f32                   f_val;
    char                  s_val[AETHER_SETTING_STR_MAX];
} aether_setting_t;

struct aether_settings {
    aether_setting_t items[AETHER_MAX_SETTINGS];
    u32              count;
};

aether_settings_t *aether_settings_create(void) {
    aether_settings_t *s = (aether_settings_t*)calloc(1, sizeof *s);
    if (s) aether_log(AETHER_LOG_INFO, "settings", "settings store created");
    return s;
}

void aether_settings_destroy(aether_settings_t *s) {
    if (!s) return;
    free(s);
}

static aether_setting_t *find_setting(aether_settings_t *s, const char *key) {
    if (!s || !key) return NULL;
    for (u32 i = 0; i < s->count; ++i) {
        if (s->items[i].used && aether_str_eq(s->items[i].key, key))
            return &s->items[i];
    }
    return NULL;
}

static aether_setting_t *new_setting(aether_settings_t *s, const char *key,
                                     aether_setting_type_t type) {
    if (!s || !key) return NULL;
    if (s->count >= AETHER_MAX_SETTINGS) return NULL;
    aether_setting_t *it = &s->items[s->count++];
    aether_str_copy(it->key, sizeof it->key, key);
    it->type = type;
    it->used = true;
    return it;
}

/* ---- Registration ---- */
aether_result_t aether_settings_register_bool(aether_settings_t *s, const char *key, bool def) {
    aether_setting_t *it = new_setting(s, key, AETHER_SETTING_BOOL);
    if (!it) return AETHER_ERR_OUT_OF_MEM;
    it->b_val = def;
    return AETHER_OK;
}
aether_result_t aether_settings_register_int(aether_settings_t *s, const char *key, i32 def) {
    aether_setting_t *it = new_setting(s, key, AETHER_SETTING_INT);
    if (!it) return AETHER_ERR_OUT_OF_MEM;
    it->i_val = def;
    return AETHER_OK;
}
aether_result_t aether_settings_register_float(aether_settings_t *s, const char *key, f32 def) {
    aether_setting_t *it = new_setting(s, key, AETHER_SETTING_FLOAT);
    if (!it) return AETHER_ERR_OUT_OF_MEM;
    it->f_val = def;
    return AETHER_OK;
}
aether_result_t aether_settings_register_string(aether_settings_t *s, const char *key, const char *def) {
    aether_setting_t *it = new_setting(s, key, AETHER_SETTING_STRING);
    if (!it) return AETHER_ERR_OUT_OF_MEM;
    aether_str_copy(it->s_val, sizeof it->s_val, def ? def : "");
    return AETHER_OK;
}

/* ---- Get ---- */
bool aether_settings_get_bool(const aether_settings_t *s, const char *key, bool *out) {
    aether_setting_t *it = find_setting((aether_settings_t*)s, key);
    if (!it || it->type != AETHER_SETTING_BOOL) return false;
    if (out) *out = it->b_val;
    return true;
}
bool aether_settings_get_int(const aether_settings_t *s, const char *key, i32 *out) {
    aether_setting_t *it = find_setting((aether_settings_t*)s, key);
    if (!it || it->type != AETHER_SETTING_INT) return false;
    if (out) *out = it->i_val;
    return true;
}
bool aether_settings_get_float(const aether_settings_t *s, const char *key, f32 *out) {
    aether_setting_t *it = find_setting((aether_settings_t*)s, key);
    if (!it || it->type != AETHER_SETTING_FLOAT) return false;
    if (out) *out = it->f_val;
    return true;
}
bool aether_settings_get_string(const aether_settings_t *s, const char *key, const char **out) {
    aether_setting_t *it = find_setting((aether_settings_t*)s, key);
    if (!it || it->type != AETHER_SETTING_STRING) return false;
    if (out) *out = it->s_val;
    return true;
}

/* ---- Set ---- */
aether_result_t aether_settings_set_bool(aether_settings_t *s, const char *key, bool v) {
    aether_setting_t *it = find_setting(s, key);
    if (!it || it->type != AETHER_SETTING_BOOL) return AETHER_ERR_NOT_FOUND;
    it->b_val = v; return AETHER_OK;
}
aether_result_t aether_settings_set_int(aether_settings_t *s, const char *key, i32 v) {
    aether_setting_t *it = find_setting(s, key);
    if (!it || it->type != AETHER_SETTING_INT) return AETHER_ERR_NOT_FOUND;
    it->i_val = v; return AETHER_OK;
}
aether_result_t aether_settings_set_float(aether_settings_t *s, const char *key, f32 v) {
    aether_setting_t *it = find_setting(s, key);
    if (!it || it->type != AETHER_SETTING_FLOAT) return AETHER_ERR_NOT_FOUND;
    it->f_val = v; return AETHER_OK;
}
aether_result_t aether_settings_set_string(aether_settings_t *s, const char *key, const char *v) {
    aether_setting_t *it = find_setting(s, key);
    if (!it || it->type != AETHER_SETTING_STRING) return AETHER_ERR_NOT_FOUND;
    aether_str_copy(it->s_val, sizeof it->s_val, v ? v : "");
    return AETHER_OK;
}

/* ---- Persistence ---- */
aether_result_t aether_settings_save(const aether_settings_t *s, const char *filepath) {
    if (!s || !filepath) return AETHER_ERR_INVALID_ARG;
    FILE *f = fopen(filepath, "w");
    if (!f) return AETHER_ERR_IO;

    for (u32 i = 0; i < s->count; ++i) {
        const aether_setting_t *it = &s->items[i];
        if (!it->used) continue;
        switch (it->type) {
            case AETHER_SETTING_BOOL:
                fprintf(f, "%s=%d\n", it->key, it->b_val ? 1 : 0); break;
            case AETHER_SETTING_INT:
                fprintf(f, "%s=%d\n", it->key, (int)it->i_val); break;
            case AETHER_SETTING_FLOAT:
                fprintf(f, "%s=%.6f\n", it->key, (double)it->f_val); break;
            case AETHER_SETTING_STRING:
                fprintf(f, "%s=%s\n", it->key, it->s_val); break;
        }
    }
    fclose(f);
    aether_log(AETHER_LOG_INFO, "settings", "saved %u settings to %s", s->count, filepath);
    return AETHER_OK;
}

aether_result_t aether_settings_load(aether_settings_t *s, const char *filepath) {
    if (!s || !filepath) return AETHER_ERR_INVALID_ARG;
    FILE *f = fopen(filepath, "r");
    if (!f) return AETHER_ERR_NOT_FOUND;

    char line[256];
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        /* Strip trailing newline */
        char *nl = strchr(val, '\n'); if (nl) *nl = '\0';

        aether_setting_t *it = find_setting(s, key);
        if (!it) continue;

        switch (it->type) {
            case AETHER_SETTING_BOOL:  it->b_val = (atoi(val) != 0); break;
            case AETHER_SETTING_INT:   it->i_val = atoi(val); break;
            case AETHER_SETTING_FLOAT: it->f_val = (f32)atof(val); break;
            case AETHER_SETTING_STRING: aether_str_copy(it->s_val, sizeof it->s_val, val); break;
        }
    }
    fclose(f);
    aether_log(AETHER_LOG_INFO, "settings", "loaded settings from %s", filepath);
    return AETHER_OK;
}

/* ---- Engine defaults ---- */
void aether_settings_register_engine_defaults(aether_settings_t *s) {
    if (!s) return;

    /* Graphics */
    aether_settings_register_int  (s, "r_fps_limit",        120);
    aether_settings_register_int  (s, "r_width",            0);
    aether_settings_register_int  (s, "r_height",           0);
    aether_settings_register_bool (s, "r_vsync",            true);
    aether_settings_register_string(s, "r_backend",         "metal");

    /* Audio */
    aether_settings_register_float(s, "s_master_volume",    1.0f);
    aether_settings_register_float(s, "s_music_volume",     0.7f);
    aether_settings_register_float(s, "s_effects_volume",   1.0f);
    aether_settings_register_bool (s, "s_mute",             false);

    /* Touch */
    aether_settings_register_int  (s, "touch_layout",       0); /* 0=RH, 1=LH */
    aether_settings_register_float(s, "touch_opacity",      0.75f);
    aether_settings_register_float(s, "touch_scale",        1.0f);
    aether_settings_register_bool (s, "touch_show_joystick", true);

    /* Input / Controller */
    aether_settings_register_float(s, "in_look_sensitivity", 1.0f);
    aether_settings_register_bool (s, "in_invert_y",         false);
    aether_settings_register_float(s, "ctrl_deadzone",       0.15f);

    /* Performance */
    aether_settings_register_bool (s, "perf_show_fps",       false);
    aether_settings_register_int  (s, "perf_target_hz",      120);

    aether_log(AETHER_LOG_INFO, "settings", "engine defaults registered");
}
