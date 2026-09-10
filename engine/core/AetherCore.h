/* AetherCore.h — Foundation types, arena allocator, logging, error codes.
 * AetherEngine-iOS · Clean-room. No XashFusion / xash3d-fwgs code reused.
 * Language: C11 · Targets: ARM64 (iOS), x86_64 (host tools).
 */
#ifndef AETHER_CORE_H
#define AETHER_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Version ---------- */
#define AETHER_VERSION_MAJOR 0
#define AETHER_VERSION_MINOR 1
#define AETHER_VERSION_PATCH 0
#define AETHER_VERSION_STRING "0.1.0-step1"

/* ---------- Fixed-width types ---------- */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef float     f32;
typedef double    f64;

/* ---------- Result codes ---------- */
typedef enum aether_result {
    AETHER_OK              =  0,
    AETHER_ERR_GENERIC     = -1,
    AETHER_ERR_INVALID_ARG = -2,
    AETHER_ERR_OUT_OF_MEM  = -3,
    AETHER_ERR_NOT_FOUND   = -4,
    AETHER_ERR_IO          = -5,
    AETHER_ERR_UNSUPPORTED = -6,
    AETHER_ERR_NOT_READY   = -7,
    AETHER_ERR_ALREADY     = -8,
    AETHER_ERR_STATE       = -9,
} aether_result_t;

const char *aether_result_string(aether_result_t r);

/* ---------- Logging ---------- */
typedef enum aether_log_level {
    AETHER_LOG_TRACE = 0,
    AETHER_LOG_DEBUG,
    AETHER_LOG_INFO,
    AETHER_LOG_WARN,
    AETHER_LOG_ERROR
} aether_log_level_t;

typedef void (*aether_log_fn)(aether_log_level_t lvl,
                              const char *tag,
                              const char *msg,
                              void *user);

void aether_log_set(aether_log_fn fn, void *user);
void aether_log(aether_log_level_t lvl, const char *tag, const char *fmt, ...);

/* ---------- Arena allocator (bump allocator, aligned) ---------- */
typedef struct aether_arena {
    u8     *base;
    size_t  cap;
    size_t  used;
} aether_arena_t;

aether_result_t aether_arena_init(aether_arena_t *a, size_t capacity_bytes);
void            aether_arena_destroy(aether_arena_t *a);
void           *aether_arena_alloc(aether_arena_t *a, size_t size, size_t align);
void            aether_arena_reset(aether_arena_t *a);
size_t          aether_arena_used(const aether_arena_t *a);
size_t          aether_arena_capacity(const aether_arena_t *a);

/* Convenience macros */
#define AETHER_ARENA_ALLOC(a, T, n) \
    ((T*)aether_arena_alloc((a), sizeof(T) * (size_t)(n), _Alignof(T)))

/* ---------- String helpers ---------- */
size_t aether_str_copy(char *dst, size_t cap, const char *src);   /* returns bytes copied excl. NUL */
bool   aether_str_eq(const char *a, const char *b);
u32    aether_hash32(const char *s);                                /* FNV-1a */

/* ---------- File utilities (thin, portable) ---------- */
bool aether_file_exists(const char *path);
bool aether_dir_exists(const char *path);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_CORE_H */
