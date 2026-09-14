#ifndef AETHER_CVAR_H
#define AETHER_CVAR_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_CVAR_NAME_MAX 64
#define AETHER_CVAR_STRING_MAX 256
#define AETHER_CVAR_MAX 512

typedef enum aether_cvar_type {
    AETHER_CVAR_BOOL = 0,
    AETHER_CVAR_INT,
    AETHER_CVAR_FLOAT,
    AETHER_CVAR_STRING
} aether_cvar_type_t;

typedef enum aether_cvar_flags {
    AETHER_CVAR_NONE = 0,
    AETHER_CVAR_ARCHIVE = 1u << 0,
    AETHER_CVAR_READONLY = 1u << 1,
    AETHER_CVAR_CHEAT = 1u << 2,
    AETHER_CVAR_USERINFO = 1u << 3,
    AETHER_CVAR_SERVERINFO = 1u << 4
} aether_cvar_flags_t;

typedef struct aether_cvar {
    char name[AETHER_CVAR_NAME_MAX];
    char string[AETHER_CVAR_STRING_MAX];
    char default_string[AETHER_CVAR_STRING_MAX];
    aether_cvar_type_t type;
    u32 flags;
    bool registered;
} aether_cvar_t;

typedef struct aether_cvar_registry aether_cvar_registry_t;

aether_cvar_registry_t *aether_cvar_create(void);
void aether_cvar_destroy(aether_cvar_registry_t *r);

aether_cvar_t *aether_cvar_register(aether_cvar_registry_t *r, const char *name,
                                     aether_cvar_type_t type, const char *default_value,
                                     u32 flags);
aether_cvar_t *aether_cvar_find(aether_cvar_registry_t *r, const char *name);
const aether_cvar_t *aether_cvar_find_const(const aether_cvar_registry_t *r, const char *name);

aether_result_t aether_cvar_set(aether_cvar_registry_t *r, const char *name, const char *value);
const char *aether_cvar_string(const aether_cvar_registry_t *r, const char *name);
bool aether_cvar_bool(const aether_cvar_registry_t *r, const char *name, bool fallback);
i32 aether_cvar_int(const aether_cvar_registry_t *r, const char *name, i32 fallback);
f32 aether_cvar_float(const aether_cvar_registry_t *r, const char *name, f32 fallback);
void aether_cvar_reset(aether_cvar_registry_t *r, const char *name);
void aether_cvar_reset_all(aether_cvar_registry_t *r);
u32 aether_cvar_count(const aether_cvar_registry_t *r);
const aether_cvar_t *aether_cvar_at(const aether_cvar_registry_t *r, u32 index);

#ifdef __cplusplus
}
#endif
#endif
