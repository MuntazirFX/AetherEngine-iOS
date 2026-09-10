#include "AetherCore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>

static aether_log_fn g_log_fn  = NULL;
static void         *g_log_user = NULL;

void aether_log_set(aether_log_fn fn, void *user) {
    g_log_fn  = fn;
    g_log_user = user;
}

static void default_log(aether_log_level_t lvl, const char *tag, const char *msg) {
    static const char *names[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR" };
    const char *n = ((int)lvl >= 0 && (int)lvl < 5) ? names[(int)lvl] : "?";
    fprintf(stderr, "[Aether/%s][%s] %s\n", n, tag ? tag : "-", msg ? msg : "");
}

void aether_log(aether_log_level_t lvl, const char *tag, const char *fmt, ...) {
    char buf[1024];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (g_log_fn) g_log_fn(lvl, tag, buf, g_log_user);
    else          default_log(lvl, tag, buf);
}

const char *aether_result_string(aether_result_t r) {
    switch (r) {
        case AETHER_OK:              return "OK";
        case AETHER_ERR_GENERIC:     return "GENERIC";
        case AETHER_ERR_INVALID_ARG: return "INVALID_ARG";
        case AETHER_ERR_OUT_OF_MEM:  return "OUT_OF_MEM";
        case AETHER_ERR_NOT_FOUND:   return "NOT_FOUND";
        case AETHER_ERR_IO:          return "IO";
        case AETHER_ERR_UNSUPPORTED: return "UNSUPPORTED";
        case AETHER_ERR_NOT_READY:   return "NOT_READY";
        case AETHER_ERR_ALREADY:     return "ALREADY";
        case AETHER_ERR_STATE:       return "STATE";
        default:                     return "UNKNOWN";
    }
}

static size_t align_up(size_t v, size_t a) {
    return (v + (a - 1)) & ~(a - 1);
}

aether_result_t aether_arena_init(aether_arena_t *a, size_t capacity_bytes) {
    if (!a || capacity_bytes == 0) return AETHER_ERR_INVALID_ARG;
    u8 *base = (u8*)malloc(capacity_bytes + 64);
    if (!base) return AETHER_ERR_OUT_OF_MEM;
    a->base = base;
    a->cap  = capacity_bytes;
    a->used = 0;
    return AETHER_OK;
}

void aether_arena_destroy(aether_arena_t *a) {
    if (!a) return;
    free(a->base);
    a->base = NULL; a->cap = 0; a->used = 0;
}

void *aether_arena_alloc(aether_arena_t *a, size_t size, size_t align) {
    if (!a || size == 0) return NULL;
    if (align == 0) align = 1;
    size_t off = align_up(a->used, align);
    if (off + size > a->cap) {
        aether_log(AETHER_LOG_ERROR, "arena", "out of memory: need=%zu used=%zu cap=%zu", size, a->used, a->cap);
        return NULL;
    }
    void *p = a->base + off;
    a->used = off + size;
    return p;
}

void aether_arena_reset(aether_arena_t *a) { if (a) a->used = 0; }
size_t aether_arena_used(const aether_arena_t *a)     { return a ? a->used : 0; }
size_t aether_arena_capacity(const aether_arena_t *a) { return a ? a->cap  : 0; }

size_t aether_str_copy(char *dst, size_t cap, const char *src) {
    if (!dst || !src || cap == 0) return 0;
    size_t n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n); dst[n] = '\0';
    return n;
}

bool aether_str_eq(const char *a, const char *b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

u32 aether_hash32(const char *s) {
    u32 h = 2166136261u;
    if (!s) return h;
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        h ^= (u32)(*p); h *= 16777619u;
    }
    return h;
}

bool aether_file_exists(const char *path) {
    if (!path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

bool aether_dir_exists(const char *path) {
    if (!path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}
