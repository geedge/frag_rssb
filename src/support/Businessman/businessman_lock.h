#ifndef _BIZMAN_LOCK_H_INCLUDE_
#define _BIZMAN_LOCK_H_INCLUDE_
#include <pthread.h>
typedef pthread_mutex_t bizman_lock_t;

#define	bizman_init_lock(lock)	pthread_mutex_init(lock, NULL)
#define	bizman_lock(lock)	pthread_mutex_lock(lock)
#define	bizman_unlock(lock)	pthread_mutex_unlock(lock)
#define	bizman_trylock(lock)	pthread_mutex_trylock(lock)

typedef pthread_cond_t	bizman_cond_t;
#define	bizman_init_cond(cond)	pthread_cond_init(cond,NULL)
#define	bizman_cond_wait(cond,lock)	pthread_cond_wait(cond,lock)
#define	bizman_cond_timedwait(cond,lock,time) pthread_cond_timedwait(cond,lock,time)
#define	bizman_cond_signal(cond)	pthread_cond_signal(cond)
#endif //_BIZMAN_LOCK_H_INCLUDE_
