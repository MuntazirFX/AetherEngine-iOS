/* AetherEvent.h — Event bus for decoupled communication between engine modules.
 * Clean-room implementation.
 */
#ifndef AETHER_EVENT_H
#define AETHER_EVENT_H

#include "AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MAX_EVENT_HANDLERS 64

typedef enum aether_event_type {
    AETHER_EVENT_NONE = 0,
    AETHER_EVENT_ENGINE_INIT,
    AETHER_EVENT_ENGINE_SHUTDOWN,
    AETHER_EVENT_GAME_SELECTED,
    AETHER_EVENT_GAME_STARTED,
    AETHER_EVENT_GAME_STOPPED,
    AETHER_EVENT_SETTINGS_CHANGED,
    AETHER_EVENT_TOUCH_INPUT,
    AETHER_EVENT_CONTROLLER_INPUT,
    AETHER_EVENT_COUNT
} aether_event_type_t;

typedef struct aether_event {
    aether_event_type_t type;
    void               *data;      /* Optional payload */
    u32                 data_size; /* Size of payload */
} aether_event_t;

typedef void (*aether_event_handler_fn)(const aether_event_t *ev, void *user);

typedef struct aether_event_bus aether_event_bus_t;

aether_event_bus_t *aether_event_bus_create(void);
void                aether_event_bus_destroy(aether_event_bus_t *bus);

/* Subscribe to an event type. Returns a handle (index) or -1 on error. */
i32  aether_event_subscribe(aether_event_bus_t *bus, aether_event_type_t type,
                            aether_event_handler_fn handler, void *user);

/* Unsubscribe using the handle returned by subscribe. */
void aether_event_unsubscribe(aether_event_bus_t *bus, i32 handle);

/* Publish an event to all subscribers. */
void aether_event_publish(aether_event_bus_t *bus, const aether_event_t *ev);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_EVENT_H */
