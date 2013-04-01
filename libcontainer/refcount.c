/* Utilities for reference-counting */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_cw.h"

unsigned long
cw_addref(pthread_mutex_t *lock, unsigned long *refcount)
{
	unsigned long r;
	
	pthread_mutex_lock(lock);
	(*refcount)++;
	r = *refcount;
	pthread_mutex_unlock(lock);
	return r;
}

unsigned long
cw_release(pthread_mutex_t *lock, unsigned long *refcount)
{
	unsigned long r;
	
	pthread_mutex_lock(lock);
	(*refcount)--;
	r = *refcount;
	pthread_mutex_unlock(lock);
	return r;
}
