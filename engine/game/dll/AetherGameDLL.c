#include "AetherGameDLL.h"
#include <string.h>
void aether_dll_registry_init(aether_dll_registry_t *r){ if(r) memset(r,0,sizeof *r); }
bool aether_dll_register(aether_dll_registry_t *r,const aether_dll_desc_t *d,void *u){ if(!r||!d||!d->name||!d->init||r->count>=AETHER_DLL_MAX)return false; if(aether_dll_find(r,d->name,d->kind))return false; aether_game_dll_t *x=&r->items[r->count++]; memset(x,0,sizeof*x); x->desc=*d; x->user_data=u; return true; }
aether_game_dll_t *aether_dll_find(aether_dll_registry_t *r,const char*n,aether_dll_kind_t k){if(!r||!n)return NULL;for(u32 i=0;i<r->count;i++)if(r->items[i].desc.kind==k&&aether_str_eq(r->items[i].desc.name,n))return &r->items[i];return NULL;}
bool aether_dll_init_all(aether_dll_registry_t*r){if(!r)return false;for(u32 i=0;i<r->count;i++){aether_game_dll_t*x=&r->items[i];if(x->desc.init&&!x->desc.init(x))return false;x->initialized=true;}return true;}
void aether_dll_frame_all(aether_dll_registry_t*r,f32 dt){if(!r)return;for(u32 i=0;i<r->count;i++)if(r->items[i].initialized&&r->items[i].desc.frame)r->items[i].desc.frame(&r->items[i],dt);}
void aether_dll_shutdown_all(aether_dll_registry_t*r){if(!r)return;for(u32 i=r->count;i>0;i--){aether_game_dll_t*x=&r->items[i-1];if(x->initialized&&x->desc.shutdown)x->desc.shutdown(x);x->initialized=false;}}
