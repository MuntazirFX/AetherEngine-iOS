#include "AetherCVar.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

struct aether_cvar_registry { aether_cvar_t items[AETHER_CVAR_MAX]; u32 count; };

static aether_cvar_t *find(aether_cvar_registry_t *r, const char *name) {
    if (!r || !name) return NULL;
    for (u32 i = 0; i < r->count; ++i)
        if (r->items[i].registered && aether_str_eq(r->items[i].name, name)) return &r->items[i];
    return NULL;
}

static bool valid_name(const char *s) {
    if (!s || !*s) return false;
    for (; *s; ++s) if (isspace((unsigned char)*s) || *s == '"') return false;
    return true;
}

aether_cvar_registry_t *aether_cvar_create(void) { return (aether_cvar_registry_t *)calloc(1, sizeof(aether_cvar_registry_t)); }
void aether_cvar_destroy(aether_cvar_registry_t *r) { free(r); }

aether_cvar_t *aether_cvar_register(aether_cvar_registry_t *r, const char *name, aether_cvar_type_t type,
                                     const char *default_value, u32 flags) {
    if (!r || !valid_name(name) || r->count >= AETHER_CVAR_MAX) return NULL;
    aether_cvar_t *old = find(r, name);
    if (old) return old;
    aether_cvar_t *v = &r->items[r->count++];
    memset(v, 0, sizeof *v);
    aether_str_copy(v->name, sizeof v->name, name);
    aether_str_copy(v->default_string, sizeof v->default_string, default_value ? default_value : "");
    aether_str_copy(v->string, sizeof v->string, default_value ? default_value : "");
    v->type = type; v->flags = flags; v->registered = true;
    return v;
}

aether_cvar_t *aether_cvar_find(aether_cvar_registry_t *r, const char *name) { return find(r, name); }
const aether_cvar_t *aether_cvar_find_const(const aether_cvar_registry_t *r, const char *name) { return find((aether_cvar_registry_t *)r, name); }

aether_result_t aether_cvar_set(aether_cvar_registry_t *r, const char *name, const char *value) {
    aether_cvar_t *v = find(r, name);
    if (!v || !value) return AETHER_ERR_NOT_FOUND;
    if (v->flags & AETHER_CVAR_READONLY) return AETHER_ERR_STATE;
    if (v->type == AETHER_CVAR_BOOL) {
        if (aether_str_eq(value,"1") || aether_str_eq(value,"true") || aether_str_eq(value,"on"))
            aether_str_copy(v->string,sizeof v->string,"1");
        else if (aether_str_eq(value,"0") || aether_str_eq(value,"false") || aether_str_eq(value,"off"))
            aether_str_copy(v->string,sizeof v->string,"0");
        else return AETHER_ERR_INVALID_ARG;
    } else {
        aether_str_copy(v->string, sizeof v->string, value);
    }
    return AETHER_OK;
}

const char *aether_cvar_string(const aether_cvar_registry_t *r, const char *name) { const aether_cvar_t *v=find((aether_cvar_registry_t*)r,name); return v?v->string:NULL; }
bool aether_cvar_bool(const aether_cvar_registry_t *r,const char *name,bool fallback){const char*s=aether_cvar_string(r,name);if(!s)return fallback;return aether_str_eq(s,"1")||aether_str_eq(s,"true")||aether_str_eq(s,"on");}
i32 aether_cvar_int(const aether_cvar_registry_t *r,const char *name,i32 fallback){const char*s=aether_cvar_string(r,name);if(!s)return fallback;char*e=NULL;long v=strtol(s,&e,10);return(e&&*e=='\0')?(i32)v:fallback;}
f32 aether_cvar_float(const aether_cvar_registry_t *r,const char *name,f32 fallback){const char*s=aether_cvar_string(r,name);if(!s)return fallback;char*e=NULL;float v=strtof(s,&e);return(e&&*e=='\0')?v:fallback;}
void aether_cvar_reset(aether_cvar_registry_t *r,const char *name){aether_cvar_t*v=find(r,name);if(v && !(v->flags&AETHER_CVAR_READONLY))aether_str_copy(v->string,sizeof v->string,v->default_string);}
void aether_cvar_reset_all(aether_cvar_registry_t *r){if(!r)return;for(u32 i=0;i<r->count;++i)aether_cvar_reset(r,r->items[i].name);}
u32 aether_cvar_count(const aether_cvar_registry_t*r){return r?r->count:0;}
const aether_cvar_t*aether_cvar_at(const aether_cvar_registry_t*r,u32 i){return(r&&i<r->count)?&r->items[i]:NULL;}
