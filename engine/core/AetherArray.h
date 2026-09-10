/* AetherArray.h — Dynamic array (type-safe via macro) for AetherEngine.
 * Clean-room. Uses AetherCore types.
 */
#ifndef AETHER_ARRAY_H
#define AETHER_ARRAY_H

#include "AetherCore.h"
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Macro to declare a dynamic array of type T */
#define AETHER_ARRAY_DECLARE(T) \
    struct { T *data; size_t size; size_t capacity; }

/* Initialize array */
#define AETHER_ARRAY_INIT(arr, initial_cap) do { \
    (arr)->size = 0; \
    (arr)->capacity = (initial_cap) > 0 ? (initial_cap) : 4; \
    (arr)->data = (void*)malloc(sizeof(*(arr)->data) * (arr)->capacity); \
} while(0)

/* Push an item */
#define AETHER_ARRAY_PUSH(arr, item) do { \
    if ((arr)->size >= (arr)->capacity) { \
        (arr)->capacity *= 2; \
        (arr)->data = (void*)realloc((arr)->data, sizeof(*(arr)->data) * (arr)->capacity); \
    } \
    (arr)->data[(arr)->size++] = (item); \
} while(0)

/* Free array */
#define AETHER_ARRAY_FREE(arr) do { \
    free((arr)->data); \
    (arr)->data = NULL; \
    (arr)->size = 0; \
    (arr)->capacity = 0; \
} while(0)

#ifdef __cplusplus
}
#endif
#endif /* AETHER_ARRAY_H */
