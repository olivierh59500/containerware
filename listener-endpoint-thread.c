#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define LOG_FACILITY                   "listener-endpoint"

#include "p_containerware.h"

static void *listener_thread_handler_(void *ptr);

/* Create a new endpoint thread for a listener
 *
 * Threading: Thread-safe; the listener's mutex must be held by the caller.
 */
int
listener_thread_create(LISTENER *p)
{
	if(pthread_create(&(p->thread), NULL, listener_thread_handler_, (void *) p))
	{
		return -1;
	}
	return 0;
}

/* Handler function for dedicated endpoint threads */
static void *
listener_thread_handler_(void *ptr)
{
	CONTAINER_REQUEST *req;
	LISTENER *me;
	int r;
	
	me = (LISTENER *) ptr;
	DPRINTF("endpoint thread is starting");
	pthread_mutex_lock(&(me->mutex));
	me->state = LS_RUNNING;
	for(;;)
	{
		DPRINTF("waiting for activity");
		r = me->endpoint->api->process(me->endpoint);
		if(r == -1)
		{
			LPRINTF(CWLOG_CRIT, "endpoint returned error: %s", strerror(errno));
			break;
		}
		if(!r)
		{
			LPRINTF(CWLOG_INFO, "endpoint terminated normally");
			break;
		}
		DPRINTF("request is ready to be acquired");
		r = me->endpoint->api->acquire(me->endpoint, &req);
		if(r)
		{
			LPRINTF(CWLOG_CRIT, "failed to acquire a request from endpoint: %s", strerror(errno));
			break;
		}
		DPRINTF("request has been acquired and will now be routed");
		route_request(req, me);
		req->api->release(req);
	}
	me->status = LS_ZOMBIE;
	me->status = r;
	me->thread = 0;
	pthread_mutex_unlock(&(me->mutex));
	DPRINTF("endpoint thread is terminating");
	return me;
}
