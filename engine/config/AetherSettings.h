/* AetherSettings.h — Typed key/value settings store for AetherEngine.
 * Foundation for Graphics/Audio/Touch/Input/Controller/Performance settings.
 */
#ifndef AETHER_SETTINGS_H
#define AETHER_SETTINGS_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MAX_SETTINGS      128
#define AETHER_SETTING_KEY_MAX   64
#define AETHER_SETTING_STR_MAX   128

typedef enum aether_setting_type {
    AETHER_SETTING_BOOL = 0,
    AETHER_SETTING_INT,
    AETHER_SETTING_FLOAT,
    AETHER_SETTING_STRING,
} aether_setting_type_t;

typedef struct aether_settings aether_settings_t;

aether_settings_t *aether_settings_create(void);
void               aether_settings_destroy(aether_settings_t *s);

/* ---- Registration (defaults) ---- */
aether_result_t aether_settings_register_bool  (aether_settings_t *s, const char *key, bool def);
aether_result_t aether_settings_register_int   (aether_settings_t *s, const char *key, i32 def);
aether_result_t aether_settings_register_float (aether_settings_t *s, const char *key, f32 def);
aether_result_t aether_settings_register_string(aether_settings_t *s, const char *key, const char *def);

/* ---- Get / Set ---- */
bool         aether_settings_get_bool  (const aether_settings_t *s, const char *key, bool  *out);
bool         aether_settings_get_int   (const aether_settings_t *s, const char *key, i32   *out);
bool         aether_settings_get_float (const aether_settings_t *s, const char *key, f32   *out);
bool         aether_settings_get_string(const aether_settings_t *s, const char *key, const char **out);

aether_result_t aether_settings_set_bool  (aether_settings_t *s, const char *key, bool v);
aether_result_t aether_settings_set_int   (aether_settings_t *s, const char *key, i32 v);
aether_result_t aether_settings_set_float (aether_settings_t *s, const char *key, f32 v);
aether_result_t aether_settings_set_string(aether_settings_t *s, const char *key, const char *v);

/* ---- Persistence (simple key=value text format) ---- */
aether_result_t aether_settings_save(const aether_settings_t *s, const char *filepath);
aether_result_t aether_settings_load(aether_settings_t *s, const char *filepath);

/* ---- Register the standard set of engine defaults ---- */
void aether_settings_register_engine_defaults(aether_settings_t *s);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_SETTINGS_H */
