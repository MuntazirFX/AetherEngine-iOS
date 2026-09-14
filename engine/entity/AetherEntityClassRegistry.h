#ifndef AETHER_ENTITY_CLASS_REGISTRY_H
#define AETHER_ENTITY_CLASS_REGISTRY_H
#include "AetherEntityBase.h"
#ifdef __cplusplus
extern "C" {
#endif
#define AETHER_ENTITY_CLASS_MAX 512
typedef struct { char classname[AETHER_ENTITY_CLASSNAME_MAX]; aether_spawn_fn spawn; aether_think_fn think; aether_use_fn use; aether_touch_fn touch; aether_damage_fn damage; } aether_entity_class_t;
typedef struct { aether_entity_class_t classes[AETHER_ENTITY_CLASS_MAX]; u32 count; } aether_entity_class_registry_t;
void aether_entity_class_registry_init(aether_entity_class_registry_t*r);
bool aether_entity_class_register(aether_entity_class_registry_t*r,const char*classname,aether_spawn_fn spawn,aether_think_fn think,aether_use_fn use,aether_touch_fn touch,aether_damage_fn damage);
const aether_entity_class_t*aether_entity_class_find(const aether_entity_class_registry_t*r,const char*classname);
u32 aether_entity_class_register_builtin(aether_entity_class_registry_t*r);
bool aether_entity_class_bind(aether_entity_class_registry_t*r,aether_entity_t*e);
#ifdef __cplusplus
}
#endif
#endif
