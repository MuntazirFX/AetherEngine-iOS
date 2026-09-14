#include "AetherThread.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
struct aether_thread{pthread_t handle;bool joined;};
struct aether_mutex{pthread_mutex_t m;};
struct aether_cond{pthread_cond_t c;};
aether_result_t aether_thread_start(aether_thread_t**out,aether_thread_fn fn,void*u,const char*n){
    if(!out||!fn)return AETHER_ERR_INVALID_ARG;
    aether_thread_t*t=calloc(1,sizeof*t);if(!t)return AETHER_ERR_OUT_OF_MEM;
    int e=pthread_create(&t->handle,NULL,fn,u);
    (void)n;
    if(e){free(t);return AETHER_ERR_GENERIC;}
    *out=t;return AETHER_OK;
}
aether_result_t aether_thread_join(aether_thread_t*t,void**r){if(!t||t->joined)return AETHER_ERR_INVALID_ARG;int e=pthread_join(t->handle,r);if(!e)t->joined=true;return e?AETHER_ERR_GENERIC:AETHER_OK;}
void aether_thread_destroy(aether_thread_t*t){if(!t)return;if(!t->joined)pthread_detach(t->handle);free(t);}
u64 aether_thread_id(void){pthread_t t=pthread_self();u64 v=0;memcpy(&v,&t,sizeof(t)<sizeof(v)?sizeof(t):sizeof(v));return v;}
void aether_thread_sleep_ms(u32 ms){struct timeval tv={(long)(ms/1000),(long)(ms%1000)*1000L};(void)select(0,NULL,NULL,NULL,&tv);}
aether_mutex_t*aether_mutex_create(void){aether_mutex_t*m=calloc(1,sizeof*m);if(m&&pthread_mutex_init(&m->m,NULL)!=0){free(m);return NULL;}return m;}
void aether_mutex_destroy(aether_mutex_t*m){if(m){pthread_mutex_destroy(&m->m);free(m);}}
void aether_mutex_lock(aether_mutex_t*m){if(m)pthread_mutex_lock(&m->m);}void aether_mutex_unlock(aether_mutex_t*m){if(m)pthread_mutex_unlock(&m->m);}bool aether_mutex_try_lock(aether_mutex_t*m){return m&&pthread_mutex_trylock(&m->m)==0;}
aether_cond_t*aether_cond_create(void){aether_cond_t*c=calloc(1,sizeof*c);if(c&&pthread_cond_init(&c->c,NULL)!=0){free(c);return NULL;}return c;}
void aether_cond_destroy(aether_cond_t*c){if(c){pthread_cond_destroy(&c->c);free(c);}}
void aether_cond_wait(aether_cond_t*c,aether_mutex_t*m){if(c&&m)pthread_cond_wait(&c->c,&m->m);}
bool aether_cond_timed_wait_ms(aether_cond_t*c,aether_mutex_t*m,u32 ms){if(!c||!m)return false;struct timeval tv;gettimeofday(&tv,NULL);struct timespec ts={(time_t)(tv.tv_sec+ms/1000),(long)tv.tv_usec*1000L+(long)(ms%1000)*1000000L};if(ts.tv_nsec>=1000000000L){ts.tv_sec++;ts.tv_nsec-=1000000000L;}return pthread_cond_timedwait(&c->c,&m->m,&ts)==0;}
void aether_cond_signal(aether_cond_t*c){if(c)pthread_cond_signal(&c->c);}void aether_cond_broadcast(aether_cond_t*c){if(c)pthread_cond_broadcast(&c->c);}
