#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "instance-thread"

#include "p_containerware.h"

static void *instance_thread_(void *arg);

int
instance_thread_create(CONTAINER_INSTANCE_HOST *host)
{
	if(pthread_create(&(host->thread), NULL, instance_thread_, (void *) host))
	{
		return -1;
	}
	return 0;
}

/* Handler for an instance thread */
static void *
instance_thread_(void *arg)
{
	CONTAINER_INSTANCE_HOST *me;
	int r;
	
	me = (CONTAINER_INSTANCE_HOST *) arg;
	DHPRINTF(me, "waiting for instance to become initialised");
	pthread_mutex_lock(&(me->mutex));
	me->state = IS_IDLE;
	me->status = -1;
	if(!me->instance)
	{
		DHPRINTF(me, "instance host is not associated with an instance");
		me->state = IS_ZOMBIE;
		me->status = 255;
		pthread_mutex_unlock(&(me->mutex));
		return NULL;
	}
	DHPRINTF(me, "passing control to the instance");
	pthread_mutex_unlock(&(me->mutex));
	r = me->instance->api->process(me->instance);
	DHPRINTF(me, "instance terminated with exit status %d", r);
	pthread_mutex_lock(&(me->mutex));
	me->state = IS_ZOMBIE;
	me->status = r;
	pthread_mutex_unlock(&(me->mutex));
	return NULL;	
}
