#include "AetherEvent.h"
#include <stdlib.h>
#include <string.h>

typedef struct aether_event_sub {
    aether_event_type_t       type;
    aether_event_handler_fn   handler;
    void                     *user;
    bool                      active;
} aether_event_sub_t;

struct aether_event_bus {
    aether_event_sub_t subs[AETHER_MAX_EVENT_HANDLERS];
    u32                count;
};

aether_event_bus_t *aether_event_bus_create(void) {
    aether_event_bus_t *bus = (aether_event_bus_t*)calloc(1, sizeof *bus);
    if (bus) {
        aether_log(AETHER_LOG_INFO, "event", "event bus created (max %d handlers)", AETHER_MAX_EVENT_HANDLERS);
    }
    return bus;
}

void aether_event_bus_destroy(aether_event_bus_t *bus) {
    if (!bus) return;
    free(bus);
    aether_log(AETHER_LOG_INFO, "event", "event bus destroyed");
}

i32 aether_event_subscribe(aether_event_bus_t *bus, aether_event_type_t type,
                           aether_event_handler_fn handler, void *user) {
    if (!bus || !handler || type >= AETHER_EVENT_COUNT) return -1;

    /* Find a free slot */
    for (u32 i = 0; i < AETHER_MAX_EVENT_HANDLERS; ++i) {
        if (!bus->subs[i].active) {
            bus->subs[i].type    = type;
            bus->subs[i].handler = handler;
            bus->subs[i].user    = user;
            bus->subs[i].active  = true;
            if (i >= bus->count) bus->count = i + 1;
            return (i32)i;
        }
    }
    aether_log(AETHER_LOG_WARN, "event", "subscription failed: max handlers reached");
    return -1;
}

void aether_event_unsubscribe(aether_event_bus_t *bus, i32 handle) {
    if (!bus || handle < 0 || handle >= (i32)AETHER_MAX_EVENT_HANDLERS) return;
    bus->subs[handle].active = false;
}

void aether_event_publish(aether_event_bus_t *bus, const aether_event_t *ev) {
    if (!bus || !ev) return;
    for (u32 i = 0; i < bus->count; ++i) {
        aether_event_sub_t *sub = &bus->subs[i];
        if (sub->active && sub->type == ev->type) {
            sub->handler(ev, sub->user);
        }
    }
}
