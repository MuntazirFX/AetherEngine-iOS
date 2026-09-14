#ifndef AETHER_THREAD_H
#define AETHER_THREAD_H
#include "../core/AetherCore.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct aether_thread aether_thread_t;
typedef struct aether_mutex aether_mutex_t;
typedef struct aether_cond aether_cond_t;
typedef void *(*aether_thread_fn)(void *user);
aether_result_t aether_thread_start(aether_thread_t **out,aether_thread_fn fn,void*user,const char*name);
aether_result_t aether_thread_join(aether_thread_t*t,void**result);
void aether_thread_destroy(aether_thread_t*t);
u64 aether_thread_id(void);
void aether_thread_sleep_ms(u32 ms);
aether_mutex_t *aether_mutex_create(void);void aether_mutex_destroy(aether_mutex_t*m);void aether_mutex_lock(aether_mutex_t*m);void aether_mutex_unlock(aether_mutex_t*m);bool aether_mutex_try_lock(aether_mutex_t*m);
aether_cond_t *aether_cond_create(void);void aether_cond_destroy(aether_cond_t*c);void aether_cond_wait(aether_cond_t*c,aether_mutex_t*m);bool aether_cond_timed_wait_ms(aether_cond_t*c,aether_mutex_t*m,u32 ms);void aether_cond_signal(aether_cond_t*c);void aether_cond_broadcast(aether_cond_t*c);
#ifdef __cplusplus
}
#endif
#endif
