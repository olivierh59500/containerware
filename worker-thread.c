#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "worker-thread"

#include "p_containerware.h"

static void *worker_thread_(void *arg);

int
worker_thread_create(CONTAINER_WORKER_HOST *host)
{
	if(pthread_create(&(host->thread), NULL, worker_thread_, (void *) host))
	{
		return -1;
	}
	return 0;
}

/* Handler for an instance thread */
static void *
worker_thread_(void *arg)
{
	CONTAINER_WORKER_HOST *me;
	int r;
	
	me = (CONTAINER_WORKER_HOST *) arg;
	DHPRINTF(me, "waiting for worker to become initialised");
	pthread_mutex_lock(&(me->mutex));
	me->state = WS_IDLE;
	me->status = -1;
	if(!me->worker)
	{
		DHPRINTF(me, "host is not associated with a worker");
		me->state = WS_ZOMBIE;
		me->status = 255;
		pthread_mutex_unlock(&(me->mutex));
		return NULL;
	}
	DHPRINTF(me, "passing control to the worker");
	pthread_mutex_unlock(&(me->mutex));
	r = me->worker->api->process(me->worker);
	DHPRINTF(me, "worker terminated with exit status %d", r);
	pthread_mutex_lock(&(me->mutex));
	me->state = WS_ZOMBIE;
	me->status = r;
	pthread_mutex_unlock(&(me->mutex));
	return NULL;	
}
