#ifndef AETHER_GAME_DLL_H
#define AETHER_GAME_DLL_H
#include "../../core/AetherCore.h"
#include "../../entity/AetherEntityBase.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_DLL_NAME_MAX 64
#define AETHER_DLL_MAX 32
typedef enum { AETHER_DLL_SERVER=0, AETHER_DLL_CLIENT=1 } aether_dll_kind_t;
typedef struct aether_game_dll aether_game_dll_t;
typedef bool (*aether_dll_init_fn)(aether_game_dll_t *dll);
typedef void (*aether_dll_shutdown_fn)(aether_game_dll_t *dll);
typedef void (*aether_dll_frame_fn)(aether_game_dll_t *dll, f32 dt);
typedef struct { const char *name; const char *version; aether_dll_kind_t kind; aether_dll_init_fn init; aether_dll_shutdown_fn shutdown; aether_dll_frame_fn frame; } aether_dll_desc_t;
struct aether_game_dll { aether_dll_desc_t desc; bool initialized; void *user_data; };
typedef struct { aether_game_dll_t items[AETHER_DLL_MAX]; u32 count; } aether_dll_registry_t;
void aether_dll_registry_init(aether_dll_registry_t *r);
bool aether_dll_register(aether_dll_registry_t *r, const aether_dll_desc_t *desc, void *user_data);
aether_game_dll_t *aether_dll_find(aether_dll_registry_t *r, const char *name, aether_dll_kind_t kind);
bool aether_dll_init_all(aether_dll_registry_t *r);
void aether_dll_frame_all(aether_dll_registry_t *r, f32 dt);
void aether_dll_shutdown_all(aether_dll_registry_t *r);
#ifdef __cplusplus
}
#endif
#endif
